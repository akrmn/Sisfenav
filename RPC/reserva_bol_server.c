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

#include "reserva_bol.h" // necesario para RPC

#define OCCUPIED '1'
#define AVAILABLE '0'

#define NOROWS 10
#define NOCOLS 4

char places[NOROWS * NOCOLS + 1] = {0};
int  freeplaces = NOROWS*NOCOLS;

/*
 * Funcion disponible remotamente que intenta reservar un puesto dado.
 */
int *
reserve_1_svc(seat *argp, struct svc_req *rqstp)
{
  static int  result;

    // extraccion de datos del objeto "asiento".
  int rowno = argp->rowno - 1;
  int colno = argp->colno - 1;

  if (
    rowno >= NOROWS ||
    colno >= NOCOLS ||
    rowno < 0 ||
    colno < 0
  ){
    result = 3; // fracaso, puesto fuera del vagon.
  } else if (freeplaces == 0) {
    result = 2; // fracaso, ocupado y no quedan disponibles.
  } else if (places[rowno*NOCOLS + colno] != OCCUPIED) {
    places[rowno*NOCOLS + colno] = OCCUPIED;
    freeplaces -= 1;
    result = 0; // exito
  } else {
    result = 1; // fracaso, existen puestos vacios.
  }

  return &result;
}

/*
 * Funcion disponible remotamente que devuelve el numero de filas del vagon.
 */
int *
rows_1_svc(void *argp, struct svc_req *rqstp)
{
  static int  result;

  result = NOROWS;

  return &result;
}

/*
 * Funcion disponible remotamente que devuelve el numero de columnas del vagon.
 */
int *
cols_1_svc(void *argp, struct svc_req *rqstp)
{
  static int  result;

  result = NOCOLS;

  return &result;
}

/*
 * Funcion disponible remotamente que informa sobre
 * los puestos vacios del vagon.
 */
char **
listfree_1_svc(void *argp, struct svc_req *rqstp)
{
    // se devuelve un string terminado en null, que es la lista de puestos.
  static char * result = places;

    // se "normaliza" la lista de puestos para que sirva como string.
  int i, j;
  for (i = 0; i < NOROWS; i++) {
    for (j = 0; j < NOCOLS; j++) {
      places[i*NOCOLS + j]
        = (
          places[i*NOCOLS + j] == OCCUPIED ?
            OCCUPIED
          : AVAILABLE
        );
    }
  }

    // se cierra el string con un caracter nulo.
  places[NOROWS * NOCOLS] = '\0';

    // se devuelve un apuntador al string.
  return &result;
}
