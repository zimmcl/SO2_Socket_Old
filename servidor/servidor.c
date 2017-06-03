/*
 * servidor.c
 *
 *  Created on: 05/04/2017
 *      Author: ezequiel
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define TAM 4096

int
validacion(char *buffer);

int
enviarDatos(int, int, int);

void
listar(char *buffer);

int
constEstruc();

void
getDatos(char *buffer, char *filtro, int);

void
precip_diarias(char *buffer, int, int, int);

/** 
 * @brief Estructura que almacena cada dato recibido
 */

struct dato
{
  int numero;		
  char *estacion;
  int id_localidad;
  char *fecha;
  float temperatura;
  float humedad;
  float punto_rocio;
  float precipitacion;
  float velocidad_viento;
  char *direccion_viento;
  float rafaga_max;
  float presion;
  float radiacion_solar;
  float temp_suelo_1;
  float temp_suelo_2;
  float temp_suelo_3;
  float hum_suelo_1;
  float hum_suelo_2;
  float hum_suelo_3;
  float hum_hoja;
};
struct dato *registro[100000];
int estaciones[10]; //maximo 10 estaciones

const char *Sensores[] =
  { "Termometro", //Temperatura
      "Higrometro", //Humedad
      "Pluviometro", //Precipitacion
      "Anemometro", //Velocidad del viento
      "Veleta", //Direccion del viento
      "Barometro", //Presion
      "Piranometro", //Radiacion solar
      "Termometro_Suelo_1", //Temp suelo profundidad/locacion 1
      "Termometro_Suelo_2", //Temp suelo profundidad/locacion 2
      "Termometro_Suelo_3", //Temp suelo profundidad/locacion 3
      "Higrometro_suelo_1", //Humedad suelo profundidad/locacion 1
      "Higrometro_suelo_2", //Humedad suelo profundidad/locacion 2
      "Higrometro_suelo_3" //Humedad suelo profundidad/locacion 3
    };

const char *Variables[] =
  { "0", "1", "2", "3", "temperatura", "humedad", "punto_rocio", "precipitacion", "velocidad_viento", "9", "10",
      "presion", "radiacion_solar", "temp_suelo_1", "temp_suelo_2", "temp_suelo_3", "hum_suelo_1", "hum_suelo_2",
      "hum_suelo_3", "hum_hoja" };

