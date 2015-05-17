#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define DEFPORT 8888
#define MAXATTEMPTS 3

void error(const char *msg)
{
  perror(msg);
  exit(0);
}

/*
 * Funcion encargada de la lectura de argumentos por linea de comandos
 */
void argread(
  int argc,
  char *argv[],
  int * rowno,
  int * colno,
  char ** hostname,
  int * portno
){
  int rflag = 0; // rows
  int cflag = 0; // cols
  int hflag = 0; // hostname
  int pflag = 0; // port

  int index;
  int c;
  char *cvalue = NULL;
  while ((c = getopt (argc, argv, "r:c:h:p:")) != -1)
    switch (c) {
      case 'r': // bandera para especificar la fila (Row)
        rflag = 1;
        sscanf (optarg,"%d", rowno);
        break;
      case 'c': // bandera para especificar la columna (Column)
        cflag = 1;
        sscanf (optarg,"%d", colno);
        break;
      case 'h': // bandera para especificar el servidor (Hostname)
        hflag = 1;
        * hostname = optarg;
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

  if (!hflag || !rflag || !cflag){
    fprintf(
      stderr,
      "Uso: %s -h <hostname> -p <puerto> -r <fila> -c <columna>\n"
      "\tlos argumentos de hostname, fila y columna son obligatorios, mientras que "
      "el puerto es opcional.\n\tSi el puerto no es especificado, se usará el "
      "puerto %d por defecto.\n",
      argv[0],
      DEFPORT
      );
    exit(1);
  }

  if (!pflag){
    printf("Usando el puerto por defecto, %d\n", DEFPORT);
    * portno = DEFPORT;
  }
}

int main(int argc, char *argv[])
{
  int rowno, colno, attempts;
  int sockfd, portno, n;
  char * hostname;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[164]; // Buffer para la comunicacion

    /* la siguiente funcion llena los valores de rowno, colno, hostname
    y portno a partir de los valores indicados por el usuario */
  argread(argc, argv, &rowno, &colno, &hostname, &portno);

    // Abrimos un socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

    /* La siguiente funcion retorna una estructura de tipo hostent
    para un name dado. Name puede ser un hostname o una direccion IPv4.
    Como en nuestro caso siempre es una direccion IPv4, lo unico
    que hace es copiar el name en el campo h_name */
  server = gethostbyname(hostname);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host\n");
    exit(0);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));

    // Definimos los atributos del servidor:
  serv_addr.sin_family = AF_INET;

    // Obtenemos el campo h_addr de la variable server (de tipo hostent)
  bcopy(
    (char *)server->h_addr,
    (char *)&serv_addr.sin_addr.s_addr,
    server->h_length
  );
  serv_addr.sin_port = htons(portno);

    // Se intenta la conexion al servidor
  attempts = 0;
  while (attempts < MAXATTEMPTS){
    if (connect(
        sockfd,
        (struct sockaddr *) &serv_addr,
        sizeof(serv_addr)
      ) < 0
    ){
      attempts = MAXATTEMPTS + 1;
    }
    attempts++;
  }
  if (attempts == MAXATTEMPTS) error("ERROR connecting");

  bzero(buffer,164);

    // Escribimos el puesto deseado en el buffer
  sprintf(buffer,"%d:%d;",rowno,colno);

    // Y lo pasamos al servidor
  n = write(sockfd,buffer,strlen(buffer));
  if (n < 0)
    error("ERROR writing to socket");

  bzero(buffer,164);

    // Leemos la respuesta del servidor
  n = read(sockfd, buffer, 163);
  if (n < 0)
    error("ERROR reading from socket");

  if (buffer[0] == '0'){
      printf(
        "La reserva ha sido realizada satisfactoriamente.\n"
      );
  } else if (buffer[0] == '1') {
      printf(
        "El puesto solicitado se encuentra ocupado.\n"
        "Intente de nuevo con un puesto de la actual lista de puestos disponibles:\n\n"
      );
      int i, fa, ca, cols; // iterador, fila actual, columna actual, columnas del vagon
      fa = 1;
      ca = 1;
      cols = buffer[strlen(buffer)-2] - '0'; // Columnas del vagon a enteros
      for (i = 1; i < strlen(buffer); i++) {
        if (buffer[i] == '0') printf("%d:%d;\t",fa,ca);
        if (ca == cols) { // Condicion que evalua cambio de fila
            printf("\n");
            fa++;
            ca = 0;
        }
        ca++;
      }
  } else if (buffer[0] == '2') {
      printf(
        "Ofrecemos nuestras disculpas, ya no quedan asientos libres.\n"
      );
  } else if (buffer[0] == '3') {
      printf(
        "El asiento %d:%d no existe. Por favor, intente de nuevo.\n",
        rowno,
        colno
      );
  } else {
      error("ERROR communication with server failed");
      exit(1);
  }

  close(sockfd);
  return 0;
}
