#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
// El puerto ahora se pasa como argumento, por lo que SERVER_PORT se elimina

pthread_mutex_t dashboard_mutex = PTHREAD_MUTEX_INITIALIZER;

// Estructura para el dashboard
typedef struct {
    char metrics[BUFFER_SIZE];
    char alerts[BUFFER_SIZE];
} Dashboard;

Dashboard dashboard;

void initialize_dashboard() {
    strcpy(dashboard.metrics, "CPU (%): 0\nRAM (%): 0\nDISK (%): 0\nLOAD: 0.00\nUSERS: 0\nPROCESSES: 0\n");
    strcpy(dashboard.alerts, "Sin alertas\n");
}

void check_thresholds(const char *metrics, char *alerts) {
    int cpu, ram, disk, users, processes;
    float load;
    sscanf(metrics, "CPU: %d\nRAM: %d\nDISK: %d\nLOAD: %f\nUSERS: %d\nPROCESSES: %d\n", &cpu, &ram, &disk, &load, &users, &processes);

    char temp_alerts[BUFFER_SIZE] = "";
    if (cpu > 60) snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA: CPU alta (%d%%)\n", cpu);
    if (ram > 60) snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA: RAM alta (%d%%)\n", ram);
    if (disk > 60) snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA: Disco alto (%d%%)\n", disk);
    if (load > 4.0) snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA: Carga alta (%.2f)\n", load);
    if (users > 10) snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA: Usuarios activos (%d)\n", users);
    if (processes > 300) snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA: Procesos activos (%d)\n", processes);

    if (strlen(temp_alerts) == 0) {
        strcpy(alerts, "Sin alertas\n");
    } else {
        strncpy(alerts, temp_alerts, BUFFER_SIZE);
    }
}

void display_dashboard() {
    printf("\033[H\033[J\033[1;36m================ DASHBOARD ================\033[0m\n");
    pthread_mutex_lock(&dashboard_mutex);

    printf("\033[1;32m%s\033[0m\n", dashboard.metrics); // Verde para métricas
    printf("\033[1;31mAlertas:\n%s\033[0m\n", dashboard.alerts); // Rojo para alertas

    pthread_mutex_unlock(&dashboard_mutex);
    printf("\033[1;36m==========================================\033[0m\n");
}

void *execute_agent(void *arg) {
    int sock = *(int *)arg;
    char buffer[BUFFER_SIZE];
    pid_t pid;
    int pipefd[2];

    // Crear pipe para redirigir la salida del agente
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Proceso hijo
        close(pipefd[0]); // Cerrar lectura
        dup2(pipefd[1], STDOUT_FILENO); // Redirigir stdout al pipe
        execl("./agente", "./agente", NULL); // Ejecutar agente
        perror("execl");
        exit(EXIT_FAILURE);
    } else { // Proceso padre
        close(pipefd[1]); // Cerrar escritura

        while (1) {
            int bytes_read = read(pipefd[0], buffer, BUFFER_SIZE - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';

                pthread_mutex_lock(&dashboard_mutex);
                strncpy(dashboard.metrics, buffer, BUFFER_SIZE);
                check_thresholds(buffer, dashboard.alerts);
                pthread_mutex_unlock(&dashboard_mutex);

                display_dashboard();

                // Enviar métricas al servidor
                send(sock, buffer, bytes_read, 0);
            } else if (bytes_read == 0) {
                break; // Fin del flujo
            } else {
                perror("read");
                break;
            }
        }

        close(pipefd[0]);
        wait(NULL); // Esperar al agente
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <dirección IP del servidor> <puerto>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int sock;
    struct sockaddr_in server_addr;

    // Leer el puerto como argumento
    int port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "El puerto debe ser un número entre 1 y 65535.\n");
        return EXIT_FAILURE;
    }

    // Crear socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear socket");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convertir la dirección IP
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Dirección IP inválida");
        return EXIT_FAILURE;
    }

    // Conectar al servidor
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al conectar al servidor");
        return EXIT_FAILURE;
    }

    initialize_dashboard();

    pthread_t agent_thread;
    if (pthread_create(&agent_thread, NULL, execute_agent, &sock) != 0) {
        perror("Error al crear hilo del agente");
        close(sock);
        return EXIT_FAILURE;
    }

    pthread_join(agent_thread, NULL);

    close(sock);
    return 0;
}