int
main(int argc, char *argv[])
{
  int i = 0;
  char *argum[3] =
    { "", "", "" };
  char *str;
  int recs = constEstruc();

  int sockfd, newsockfd, puerto, pid;
  socklen_t clilen;
  char buffer[TAM];
  char pass[20];
  struct sockaddr_in serv_addr, cli_addr;
  int n;

  /*
   int socket( int domain, int type, int protocol );
   domain: corresponde a los tipos de socket vistos (AF_UNIX, AF_INET, etc.)
   type: Establece si es secuencia de caracteres o datagrama (SOCK_DGRAM, SOCK STREAM).
   protocol: protocolo a utilizar, generalmente se usa 0 para que adopte el valor por defecto basado en domain y type.
   La funcion devuelve un numero positivo si la creacion fue exitosa y un valor negativo en caso contrario
   */

  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0)
    {
      perror(" apertura de socket ");
      exit(1);
    }

  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  puerto = 6020;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(puerto);

  /**
   *Para la funcion bind(int sd, struct sockaddr *my_addr, int addrlen)
   *sd: file descriptor devuelto por socket()
   *my_addr: estructura que contiene la direccion, sockaddr_un para UNIX y sockaddr_in para INTERNET,
   *que es convertida a sockaddr mediante casting
   *addrlen: longitud del campo direccion
   *
   *Esta funcion devuelve cero en caso exitoso y un numero negrativo en caso de fallo.
   */

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("Error de ligadura de Socket");
      exit(1);
    }

  printf("Proceso: %d - socket disponible: %d\n", getpid(), ntohs(serv_addr.sin_port));

  listen(sockfd, 5);
  clilen = sizeof(cli_addr);

  while (1)
    {

   /**
    *int accept( int sockfd, struct sockaddr *addr, socklen_t *addrlen );
    *sockfd: socket descriptor.
    *addr: estructura sockaddr usada para la comunicacion.
    *addrlen: tamanio de la estructura sockaddr.
    *
    *Devuelve un file descriptor positivo si fue exitoso y -1 en caso contrario.
    */

      newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (newsockfd < 0)
        {
          perror("Error: No se puedo aceptar al cliente");
          exit(1);
        }

      /* Loop para validación de cliente entrante */
      while (1)
        {
          printf("Esperando contraseña\n");
          n = read(newsockfd, pass, sizeof(pass) - 1);
          if (n == -1)
            {
              perror("Error en la recepcion de mensaje");
              exit(1);
            }
          else
            {
              pass[n] = '\0';
              if (n > 0 && pass[n - 1] == '\n')
                pass[--n] = '\0';
              printf("Contraseña recibida %s\n", pass);
            }
          if (validacion(pass) == 1)
            {
              printf("La contraseña ingresada es correcta\n");
              snprintf(buffer, sizeof(buffer), "CORRECTO");
            }
          else
            {
              printf("La contraseña ingresada es incorrecta\n");
              snprintf(buffer, sizeof(buffer), "INCORRECTO");
            }
          n = write(newsockfd, buffer, strlen(buffer));
          if (n == -1)
            {
              perror("Error en el envio del mensaje");
              exit(1);
            }

          if (buffer[0] != 'I')
            break;
        }

      /* Transfiere la comunicacion con el cliente a otro puerto para liberar el
       * puerto 'original' a la espera de nuevas conexiones. */
      pid = fork();
      if (pid < 0)
        {
          perror("Error: problema en el fork");
          exit(1);
        }

      if (pid == 0) /* proceso hijo */
        {
          close(sockfd);

          while (1)
            {
              memset(buffer, '\0', TAM);

              /* Recibe peticion del cliente, la analiza y determina que
               * acción tomar. */
              n = read(newsockfd, buffer, TAM - 1);
              if (n < 0)
                {
                  perror("Error: Problema en la lectura del socket");
                  exit(1);
                }

              buffer[strlen(buffer) - 1] = '\0';

              str = strtok(buffer, " ");
              while (str != NULL)
                {
                  argum[i] = str;
                  i++;
                  str = strtok(NULL, " ");
                }

              if (strcmp("disconnect", argum[0]) == 0)
                {
                  printf(ANSI_COLOR_RED"El cliente %d se ha desconectado", getpid());
                  printf(ANSI_COLOR_RESET"\n");
                  strcpy(buffer, "disconnected");
                  n = write(newsockfd, buffer, TAM);
                  if (n < 0)
                    {
                      perror("escritura en socket");
                      exit(1);
                    }
                  else
                    exit(1);
                }
              else if (strcmp(("listar"), argum[0]) == 0)
                {
                  printf("El cliente %d solicita listar sensores disponibles en las estaciones\n", getpid());
                  listar(buffer);
                  i = 0;
                  n = write(newsockfd, buffer, TAM);
                  if (n < 0)
                    {
                      perror("escritura en socket");
                      exit(1);
                    }
                  continue;
                }
              else if (strcmp(("diario_precipitacion"), argum[0]) == 0)
                {
                  printf("El cliente %d solicita precipitacion diaria en la estacion %s\n", getpid(), argum[1]);
                  precip_diarias(buffer, (int) strtol(argum[1], (char **) NULL, 10), recs, 0);
                  i = 0;
                  n = write(newsockfd, buffer, TAM);
                  if (n < 0)
                    {
                      perror("escritura en socket");
                      exit(1);
                    }
                  continue;
                }
              else if (strcmp(("mensual_precipitacion"), argum[0]) == 0)
                {
                  printf("El cliente %d solicita precipitacion mensual en la estacion %s\n", getpid(), argum[1]);
                  precip_diarias(buffer, (int) strtol(argum[1], (char **) NULL, 10), recs, 1);
                  i = 0;
                  n = write(newsockfd, buffer, TAM);
                  if (n < 0)
                    {
                      perror("escritura en socket");
                      exit(1);
                    }
                  continue;
                }
              else if (strcmp(("promedio"), argum[0]) == 0)
                {
                  printf("El cliente %d solicita el promedio de la variable %s \n", getpid(), argum[1]);
                  getDatos(buffer, argum[1], recs);
                  i = 0;
                  n = write(newsockfd, buffer, TAM);
                  if (n < 0)
                    {
                      perror("escritura en socket");
                      exit(1);
                    }
                  continue;
                }
              else if (strcmp(("descargar"), argum[0]) == 0)
                {
                  printf("El cliente %d solicita descargar los datos de la estacion %s \n", getpid(), argum[1]);
                  strcpy(buffer, "descargar");
                  n = write(newsockfd, buffer, TAM);
                  if (n < 0)
                    {
                      perror("escritura en socket");
                      exit(1);
                    }
                  enviarDatos(puerto, (int) strtol(argum[1], (char **) NULL, 10), recs);
                  i = 0;
                  continue;
                }
              else /* default */
                {
                  printf("El cliente %d ha ingresado un comando desconocido\n", getpid());

                  n = write(newsockfd, "Comando desconocido\n", TAM);
                  if (n < 0)
                    {
                      perror("escritura en socket");
                      exit(1);
                    }
                  continue;
                }
            }
        }
      else
        {
          printf(ANSI_COLOR_GREEN"Nuevo cliente conectado, ID: %d", pid);
          printf(ANSI_COLOR_RESET"\n");
        }
    }
  return 0;
}

