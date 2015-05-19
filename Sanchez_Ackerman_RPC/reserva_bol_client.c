#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "reserva_bol.h" // necesario para RPC

/*
 * Funcion encargada de la lectura de argumentos por linea de comandos
 */
void argread(
  int argc,
  char *argv[],
  int * rowno,
  int * colno,
  char ** hostname
){
  int rflag = 0; // rows
  int cflag = 0; // cols
  int hflag = 0; // hostname

  int index;
  int c;
  char *cvalue = NULL;
  while ((c = getopt (argc, argv, "r:c:h:")) != -1)
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
      "Uso: %s -h <hostname> -r <fila> -c <columna>\n"
      "\ttodos los argumentos son obligatorios.\n",
      argv[0]
    );
    exit(1);
  }
}

/*
 * Funcion auxiliar que muestra los puestos disponibles
 */
void
showfree(CLIENT *clnt)
{
  int *rows, *cols;
  char ** message;
  char *rows_1_arg;
  char *cols_1_arg;
  char *listfree_1_arg;

    // llamada remota para conocer el numero de filas del vagon
  rows = rows_1((void*)&rows_1_arg, clnt);
  if (rows == (int *) NULL) {
    clnt_perror (clnt, "call failed");
  }

    // llamada remota para conocer el numero de columnas del vagon
  cols = cols_1((void*)&cols_1_arg, clnt);
  if (cols == (int *) NULL) {
    clnt_perror (clnt, "call failed");
  }

    // llamada remota para conocer los puestos disponibles en el vagon
  message = listfree_1((void*)&listfree_1_arg, clnt);
  if (message == (char **) NULL) {
    clnt_perror (clnt, "call failed");
  }

    // imprimir en pantalla los puestos libres
  int i, j;
  for (i = 0; i < *rows; i++) {
    for (j = 0; j < *cols; j++) {
      if ((*message)[i*(*cols) + j] == '0') {
        printf("%d:%d;\t", i+1, j+1);
      }
    }
    printf("\n");
  }

}

void
reserva_bol_prog_1(char *host, int rowno, int colno)
{
  CLIENT *clnt; // variable usada por RPC
  int  *result_1;
  seat  reserve_1_arg = {rowno, colno}; // objeto "asiento" que se pasa por RPC
  char * *result_2;
  char *listfree_1_arg;

    // creacion del objeto "cliente", generado por rpcgen
  clnt = clnt_create (host, RESERVA_BOL_PROG, RESERVA_BOL_VERS, "udp");
  if (clnt == NULL) {
    clnt_pcreateerror (host);
    exit (1);
  }

    // llamada remota para verificar disponibilidad del puesto
  result_1 = reserve_1(&reserve_1_arg, clnt);
  if (result_1 == (int *) NULL) {
    clnt_perror (clnt, "call failed");
  }

  switch(*result_1){
    case 0: // exito
      printf(
        "La reserva ha sido realizada satisfactoriamente.\n"
      );
      break;
    case 1: // fracaso, ocupado. Se muestran puestos vacios.
      printf(
        "El puesto solicitado se encuentra ocupado.\n"
        "Intente de nuevo con un puesto de la actual lista de puestos disponibles:\n\n"
      );
      showfree(clnt);
      break;
    case 2: // fracaso, ocupado y sin puestos libres.
      printf(
        "Ofrecemos nuestras disculpas, ya no quedan asientos libres.\n"
      );
      showfree(clnt);
    case 3: // fracaso, puesto fuera del vagon.
      printf(
        "El asiento %d:%d no existe. Por favor, intente de nuevo.\n",
        rowno,
        colno
      );
    default: // fracaso, mensaje inesperado recibido del servidor
      fprintf(
        stderr,
        "El servidor devolvio el codigo inesperado `%d'.\n",
        *result_1
      );
      exit(1);
      break;
  }
  clnt_destroy (clnt);
}


int
main (int argc, char *argv[])
{
  int rowno, colno, result;
  char *hostname;

    // lectura de argumentos por linea de comandos
  argread(argc, argv, &rowno, &colno, &hostname);

    // procedimiento principal
  reserva_bol_prog_1(hostname, rowno, colno);

exit (0);
}
