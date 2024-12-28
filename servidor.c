#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <curl/curl.h>

#define SERVER_PORT 1234
#define BUFFER_SIZE 1024

pthread_mutex_t dashboard_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int client_id;
    char metrics[BUFFER_SIZE];
    char alerts[BUFFER_SIZE];
    int active;

    // Flags para rastrear si ya se envió una alerta por métrica
    int cpu_alert_sent;
    int ram_alert_sent;
    int disk_alert_sent;
    int load_alert_sent;
    int users_alert_sent;
    int processes_alert_sent;
} Dashboard;

Dashboard *dashboards;
int max_clients;

void initialize_dashboards() {
    for (int i = 0; i < max_clients; i++) {
        dashboards[i].client_id = i + 1;
        strcpy(dashboards[i].metrics, "CPU (%): 0\nRAM (%): 0\nDISK (%): 0\nLOAD: 0.00\nUSERS: 0\nPROCESSES: 0\n");
        strcpy(dashboards[i].alerts, "Sin alertas\n");
        dashboards[i].active = 0;

        // Inicializar flags de alerta
        dashboards[i].cpu_alert_sent = 0;
        dashboards[i].ram_alert_sent = 0;
        dashboards[i].disk_alert_sent = 0;
        dashboards[i].load_alert_sent = 0;
        dashboards[i].users_alert_sent = 0;
        dashboards[i].processes_alert_sent = 0;
    }
    printf("Dashboards inicializados con %d espacios inactivos.\n", max_clients);
}

size_t no_op_callback(void *ptr, size_t size, size_t nmemb, void *data) {
    return size * nmemb; // No hacer nada con la respuesta.
}

void enviar_notificación_whatsapp(const char *message) {
    CURL *curl;
    CURLcode res;

    const char *account_sid = "ACa4f0e37aee9ff628103ba8a1170a62c8";
    const char *auth_token = "529a8918d5060c666bedef8c3a872275";
    const char *twilio_whatsapp_number = "+14155238886";
    const char *recipient_whatsapp_number = "+593980740851";

    // Inicializar libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        char *encoded_from = curl_easy_escape(curl, twilio_whatsapp_number, 0);
        char *encoded_to = curl_easy_escape(curl, recipient_whatsapp_number, 0);
        char url[256];
        snprintf(url, sizeof(url), "https://api.twilio.com/2010-04-01/Accounts/%s/Messages.json", account_sid);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // Construir el cuerpo del mensaje
        char postfields[2048];
        snprintf(postfields, sizeof(postfields), "From=whatsapp:%s&To=whatsapp:%s&Body=%s", encoded_from, encoded_to, message);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);

        char userpwd[512];
        snprintf(userpwd, sizeof(userpwd), "%s:%s", account_sid, auth_token);
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, no_op_callback);
        printf("%s\n", message);
        // Realizar la solicitud
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        // Limpiar
        curl_easy_cleanup(curl);
        curl_free(encoded_from);
        curl_free(encoded_to);
    } else {
        fprintf(stderr, "curl_easy_init() failed\n");
    }
    curl_global_cleanup();
}
 
void check_thresholds(const char *metrics, char *alerts, int client_id) {
    int cpu, ram, disk, users, processes;
    float load;
    sscanf(metrics, "CPU: %d\nRAM: %d\nDISK: %d\nLOAD: %f\nUSERS: %d\nPROCESSES: %d\n", &cpu, &ram, &disk, &load, &users, &processes);

    char temp_alerts[BUFFER_SIZE] = "";
    Dashboard *dashboard = &dashboards[client_id - 1];

    // Evaluar y manejar cada métrica
    if (cpu > 70) {
        if (!dashboard->cpu_alert_sent) { // Solo enviar alerta si no se ha enviado antes
            snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA Cliente %d: CPU alta (%d%%)\n", client_id, cpu);
            dashboard->cpu_alert_sent = 1; // Marcar como enviada
            enviar_notificación_whatsapp(temp_alerts);
        }
    } else {
        dashboard->cpu_alert_sent = 0; // Resetear flag si la métrica vuelve al nivel seguro
    }

    if (ram > 70) {
        if (!dashboard->ram_alert_sent) {
            snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA Cliente %d: RAM alta (%d%%)\n", client_id, ram);
            dashboard->ram_alert_sent = 1;
            enviar_notificación_whatsapp(temp_alerts);
        }
    } else {
        dashboard->ram_alert_sent = 0;
    }

    if (disk > 70) {
        if (!dashboard->disk_alert_sent) {
            snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA Cliente %d: Disco alto (%d%%)\n", client_id, disk);
            dashboard->disk_alert_sent = 1;
            enviar_notificación_whatsapp(temp_alerts);
        }
    } else {
        dashboard->disk_alert_sent = 0;
    }

    if (load > 5.0) {
        if (!dashboard->load_alert_sent) {
            snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA Cliente %d: Carga alta (%.2f)\n", client_id, load);
            dashboard->load_alert_sent = 1;
            enviar_notificación_whatsapp(temp_alerts);
        }
    } else {
        dashboard->load_alert_sent = 0;
    }

    if (users > 5) {
        if (!dashboard->users_alert_sent) {
            snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA Cliente %d: Usuarios activos (%d)\n", client_id, users);
            dashboard->users_alert_sent = 1;
            enviar_notificación_whatsapp(temp_alerts);
        }
    } else {
        dashboard->users_alert_sent = 0;
    }

    if (processes > 300) {
        if (!dashboard->processes_alert_sent) {
            snprintf(temp_alerts + strlen(temp_alerts), BUFFER_SIZE - strlen(temp_alerts), "ALERTA Cliente %d: Procesos activos (%d)\n", client_id, processes);
            dashboard->processes_alert_sent = 1;
            enviar_notificación_whatsapp(temp_alerts);
        }
    } else {
        dashboard->processes_alert_sent = 0;
    }

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
        check_thresholds(buffer, dashboards[index].alerts, dashboards[index].client_id);
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