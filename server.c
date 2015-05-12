#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define OCCUPIED 1
#define AVAILABLE 0

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void argread(
    int argc,
    char const *argv[],
    int &rows,
    int &cols,
    int &portno
)
{

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
    int rows, cols, freeplaces;

    int sockfd, newsockfd, portno, n;
    socklen_t clilen;
    char buffer[64];
    struct sockaddr_in serv_addr, cli_addr;

    argread(argc, argv, &rows, &cols, &portno)

    int **places = malloc(rows * sizeof(int *));
    places[0] = malloc(rows * cols * sizeof(int));
    for(i = 1; i < rows; i++)
        places[i] = places[0] + i * cols;

    freeplaces = rows * cols;

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
            "%d:%d;"
            rowno,
            colno
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