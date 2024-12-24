#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234

void send_metrics_to_server() {
    int sock;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Ejecutar el programa "agente" y redirigir su salida
    FILE *fp = popen("./agente", "r");
    if (!fp) {
        perror("Error ejecutando agente");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char metrics[1024];
    memset(metrics, 0, sizeof(metrics));
    fread(metrics, 1, sizeof(metrics) - 1, fp);
    pclose(fp);

    int len = strlen(metrics);
    send(sock, &len, sizeof(len), 0);
    send(sock, metrics, len, 0);

    close(sock);
}

int main() {
    while (1) {
        send_metrics_to_server();
        sleep(5); // Refrescar cada 5 segundos
    }
    return 0;
}
