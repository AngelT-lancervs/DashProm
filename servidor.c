/* servidor.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>

#define SERVER_PORT 1234
#define BUFFER_SIZE 1024

pthread_mutex_t dashboard_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int client_id;
    char metrics[BUFFER_SIZE];
    char alerts[BUFFER_SIZE];
    int active;
} Dashboard;

Dashboard *dashboards;
int max_clients;

void initialize_dashboards() {
    for (int i = 0; i < max_clients; i++) {
        dashboards[i].client_id = i + 1;
        strcpy(dashboards[i].metrics, "CPU (%): 0\nRAM (%): 0\nDISK (%): 0\nLOAD: 0.00\nUSERS: 0\nPROCESSES: 0\n");
        strcpy(dashboards[i].alerts, "Sin alertas\n");
        dashboards[i].active = 0;
    }
    printf("Dashboards inicializados con %d espacios inactivos.\n", max_clients);
}

void check_thresholds(const char *metrics, char *alerts) {
    int cpu, ram, disk, users, processes;
    float load;
    sscanf(metrics, "CPU: %d\nRAM: %d\nDISK: %d\nLOAD: %f\nUSERS: %d\nPROCESSES: %d\n", &cpu, &ram, &disk, &load, &users, &processes);

    char temp_alerts[BUFFER_SIZE] = "";
    if (cpu > 80) snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA: CPU alta (%d%%)\n", cpu);
    if (ram > 80) snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA: RAM alta (%d%%)\n", ram);
    if (disk > 80) snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA: Disco alto (%d%%)\n", disk);
    if (load > 2.0) snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA: Carga alta (%.2f)\n", load);
    if (users > 10) snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA: Usuarios activos (%d)\n", users);
    if (processes > 100) snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA: Procesos activos (%d)\n", processes);

    if (strlen(temp_alerts) == 0) {
        strcpy(alerts, "Sin alertas\n");
    } else {
        strncpy(alerts, temp_alerts, BUFFER_SIZE);
    }
}

void display_dashboards() {
    // Limpiar pantalla y mostrar encabezado
    printf("\033[H\033[J\033[1;36m================ DASHBOARDS ================\033[0m\n");
    pthread_mutex_lock(&dashboard_mutex);

    for (int i = 0; i < max_clients; i++) {
        if (dashboards[i].active) {
            // Cliente activo
            printf("\033[1;33mCliente %d:\033[0m\n", dashboards[i].client_id); // Amarillo para cliente
            printf("\033[1;32m%s\033[0m\n", dashboards[i].metrics);            // Verde para métricas
            printf("\033[1;31mAlertas:\n%s\033[0m\n", dashboards[i].alerts);   // Rojo para alertas
        } else {
            // Cliente inactivo
            printf("\033[1;34mDashboard %d: INACTIVO\033[0m\n", dashboards[i].client_id); // Azul para inactivos
        }
        printf("\033[1;36m--------------------------------------------\033[0m\n"); // Línea separadora en cyan
    }

    pthread_mutex_unlock(&dashboard_mutex);
}

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];

    pthread_mutex_lock(&dashboard_mutex);
    int index = -1;
    for (int i = 0; i < max_clients; i++) {
        if (!dashboards[i].active) {
            index = i;
            dashboards[i].active = 1;
            break;
        }
    }
    pthread_mutex_unlock(&dashboard_mutex);

    if (index == -1) {
        printf("No hay espacio para el cliente.\n");
        close(client_sock);
        return NULL;
    }

    while (1) {
        int len = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (len <= 0) break;

        buffer[len] = '\0';

        pthread_mutex_lock(&dashboard_mutex);
        strncpy(dashboards[index].metrics, buffer, BUFFER_SIZE);
        check_thresholds(buffer, dashboards[index].alerts);
        pthread_mutex_unlock(&dashboard_mutex);

        display_dashboards();
    }

    pthread_mutex_lock(&dashboard_mutex);
    dashboards[index].active = 0;
    pthread_mutex_unlock(&dashboard_mutex);

    close(client_sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <número de clientes>\n", argv[0]);
        return EXIT_FAILURE;
    }

    max_clients = atoi(argv[1]);
    if (max_clients <= 0) {
        fprintf(stderr, "El número de clientes debe ser mayor a 0.\n");
        return EXIT_FAILURE;
    }

    dashboards = malloc(sizeof(Dashboard) * max_clients);
    initialize_dashboards();

    int server_sock, *client_sock;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(client);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket");
        return EXIT_FAILURE;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("Bind");
        return EXIT_FAILURE;
    }

    if (listen(server_sock, max_clients) == -1) {
        perror("Listen");
        return EXIT_FAILURE;
    }

    printf("Servidor esperando conexiones en el puerto %d...\n", SERVER_PORT);

    while (1) {
        client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, (struct sockaddr *)&client, &client_len);
        if (*client_sock == -1) {
            perror("Accept");
            free(client_sock);
            continue;
        }

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, client_sock);
        pthread_detach(thread);
    }

    close(server_sock);
    free(dashboards);
    return 0;
}