/**
 * @brief Establece conexion UDP con el cliente para el envio de datos
 * 
 * @param puerto Puerto del servidor
 * @param num_estacion Numero de estación de la que se solicitan los datos
 * @param recs Cantidad de datos
 * 
 * @return Si la tranferencia se pudo realizar retorno 1, en caso contrario -1.
 */
int
enviarDatos(int puerto, int num_estacion, int recs)
{
  int sockfd, iter = 0;
  char buffC[TAM];
  socklen_t tamano_direccion;
  struct sockaddr_in serv_addr;
  int n = 0;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      printf(" Puerto: %d\n", puerto);
      perror("ERROR en apertura de socket");
      return -1;
    }

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(puerto);
  memset(&(serv_addr.sin_zero), '\0', 8);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("ERROR en binding");
      return -1;
    }

  memset(buffC, 0, TAM);
  tamano_direccion = sizeof(struct sockaddr);

  n = recvfrom(sockfd, buffC, TAM, 0, (struct sockaddr *) &serv_addr, &tamano_direccion);
  if (n < 0)
    {
      perror("lectura de socket");
      return -1;
    }

  while (n >= 0)
    {
      if (!strcmp("OK", buffC))
        {
          memset(buffC, 0, TAM);
          snprintf(buffC, TAM, "Numero      Estacion       Localidad      Fecha   Temperatura\n");
          n = sendto(sockfd, buffC, TAM, 0, (struct sockaddr *) &serv_addr, tamano_direccion);
          for (iter = 0; iter < recs - 1; iter++)
            {
              if (registro[iter]->numero == num_estacion)
                {
                  snprintf(
                      buffC,
                      TAM,
                      "%2d   %12s   %3d   %3s   %-2.1f   %-2.1f   %-2.1f   %-2.1f   %-3.1f   %-20s      %-3.1f   %-3.1f   %-4.1f   %-2.1f   %-2.1f   %-2.1f   %-2.1f   %2.1f   %2.1f   %2.1f\n",
                      registro[iter]->numero, registro[iter]->estacion, registro[iter]->id_localidad,
                      registro[iter]->fecha, registro[iter]->temperatura, registro[iter]->humedad,
                      registro[iter]->punto_rocio, registro[iter]->precipitacion, registro[iter]->velocidad_viento,
                      registro[iter]->direccion_viento, registro[iter]->rafaga_max, registro[iter]->presion,
                      registro[iter]->radiacion_solar, registro[iter]->temp_suelo_1, registro[iter]->temp_suelo_2,
                      registro[iter]->temp_suelo_3, registro[iter]->hum_suelo_1, registro[iter]->hum_suelo_2,
                      registro[iter]->hum_suelo_3, registro[iter]->hum_hoja);
                  n = sendto(sockfd, buffC, 1024, 0, (struct sockaddr *) &serv_addr, tamano_direccion);
                  if (n < 0)
                    {
                      perror("escritura en socket");
                      return -1;
                    }
                  usleep(400);
                  memset(buffC, 0, TAM);
                }
            }
          n = sendto(sockfd, "terminar", 9, 0, (struct sockaddr *) &serv_addr, tamano_direccion);
          close(sockfd);
          n = -1;
        }
    }
  return 1;
}

