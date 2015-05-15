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
  char const *argv[],
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
          "Unknown flag -%c",
          c
        );
        exit(1);
    }

  if (!hflag || !rflag || !cflag){
    fprintf(
      stderr,
      "Usage: %s -h <hostname> -p <port> -r <row> -c <column>\n"
      "\tthe hostname, row and column arguments are mandatory, while "
      "the port is optional.\n\tIf the port is left out, the default "
      "value of %d will be used.\n",
      argv[0],
      DEFPORT
      );
    exit(1);
  }

  if (!pflag){
    printf("Using default port, %d\n", DEFPORT);
    * portno = DEFPORT;
  }
}

int main(int argc, char *argv[])
{
  int rowno, colno;
  int sockfd, portno, n;
  char * hostname;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[64]; // Buffer para la comunicacion

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
  if (
    connect(
      sockfd,
      (struct sockaddr *) &serv_addr,
      sizeof(serv_addr)
    ) < 0
  )
    error("ERROR connecting");

  bzero(buffer,64);

    // Escribimos el puesto deseado en el buffer
  sprintf(buffer,"%d:%d;",rowno,colno);

    // Y lo pasamos al servidor
  n = write(sockfd,buffer,strlen(buffer));
  if (n < 0)
    error("ERROR writing to socket");

  bzero(buffer,64);

    // Leemos la respuesta del servidor
  n = read(sockfd, buffer, 63);
  if (n < 0)
    error("ERROR reading from socket");
  int resp = atoi(buffer);

  switch (resp){
    case 0:
      printf(
        "Your reservation has been successfully completed.\n"
      );
      break;
    case 1:
      printf(
        "The requested seat is unavailable. Please try again.\n"
      );
      // MOSTRAR PUESTOS DISPONIBLES
      break;
    case 2:
      printf(
        "Our sincere apologies, there are no free places in this wagon.\n"
      );
      break;
    case 3:
      printf(
        "The seat %d:%d does not exist. Please try again with a valid seat.\n",
        rowno,
        colno
      );
      break;
    default:
      error("ERROR communication with server failed");
      exit(1);
  }

  close(sockfd);
  return 0;
}
