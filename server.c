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

#define MAXROWS 100
#define DEFROWS 10
#define MAXCOLS 100
#define DEFCOLS 4

#define DEFPORT 8888

void argread(
  int argc,
  char const *argv[],
  int * rows,
  int * cols,
  int * portno
){
  int rflag = 0; // rows
  int cflag = 0; // cols
  int pflag = 0; // port

  // int index;
  int c;
  char *cvalue = NULL;
  while ((c = getopt (argc, argv, "r:c:p:")) != -1)
    switch (c) {
      case 'r':
        rflag = 1;
        sscanf (optarg,"%d", rows);
        if (*rows > MAXROWS || rows <= 0){
          fprintf(
            stderr,
            "Invalid number of rows, number must be "
            "larger than zero and smaller than %d\n",
            MAXROWS
          );
          exit(1);
        }
        break;
      case 'c':
        cflag = 1;
        sscanf (optarg,"%d", cols);
        if (*cols > MAXCOLS || cols <= 0){
          fprintf(
            stderr,
            "Invalid number of columns, number must be "
            "larger than zero and smaller than %d\n",
            MAXCOLS
          );
          exit(1);
        }
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

  if (!rflag)
    * rows = DEFROWS;

  if (!cflag)
    * cols = DEFCOLS;

  if (!pflag)
    * portno = DEFPORT;
}

void error(
  const char *msg
){
  perror(msg);
  exit(1);
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

int main(int argc, char const *argv[])
{
  int rows, cols;
  int portno;
  argread(argc, argv, &rows, &cols, &portno);
  int freeplaces = rows * cols;


  int sockfd, newsockfd, n;
  socklen_t clilen;
  char buffer[64];
  struct sockaddr_in serv_addr, cli_addr;

  int **places = malloc(rows * sizeof(int *));
  places[0] = malloc(rows * cols * sizeof(int));
  int i;
  for(i = 1; i < rows; i++)
    places[i] = places[0] + i * cols;

  sockfd  = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (
    bind(
      sockfd,
      (struct sockaddr *) &serv_addr,
      sizeof(serv_addr)
    ) < 0
  ) error("ERROR on binding");

  listen(sockfd, 5);

  while(1){
    clilen = sizeof(cli_addr);
    newsockfd = accept(
      sockfd,
      (struct sockaddr *) &cli_addr,
      &clilen
    );
    if (newsockfd < 0)
      error("ERROR on accept");

    bzero(buffer,64);
    n = read(newsockfd,buffer,63);

    if (n < 0) error("ERROR reading from socket");

    int rowno, colno;
    sscanf(
      buffer,
      "%d:%d;",
      &rowno,
      &colno
    );

    rowno--;
    colno--;

    if (
      rowno >= rows ||
      colno >= cols ||
      rowno < 0 ||
      colno < 0
    ){
      n = write(newsockfd,"3",1); // non-existing place
    } else if (freeplaces == 0){
      n = write(newsockfd,"2",1); // full train
    } else if (reserve(places, rowno, colno)){
      freeplaces--;
      n = write(newsockfd,"0",1); // successfully reserved
    } else {
      n = write(newsockfd,"1",1); // occupied place
    }
  }
}
