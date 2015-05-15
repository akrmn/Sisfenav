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
      case 'r':
        rflag = 1;
        sscanf (optarg,"%d", rowno);
        break;
      case 'c':
        cflag = 1;
        sscanf (optarg,"%d", colno);
        break;
      case 'h':
        hflag = 1;
        * hostname = optarg;
        break;
      case 'p':
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

  if (!hflag){
    fprintf(stderr, "Must give host name or IP address with -h option\n");
    exit(1);
  }

  if (!rflag){
    fprintf(stderr, "Must give row number with -r option\n");
    exit(1);
  }

  if (!cflag){
    fprintf(stderr, "Must give column number with -c option\n");
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

    argread(argc, argv, &rowno, &colno, &hostname, &portno);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
 /*La siguiente funcion retorna una estructura de tipo hostent
 para un name dado. Name puede ser un hostname o una direccion IPv4.
 Como en nuestro caso siempre es una direccion IPv4, lo unico
 que hace es copiar el name en el campo h_name*/
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
 // Definimos los atributos del servidor:
    serv_addr.sin_family = AF_INET;
    // Obtenemos el campo h_addr de la variable server (de tipo hostent)
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    // Se intenta la conexion al servidor
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    bzero(buffer,64);
    sprintf(buffer,"%d:%d;",rowno,colno); // Obtenemos el puesto deseado
    n = write(sockfd,buffer,strlen(buffer)); // Y lo pasamos al servidor
    if (n < 0)
         error("ERROR writing to socket");
    bzero(buffer,64);
    // Leemos la respuesta del servidor
    n = read(sockfd, buffer, 63);
    if (n < 0)
         error("ERROR reading from socket");
    int resp = atoi(buffer);
    if (resp == 0){
        printf("La reserva ha sido realizada con éxito.\n");
    } else if (resp == 1){
        printf("El puesto deseado ya se encuentra ocupado. Intente de nuevo.\n");
        // Falta lo de imprimir los puestos disponibles.
    } else if (resp == 2){
        printf("Lo sentimos. No quedan asientos libres en el vagón.\n");
    } else if (resp == 3){
        printf("El puesto %d:%d no existe. Intente con uno válido.\n", rowno, colno);
    } else {
        error("ERROR communication with server failed");
    }
    close(sockfd);
    return 0;
}
