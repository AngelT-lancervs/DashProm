/* agente.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

void execute_command(const char *command, char *result) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Proceso hijo
        close(pipefd[0]); // Cerrar lectura
        dup2(pipefd[1], STDOUT_FILENO); // Redirigir salida estándar al pipe
        execlp("sh", "sh", "-c", command, NULL);
        perror("execlp"); // Si execlp falla
        exit(EXIT_FAILURE);
    } else { // Proceso padre
        close(pipefd[1]); // Cerrar escritura
        ssize_t bytes_read = read(pipefd[0], result, BUFFER_SIZE - 1);
        if (bytes_read < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        result[bytes_read] = '\0'; // Asegurar terminación nula
        close(pipefd[0]);
        wait(NULL); // Esperar al hijo
    }
}

void get_metrics(char *buffer) {
    char cpu_usage_output[BUFFER_SIZE];
    char ram_usage_output[BUFFER_SIZE];
    char disk_usage_output[BUFFER_SIZE];
    char load_avg_output[BUFFER_SIZE];
    char users_output[BUFFER_SIZE];
    char processes_output[BUFFER_SIZE];

    int cpu_usage = 0, ram_usage = 0, disk_usage = 0, users = 0, processes = 0;
    float load_avg = 0.0;

    // Obtener uso de CPU
    execute_command("grep 'cpu ' /proc/stat", cpu_usage_output);
    long user, nice, system, idle;
    sscanf(cpu_usage_output, "cpu %ld %ld %ld %ld", &user, &nice, &system, &idle);
    cpu_usage = (int)((user + nice + system) * 100 / (user + nice + system + idle));

    // Obtener uso de RAM
    execute_command("free | grep Mem", ram_usage_output);
    long total, used;
    sscanf(ram_usage_output, "Mem: %ld %ld", &total, &used);
    ram_usage = (int)((used * 100) / total);

    // Obtener uso de disco
    execute_command("df / | tail -1", disk_usage_output);
    int percent;
    sscanf(disk_usage_output, "%*s %*s %*s %*s %d%%", &percent);
    disk_usage = percent;

    // Obtener carga promedio
    execute_command("cat /proc/loadavg", load_avg_output);
    sscanf(load_avg_output, "%f", &load_avg);

    // Obtener número de usuarios conectados
    execute_command("who | wc -l", users_output);
    sscanf(users_output, "%d", &users);

    // Obtener número de procesos activos
    execute_command("ps aux | wc -l", processes_output);
    sscanf(processes_output, "%d", &processes);

    snprintf(buffer, BUFFER_SIZE, "CPU: %d\nRAM: %d\nDISK: %d\nLOAD: %.2f\nUSERS: %d\nPROCESSES: %d\n",
             cpu_usage, ram_usage, disk_usage, load_avg, users, processes);
}

int main() {
    char metrics[BUFFER_SIZE];

    while (1) {
        get_metrics(metrics);
        printf("%s", metrics);
        fflush(stdout);
        sleep(10); // Intervalo de 2 segundos para monitoreo en tiempo real
    }

    return 0;
}