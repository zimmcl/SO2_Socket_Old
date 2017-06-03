/*
 * cliente.c
 *
 *  Created on: 05/04/2017
 *      Author: ezequiel
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define TAM 4096

int
conectar(char*, int*, char*, char*);

int
recibirDatos(int puerto, char *direccionIp);

int
main(int argc, char *argv[])
{
  int sockfd, puerto, n, validacion = 0;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  char usuario[10], contras[10];
  char direccionIp[20];
  direccionIp[0] = '\0';

  char buffer[TAM];
  char bufferConexion[TAM];
  bufferConexion[0] = '\0';

  int conexion = 0;
  printf("\nInicio del programa cliente");
  printf("\n===========================\n\n");
  do
    {
      printf("desconectado");
      printf("~$ ");
      fgets(bufferConexion, TAM - 1, stdin);
      conexion = conectar(bufferConexion, &puerto, direccionIp, usuario);
    }
  while (conexion == 0);

  /* Crea socket TCP */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    {
      perror("ERROR apertura de socket");
      exit(1);
    }

  server = gethostbyname(direccionIp);

  if (server == NULL)
    {
      fprintf(stderr, "Error, no existe el host\n");
      exit(0);
    }
  /* Construye la estructura server sockaddr_in */
  memset((char *) &serv_addr, '0', sizeof(serv_addr));		/* Limpia la estructura */
  serv_addr.sin_family = AF_INET;		/* Internet/IP */
  bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);	/* dirección IP */
  serv_addr.sin_port = htons(puerto);	/* puerto del servidor */
  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("conexion");
      exit(1);
    }

  /* Establecida la conexión con el servidor, se pide al usuario que se valide
  *	 mediante el ingreso de la contraseña asociada.*/
  while (validacion == 0)
    {
      printf("Ingrese la contraseña: ");
      fgets(contras, sizeof(buffer), stdin);
      strcpy(buffer, usuario);
      strcat(buffer, ":");
      strcat(buffer, contras);

      /* Envia información de usuario y contraseña al servidor */
      n = write(sockfd, buffer, strlen(buffer));
      if (n == -1)
        {
          perror("Error en el envio de mensaje");
          exit(1);
        }
      else
        {
          printf("Contraseña enviada, esperando validacion del servidor...\n");
        }
      memset(&buffer[0], 0, sizeof(buffer));
      /* Respuesta del servidor */  
      n = read(sockfd, buffer, sizeof(buffer));
      if (n == -1)
        {
          perror("Error en el mensaje recibido");
          exit(1);
        }
      if (buffer[0] != 'I')
        {
          printf(ANSI_COLOR_GREEN" Usuario validado: ");
          printf(ANSI_COLOR_CYAN"%s\n", usuario);
          validacion = 1;
        }
      else
        {
          printf(ANSI_COLOR_RED" Nombre de usuario y/o contraseña incorrecto\n"ANSI_COLOR_RESET);
          memset(buffer, '\0', TAM);
          printf("Ingrese el usuario: ");
          fgets(usuario, sizeof(buffer), stdin);
          usuario[strlen(usuario) - 1] = '\0';
        }
    }
  /* Set de opciones para interactuar con el servidor.
   * El bucle finaliza con el ingreso de 'disconnect' */  
  while (1)
    {
      printf(ANSI_COLOR_RESET"\n%-20sOPCIONES\n", " ");
      printf(" 1)listar \n"
        " 2)descargar <N°estacion> \n"
        " 3)diario_precipitacion <N°estacion> \n"
        " 4)mensual_precipitacion <N°estacion> \n"
        " 5)promedio <variable> \n"
        " 6)disconnect \n\n");
      printf(ANSI_COLOR_CYAN"%s", usuario);
      printf(ANSI_COLOR_RESET"@%s:~$", server->h_name);

      memset(buffer, '\0', TAM);
      fgets(buffer, TAM - 1, stdin);

      /* Envia al servidor la petición del usuario */
      n = write(sockfd, buffer, strlen(buffer));
      if (n < 0)
        {
          perror("escritura de socket");
          exit(1);
        }

      buffer[strlen(buffer) - 1] = '\0';

      /* Recibe los datos del servidor */
      n = read(sockfd, buffer, TAM);
      if (n < 0)
        {
          perror("lectura de socket");
          exit(1);
        }
      if (!strcmp("descargar", buffer))
        {
          recibirDatos(puerto, direccionIp);
          continue;
        }

      if (!strcmp("disconnected", buffer))
        {
          printf("La conexión con '%s' en el puerto '%d' ha finalizado exitosamente\n\n", direccionIp, puerto);
          exit(0);
        }
      printf("%s", buffer);
    }
  return 0;
}

