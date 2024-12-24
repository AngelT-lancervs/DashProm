/* Mejorado: servidor.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>

#define SERVER_PORT 1234
#define MAX_CLIENTS 10
#define THRESHOLD_RAM 90
#define THRESHOLD_CPU 80
#define THRESHOLD_DISK 90
#define THRESHOLD_LOAD 2.0
#define THRESHOLD_USERS 10
#define THRESHOLD_IOWAIT 10

pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count = 0;

void send_notification(const char *message) {
    printf("ALERTA: %s\n", message);
}

void process_metrics(const char *metrics) {
    int ram_used = 0, cpu_used = 0, disk_used = 0, users = 0, iowait = 0;
    float load_avg = 0.0;

    sscanf(metrics, "Mem: %*d %d", &ram_used);
    sscanf(metrics, "CPU: %d", &cpu_used);
    sscanf(metrics, "Disk: %*d %d", &disk_used);
    sscanf(metrics, "Load: %f", &load_avg);
    sscanf(metrics, "Users: %d", &users);
    sscanf(metrics, "IOWait: %d", &iowait);

    if (ram_used > THRESHOLD_RAM) {
        send_notification("Uso de RAM alto.");
    }
    if (cpu_used > THRESHOLD_CPU) {
        send_notification("Uso de CPU alto.");
    }
    if (disk_used > THRESHOLD_DISK) {
        send_notification("Uso de Disco alto.");
    }
    if (load_avg > THRESHOLD_LOAD) {
        send_notification("Promedio de carga alto.");
    }
    if (users > THRESHOLD_USERS) {
        send_notification("Número de usuarios activos alto.");
    }
    if (iowait > THRESHOLD_IOWAIT) {
        send_notification("IO Wait alto.");
    }
}

void *handle_client(void *client_sock_ptr) {
    int client_sock = *(int *)client_sock_ptr;
    free(client_sock_ptr);

    char buffer[1024];
    int len;

    pthread_mutex_lock(&client_count_mutex);
    client_count++;
    pthread_mutex_unlock(&client_count_mutex);

    while (1) {
        if (recv(client_sock, &len, sizeof(len), 0) <= 0) break;
        if (recv(client_sock, buffer, len, 0) <= 0) break;
        buffer[len] = '\0';

        pthread_mutex_lock(&client_count_mutex);
        printf("\033[H\033[J"); // Limpiar la consola globalmente
        printf("Clientes conectados: %d\n\n", client_count);
        pthread_mutex_unlock(&client_count_mutex);

        printf("Dashboard (Cliente %d):\n", client_sock);
        printf("%s\n", buffer);

        process_metrics(buffer); // Procesar métricas y verificar thresholds
    }

    close(client_sock);

    pthread_mutex_lock(&client_count_mutex);
    client_count--;
    pthread_mutex_unlock(&client_count_mutex);

    printf("Cliente desconectado: %d\n", client_sock);
    return NULL;
}

int main() {
    int server_sock, *client_sock;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(client);

    signal(SIGPIPE, SIG_IGN); // Ignorar señales SIGPIPE

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("No se pudo crear el socket");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Error en bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    listen(server_sock, MAX_CLIENTS);
    printf("Esperando conexiones en el puerto %d...\n", SERVER_PORT);

    while ((client_sock = malloc(sizeof(int)), *client_sock = accept(server_sock, (struct sockaddr *)&client, &client_len)) >= 0) {
        pthread_t client_thread;
        printf("Cliente conectado: %d\n", *client_sock);

        if (pthread_create(&client_thread, NULL, handle_client, client_sock) != 0) {
            perror("Error al crear el hilo del cliente");
            free(client_sock);
            continue;
        }

        pthread_detach(client_thread); // Separar el hilo para que no bloquee
    }

    close(server_sock);
    return 0;
}