/**
 * @brief Carga el archivo de usuarios y verifica que el usuario ingresado exista
 * y que la contraseña asociada sea correcta.
 * 
 * @param buffer Usuario y contraseña ingresado por el cliente.
 * 
 * @return Si la validacion es exitosa retorna 1, en caso contrario 0.
 */
int
validacion(char *buffer)
{
  char *usuario, *contras;
  char *token;
  char line[256];
  token = strtok(buffer, ":");
  usuario = token;
  contras = strtok(NULL, " ");
  FILE *fp = fopen("archivos/usuarios.txt", "r");
  if (fp == 0)
    {
      perror("Canot open input file\n");
      exit(-1);
    }
  else
    {
      while (fgets(line, sizeof(line), fp))
        {
          token = strtok(line, ":");
          if (strcmp(token, usuario) == 0)
            {
              token = strtok(NULL, " ");
              if (strcmp(token, contras) == 0)
                {
                  return 1;
                }
            }
        }
    }

  fclose(fp);
  return 0;

}

/**
 * @brief Lee los datos del archivo de la base de datos de las estaciones, parsea la
 * información y lo carga al campo correspondiente en la estructura.
 */
int
constEstruc(void)
{

  int recs = 0;
  struct dato *statsptr = NULL;
  FILE *fptr;
  char *line = NULL;
  char *p = NULL;
  char *start = NULL;
  size_t len = 0;
  size_t read;

  int i = -1;
  if ((fptr = fopen("archivos/datos_meteorologicos.CSV", "r")) == NULL)
    {
      printf("Error en apertura de archivo de entrada \n");
    }
  int valor = 419;
  int seek = fseek(fptr, valor, SEEK_SET);
  if (seek)
    {
      printf("Error en posicionamiento de puntero \n");
    }

  while ((read = getdelim(&line, &len, '\r', fptr)) != -1)
    {

      p = line;
      statsptr = malloc(sizeof(struct dato));

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->numero = atoi(line);
      start = ++p;

      if (i != -1)
        {
          if (statsptr->numero == registro[recs - 1]->numero)
            {
              estaciones[i] = recs;
            }
          else
            i++;
        }
      else
        i = 0;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->estacion = strdup(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->id_localidad = atoi(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->fecha = strdup(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->temperatura = atof(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->humedad = atof(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->punto_rocio = atof(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->precipitacion = atof(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->velocidad_viento = atof(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->direccion_viento = strdup(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->rafaga_max = atof(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->presion = atof(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->radiacion_solar = atof(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->temp_suelo_1 = atof(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->temp_suelo_2 = atof(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->temp_suelo_3 = atof(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->hum_suelo_1 = atof(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->hum_suelo_2 = atof(start);
      start = ++p;

      while (*p != ',' && *p != '\n')
        p++;
      *p = '\0';
      statsptr->hum_suelo_3 = atof(start);
      start = ++p;

      statsptr->hum_hoja = atof(start);

      registro[recs] = statsptr;
      recs++;

    }
  return recs;
}


/**
 * @brief Comprueba en la estructura de datos del servidor aquellas variables que contienen datos para luego
 * listar el sensor correspondiente.
 * 
 * @param buffer Lo datos se concatenan en el buffer para luego ser enviado al cliente.
 */
void
listar(char *buffer)
{
  int i = 0;
  char buffC[TAM];
  float temp, humedad, presip, veloc, presion, rad, temp_s1, temp_s2, temp_s3, hum_s1, hum_s2, hum_s3;

  int numero = estaciones[i];
  memset(buffer, '\0', TAM);
  while (estaciones[i] != 0)
    {

      for (int j = 0; j < numero; j++)
        {
          temp = temp + registro[j]->temperatura;
          humedad = humedad + registro[j]->humedad;
          presip = presip + registro[j]->precipitacion;
          veloc = veloc + registro[j]->velocidad_viento;
          presion = presion + registro[j]->presion;
          rad = rad + registro[j]->radiacion_solar;
          temp_s1 = temp_s1 + registro[j]->temp_suelo_1;
          temp_s2 = temp_s2 + registro[j]->temp_suelo_2;
          temp_s3 = temp_s3 + registro[j]->temp_suelo_3;
          hum_s1 = hum_s1 + registro[j]->hum_suelo_1;
          hum_s2 = hum_s2 + registro[j]->hum_suelo_2;
          hum_s3 = hum_s3 + registro[j]->hum_suelo_3;
        }

      snprintf(buffC, sizeof(buffC), "Estacion N° %d: %s  cuenta con los sensores:\n", registro[estaciones[i]]->numero,
          registro[estaciones[i]]->estacion);
      strcat(buffer, buffC);
      if (temp != 0)
        {
          snprintf(buffC, sizeof(buffC), "%20s%s\n", "", Sensores[1]);
          strcat(buffer, buffC);
        }
      if (humedad != 0)
        {
          snprintf(buffC, sizeof(buffC), "%20s%s\n", "", Sensores[2]);
          strcat(buffer, buffC);
        }
      if (presip != 0)
        {
          snprintf(buffC, sizeof(buffC), "%20s%s\n", "", Sensores[3]);
          strcat(buffer, buffC);
        }
      if (veloc != 0)
        {
          snprintf(buffC, sizeof(buffC), "%20s%s\n", "", Sensores[4]);
          strcat(buffer, buffC);
        }
      if (registro[estaciones[i]]->direccion_viento != 0)
        {
          snprintf(buffC, sizeof(buffC), "%20s%s\n", "", Sensores[5]);
          strcat(buffer, buffC);
        }
      if (presion != 0)
        {
          snprintf(buffC, sizeof(buffC), "%20s%s\n", "", Sensores[6]);
          strcat(buffer, buffC);
        }
      if (rad != 0)
        {
          snprintf(buffC, sizeof(buffC), "%20s%s\n", "", Sensores[7]);
          strcat(buffer, buffC);
        }
      if (temp_s1 != 0)
        {
          snprintf(buffC, sizeof(buffC), "%20s%s\n", "", Sensores[8]);
          strcat(buffer, buffC);
        }
      if (temp_s2 != 0)
        {
          snprintf(buffC, sizeof(buffC), "%20s%s\n", "", Sensores[9]);
          strcat(buffer, buffC);
        }

      temp = humedad = presip = veloc = presion = rad = temp_s1 = temp_s2 = 0;
      i++;
      numero = estaciones[i] - numero;
    }

}


/**
 * @brief Recupera los datos de la estructura acorde con el filtro suministrado
 * 
 * @param buffer Informacion concatenada para ser enviada al cliente
 * @param filtro Filtro de busqueda de informacion en la estructura de datos
 * @param recs Cantidad de datos
 */
void
getDatos(char *buffer, char *filtro, int recs)
{
  char buffC[TAM];
  int i = 0;
  while (0 != strcmp(filtro, Variables[i]))
    {
      if (i > 18)
        {
          snprintf(buffer, TAM, "Variable %s desconocida\n", filtro);
          return;
        }
      i++;
    }

  int iter = 0;
  float suma = 0;
  int aux = 0;
  struct dato *ptr;
  ptr = registro[0];
  for (iter = 0; iter < recs - 1; iter++)
    {
      if (ptr->numero != registro[iter]->numero)
        {
          snprintf(buffC, TAM, "Estacion N° %d: %s\n  Variable: %s\n %3.2f (acumulado) %3.2f (media) \n\n",
              ptr->numero, ptr->estacion, Variables[i], suma, suma / aux);
          strcat(buffer, buffC);
          suma = 0;
          aux = 0;
          ptr = registro[iter];
        }
      aux++;
      if (i == 4)
        suma = suma + registro[iter]->temperatura;
      if (i == 5)
        suma = suma + registro[iter]->humedad;
      if (i == 6)
        suma = suma + registro[iter]->punto_rocio;
      if (i == 7)
        suma = suma + registro[iter]->precipitacion;
      if (i == 8)
        suma = suma + registro[iter]->velocidad_viento;
      if (i == 11)
        suma = suma + registro[iter]->presion;
      if (i == 12)
        suma = suma + registro[iter]->radiacion_solar;
      if (i == 13)
        suma = suma + registro[iter]->temp_suelo_1;
      if (i == 14)
        suma = suma + registro[iter]->temp_suelo_2;
      if (i == 15)
        suma = suma + registro[iter]->temp_suelo_3;
      if (i == 16)
        suma = suma + registro[iter]->hum_suelo_1;
      if (i == 17)
        suma = suma + registro[iter]->hum_suelo_2;
      if (i == 18)
        suma = suma + registro[iter]->hum_suelo_3;
      if (i == 19)
        suma = suma + registro[iter]->hum_hoja;

    }
  snprintf(buffC, TAM, "Estacion N° %d: %s\n  Variable %s:\n %3.2f (acumulado) %3.2f (media) \n\n", ptr->numero,
      ptr->estacion, Variables[i], suma, suma / aux);
  strcat(buffer, buffC);
  suma = 0;
  aux = 0;
}

/**
 * @brief Calcula las precipitaciones diarias de la estacion pasada como parametro.
 * Tambien calcula el aculumado y la media.
 * 
 * @param buffer Informacion concatenada para ser enviada al cliente
 * @param num_estacion Numero de estación de la que se solicitan los datos
 * @param recs Cantidad de datos
 * @param cond Si es 1 imprime el acumulado de la variable
 */
void
precip_diarias(char *buffer, int num_estacion, int recs, int cond)
{

  char buffC[TAM];
  memset(buffer, '\0', TAM);
  float pres = 0, acumulado = 0;
  int inicio = 0, elemento = 0;
  int registros = 0, condicion = 1;
  struct tm *fechaptr = NULL;
  struct tm *fechas[100000];

  char *str;
  for (int i = 0; i < recs - 1; i++)
    {
      if (registro[i]->numero == num_estacion)
        {
          if (condicion == 1)
            {
              inicio = i;
              condicion = 0;
            }
          fechaptr = malloc(sizeof(struct tm));
          str = strtok(registro[i]->fecha, "/");
          if (str != NULL)
            {
              fechaptr->tm_mday = atoi(str);
              str = strtok(NULL, "/");
            }

          if (str != NULL)
            {
              fechaptr->tm_mon = atoi(str);
              str = strtok(NULL, " ");
            }

          if (str != NULL)
            {
              fechaptr->tm_year = atoi(str);
            }

          fechas[registros] = fechaptr;
          registros++;
        }
    }
  pres = registro[inicio]->precipitacion;
  inicio++;

  snprintf(buffC, sizeof(buffC), "Estacion N° %d: %s\n  Fecha[dd/mm/aaaa]     Precipitacion[mm]\n",
      registro[inicio]->numero, registro[inicio]->estacion);
  strcat(buffer, buffC);

  for (int i = 0; i < registros; i++)
    {
      if ((fechas[elemento + 1]->tm_mday == fechas[elemento]->tm_mday) && (fechas[elemento + 1]->tm_mon
          == fechas[elemento]->tm_mon))
        {
          pres = pres + registro[inicio]->precipitacion;
          elemento++;
          inicio++;
        }
      else
        {
          if (fechas[elemento]->tm_mday < 10)
            {
              snprintf(buffC, sizeof(buffC), "     0%d/%d/%d           %5s%-3.2f \n", fechas[elemento]->tm_mday,
                  fechas[elemento]->tm_mon, fechas[elemento]->tm_year, "", pres);
              strcat(buffer, buffC);
            }
          else
            {
              snprintf(buffC, sizeof(buffC), "     %d/%d/%d           %5s%-3.2f \n", fechas[elemento]->tm_mday,
                  fechas[elemento]->tm_mon, fechas[elemento]->tm_year, "", pres);
              strcat(buffer, buffC);
            }
          elemento++;
          acumulado = acumulado + pres;
          pres = 0;
        }
      //Control para evitar segment segmentation fault
      if (i + 2 == registros)
        {
          i++;
        }
    }
  if (cond == 1)
    {
      snprintf(buffC, sizeof(buffC), "Precipitacion acumulada %3.2f(mm)\n", acumulado);
      strcpy(buffer, buffC);
    }
}
