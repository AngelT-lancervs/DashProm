CC = gcc
CFLAGS = -Wall -O2

all: agente cliente servidor

agente: agente.c
	$(CC) $(CFLAGS) -o agente agente.c

cliente: cliente.c
	$(CC) $(CFLAGS) -o cliente cliente.c

servidor: servidor.c
	$(CC) $(CFLAGS) -o servidor servidor.c -lcurl

clean:
	rm -f agente cliente servidor
