.DELETE_ON_ERROR:#Borra el target en caso de producirse un error en alguno de los comandos

#Definimos las macros
CC=gcc	#Compilador a usar
CFLAGS= -std=gnu99 -Werror -Wall -pedantic	#Banderas a utilizar

all: servidor
	@echo "Compilación exitosa | ${shell date --iso=seconds}"

servidor: servidor.c
	${CC} ${CFLAGS} -o servidor servidor.c
	@rm -f servidor.o 

clean:
	@rm -f servidor
	@echo "Se eliminaron correctamente todos los archivos."
