#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define METRIC_COUNT 6

// Definir códigos de color ANSI
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define RESET "\033[0m"

void gather_metrics(char *output) {
    int pipe_fd[2];
    pid_t pid;
    char buffer[128];
    memset(output, 0, 1024);

    if (pipe(pipe_fd) == -1) {
        perror("Pipe failed");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    }

    if (pid == 0) { // Child process
        close(pipe_fd[0]); // Close read end
        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(pipe_fd[1]);
        execlp("sh", "sh", "-c", "free | grep Mem && df / | tail -1 && top -b -n1 | grep '%Cpu' && uptime && who | wc -l && iostat | grep '^avg-cpu' -A 1", NULL);
        perror("execlp failed");
        exit(1);
    } else { // Parent process
        close(pipe_fd[1]); // Close write end
        FILE *fp = fdopen(pipe_fd[0], "r");
        if (!fp) {
            perror("fdopen failed");
            exit(1);
        }
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            strcat(output, buffer);
        }
        fclose(fp);
        wait(NULL); // Wait for child process
    }
}

void display_dashboard(char *metrics) {
    printf("\n%sReal-Time Dashboard (Agent)%s\n", BLUE, RESET);
    printf("%s=====================================%s\n", GREEN, RESET);

    char *line = strtok(metrics, "\n");
    int line_count = 0;

    while (line) {
        if (line_count == 0) {
            printf("%sMemory Usage:%s\n", YELLOW, RESET);
        } else if (line_count == 1) {
            printf("%sDisk Usage:%s\n", YELLOW, RESET);
        } else if (line_count == 2) {
            printf("%sCPU Usage:%s\n", YELLOW, RESET);
        } else if (line_count == 3) {
            printf("%sSystem Load:%s\n", YELLOW, RESET);
        } else if (line_count == 4) {
            printf("%sActive Users:%s\n", YELLOW, RESET);
        } else if (line_count == 5) {
            printf("%sI/O Wait:%s\n", YELLOW, RESET);
        }

        printf("  %s%s%s\n", GREEN, line, RESET);
        line = strtok(NULL, "\n");
        line_count++;
    }

    printf("%s=====================================%s\n", GREEN, RESET);
}

int main() {
    char metrics[1024];
    gather_metrics(metrics);
    display_dashboard(metrics);
    return 0;
}
