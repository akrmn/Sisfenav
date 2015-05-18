#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define OCCUPIED 1
#define AVAILABLE 0

#define MAXROWS 20
#define DEFROWS 10
#define MAXCOLS 8
#define DEFCOLS 4

#define DEFPORT 8888

void error(
  const char *msg
){
  perror(msg);
  exit(1);
}

/*
 * Funcion encargada de la lectura de argumentos por linea de comandos
 */
void argread(
  int argc,
  char *argv[],
  int * rows,
  int * cols,
  int * portno
){
  int rflag = 0; // rows
  int cflag = 0; // cols
  int pflag = 0; // port

  int c;
  char *cvalue = NULL;
  while ((c = getopt (argc, argv, "r:c:p:")) != -1)
    switch (c) {
      case 'r': // bandera para especificar la cantidad de filas (Rows)
        rflag = 1;
        sscanf (optarg,"%d", rows);
        if (*rows > MAXROWS || *rows <= 0){
          fprintf(
            stderr,
            "Número inválido de filas. El mismo debe ser "
            "mayor que 0 y menor que %d\n",
            MAXROWS
          );
          exit(1);
        }
        break;
      case 'c': // bandera para especificar la cantidad de columnas (Columns)
        cflag = 1;
        sscanf (optarg,"%d", cols);
        if (*cols > MAXCOLS || *cols <= 0){
          fprintf(
            stderr,
            "Número inválido de columnas. El mismo debe ser "
            "mayor que 0 y menor que %d\n",
            MAXCOLS
          );
          exit(1);
        }
        break;
      case 'p': // bandera para especificar el puerto (Port)
        pflag = 1;
        sscanf (optarg,"%d", portno);
        break;
      default:
        fprintf(
          stderr,
          "Flag desconocido -%c\n",
          c
        );
        exit(1);
    }

  if (!rflag){
    printf("Usando el número de filas por defecto, %d\n", DEFROWS);
    * rows = DEFROWS;
  }

  if (!cflag){
    printf("Usando el número de columnas por defecto, %d\n", DEFCOLS);
    * cols = DEFCOLS;
  }

  if (!pflag){
    printf("Usando el puerto por defecto, %d\n", DEFPORT);
    * portno = DEFPORT;
  }
}

int reserve(
  int ** places,
  int rowno,
  int colno
){
  if (places[rowno][colno] == AVAILABLE){
    places[rowno][colno] = OCCUPIED;
    return 1;
  } else {
    return 0;
  }
}

int main(int argc, char *argv[])
{
  int rows, cols;
  int portno;
    /* la siguiente funcion llena los valores de rows, cols y portno a
    partir de los valores indicados por el usuario */
  argread(argc, argv, &rows, &cols, &portno);
  int freeplaces = rows * cols;

    // Creamos la matriz de puestos del vagon
  int **places = malloc(rows * sizeof(int *));
  places[0] = malloc(rows * cols * sizeof(int));
  int i;
  for(i = 1; i < rows; i++)
    places[i] = places[0] + i * cols;

  int sockfd, newsockfd, n;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  char buffer[164]; // Buffer para la comunicacion

    // Abrimos un socket
  sockfd  = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  bzero((char *) &serv_addr, sizeof(serv_addr));

    // Definimos los atributos del servidor
  serv_addr.sin_family = AF_INET;
    // Se pueden recibir peticiones desde cualquier IP, luego usamos INADDR_ANY
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

    // Se asocia el socket a la direccion del servidor
  if (
    bind(
      sockfd,
      (struct sockaddr *) &serv_addr,
      sizeof(serv_addr)
    ) < 0
  ) error("ERROR on binding");

    /* Se espera que los clientes intenten conectarse,
    dejando hasta cinco en la cola */
  listen(sockfd, 5);

    /* El servidor acepta conexiones siempre que este activo,
    asi que usamos un while infinito */
  while(1){
    clilen = sizeof(cli_addr);

      // Se acepta la primera conexion de la cola
    newsockfd = accept(
      sockfd,
      (struct sockaddr *) &cli_addr,
      &clilen
    );
    if (newsockfd < 0)
      error("ERROR on accept");

    bzero(buffer,164);

      // Se lee el mensaje del cliente al buffer
    n = read(newsockfd,buffer,163);
    if (n < 0) error("ERROR reading from socket");

      // Se extrae la información deseada del buffer
    int rowno, colno;
    sscanf(
      buffer,
      "%d:%d;", // formato especificado por el protocolo
      &rowno,
      &colno
    );

    rowno--;
    colno--;
      /* Se verifica en que categoria cae el puesto solicitado y se
      envia la respuesta apropiada */
    if (
      rowno >= rows ||
      colno >= cols ||
      rowno < 0 ||
      colno < 0
    ){
      n = write(newsockfd,"3",1); // Puesto inexistente
    } else if (freeplaces == 0){
      n = write(newsockfd,"2",1); // Vagon lleno
    } else if (reserve(places, rowno, colno)){
      freeplaces--;
      n = write(newsockfd,"0",1); // Reserva exitosa
    } else {
      buffer[0] = '1'; // Puesto ocupado
      /* Resumimos la lista en el string del buffer tal que:
         buffer[auxBuff] = 0 si el puesto esta libre
         buffer[auxBuff] = 1 si el puesto esta ocupado */
      int auxBuff = 1; // Variable para llenar el buffer
      int j; // Variable de iteracion sobre las columnas
      for (i = 0; i < rows; i++) {
          for (j = 0; j < cols; j++) {
              buffer[auxBuff] = (places[i][j] == 0) ?  '0' : '1';
              auxBuff++;
          }
      }
      buffer[auxBuff] = cols + '0'; // Pasa el numero de filas del vagon al cliente.
      buffer[auxBuff+1] = '\n';
      n = write(newsockfd, buffer, sizeof(buffer));
    }
  }
}