/**
 * @brief Verifica que el comando de conexión ingresado sea de la forma
 * 'connect usuario@direccionIp:puerto'.
 * 
 * @param buffer Comando ingresado mediante la terminal.
 * @param puerto Puerto del servidor al que se intentara conectar.
 * @param direccionIp Dirección IP del servidor.
 * @param usuario Usuario que intenta loguearse al servidor.
 * 
 * @return Si la sintaxis de conexión es correcta retorna 1, caso contrario
 * retorna 0.
 */
int
conectar(char *buffer, int *puerto, char *direccionIp, char *usuario)
{
  char c[2] = " ", d[2] = "@", e[2] = ":";
  char *token;
  token = strtok(buffer, c);

  if (!strcmp(token, "connect") && token != NULL)
    {
      token = strtok(NULL, d);
      if (token != NULL)
        {
          strcpy(usuario, token);
          token = strtok(NULL, e);
          if (token != NULL)
            {
              strcpy(direccionIp, token);
              token = strtok(NULL, c);
              if (token != NULL)
                {
                  *puerto = atoi(token);
                  return 1;
                }
              else
                {
                  printf("\nError: puerto inválido\n");
                  return 0;
                }
            }
          else
            {
              printf("\nError: Dirección IP inválida\n");
              return 0;
            }
        }
      else
        {
          printf("\nError: Usuario inválido\n");
          return 0;
        }
    }
  else
    {
      printf("Comando desconocido. Intente 'connect <usuario>@<ip>:<puerto>'\n");
      return 0;
    }

}

/**
 * @brief Se reciben datos desde el servidor mediante un socket UDP.
 * 
 * @param puerto Puerto del servidor
 * @param host Direccion IP del servidor
 * 
 * @return Si se realiza la transferencia retorna 0, en caso contrario -1.
 */
int
recibirDatos(int puerto, char *host)
{
  FILE *fptSalida;
  fptSalida = fopen("archivos/datosSalida.CSV", "w");
  int sockfd, n;
  socklen_t tamano_direccion;
  struct sockaddr_in dest_addr;
  struct hostent *server;
  char buffer[TAM];
  server = gethostbyname(host);
  /* Crea socket UDP */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      perror("apertura de socket");
      return -1;
    }

  /* Construye la estructura server sockaddr_in */
  dest_addr.sin_family = AF_INET;	/* Internet/IP */
  dest_addr.sin_port = htons(puerto);	/* puerto del server */
  dest_addr.sin_addr = *((struct in_addr *) server->h_addr); /* dirección IP */ 
  memset(&(dest_addr.sin_zero), '\0', 8);
  tamano_direccion = sizeof(dest_addr);

  /* Envia al servidor confirmación para que comience el envio de datos */
  n = sendto(sockfd, "OK", 3, 0, (struct sockaddr *) &dest_addr, tamano_direccion);
  if (n < 0)
    {
      perror("Escritura en socket");
      return -1;
    }

  printf("Servicio UDP establecido, descargando datos... \n");
  n = recvfrom(sockfd, buffer, TAM, 0, (struct sockaddr *) &dest_addr, &tamano_direccion);
  fprintf(fptSalida, "%s", buffer);
  int terminar = 1;

  /* Finaliza la recepcion de datos cuando el servidor envia 'terminar'.
   * Luego se cierra el socket. */
  while (terminar != 0)
    {
      n = recvfrom(sockfd, buffer, TAM, 0, (struct sockaddr *) &dest_addr, &tamano_direccion);
      if (n < 0)
        {
          perror("Lectura de socket");
          return -1;
        }
      fprintf(fptSalida, "%s", buffer);
      if (strcmp(buffer, "terminar") == 0)
        {
          terminar = 0;
        }
    }
  printf("Datos descargados correctamente en archivos/datosSalida.CSV\n");
  close(sockfd);
  return 0;
}

