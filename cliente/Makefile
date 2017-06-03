.DELETE_ON_ERROR:#Borra el target en caso de producirse un error en alguno de los comandos

#Definimos las macros
CC=gcc	#Compilador a usar
CFLAGS= -std=gnu99 -Werror -Wall -pedantic	#Banderas a utilizar

all: cliente
	@echo "Compilación exitosa | ${shell date --iso=seconds}"


cliente: cliente.c
	${CC} ${CFLAGS} -o cliente cliente.c
	@rm -f cliente.o

clean:
	@rm -f cliente archivos/datosSalida.CSV 
	@echo "Se eliminaron correctamente todos los archivos."
