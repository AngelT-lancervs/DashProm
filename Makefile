CC = gcc
CFLAGS = -Wall -O2

all: agente cliente servidor prueba_stress

agente: agente.c
	$(CC) $(CFLAGS) -o agente agente.c

cliente: cliente.c
	$(CC) $(CFLAGS) -o cliente cliente.c

servidor: servidor.c
	$(CC) $(CFLAGS) -o servidor servidor.c -lcurl

stress_test: stress_test.c
	$(CC) $(CFLAGS) -pthread -o prueba_stress prueba_stress.c

clean:
	rm -f agente cliente servidor prueba_stress