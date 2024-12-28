#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define RAM_ALLOCATION_MB 500  // Aumenta a 500 MB por hilo
#define DISK_FILE_SIZE_MB 500  // Aumenta a 500 MB
#define DISK_TEST_FILE "stress_test_file"
#define NUM_CPU_THREADS 8      // Duplica el número de hilos
#define NUM_RAM_THREADS 4      // Más hilos para RAM
#define NUM_DISK_THREADS 2     // Más hilos para disco

void *cpu_stress(void *arg) {
    printf("[CPU Thread] Starting CPU stress test...\n");
    while (1) {
        volatile double result = 1.0;
        for (int i = 0; i < 1000000; i++) { // Incrementa cálculos matemáticos
            result *= 1.000001;
        }
    }
    return NULL;
}

void *ram_stress(void *arg) {
    printf("[RAM Thread] Allocating and writing %d MB to memory...\n", RAM_ALLOCATION_MB);
    size_t size = RAM_ALLOCATION_MB * 1024 * 1024;
    char *memory = malloc(size);
    if (memory == NULL) {
        perror("[RAM Thread] Memory allocation failed");
        pthread_exit(NULL);
    }

    memset(memory, 0, size);
    while (1) {
        for (size_t i = 0; i < size; i += 4096) { // Accede más rápidamente a la memoria
            memory[i] = (char)(i % 256);
        }
    }
    free(memory);
    return NULL;
}

void *disk_stress(void *arg) {
    printf("[Disk Thread] Writing %d MB to file %s...\n", DISK_FILE_SIZE_MB, DISK_TEST_FILE);
    size_t size = DISK_FILE_SIZE_MB * 1024 * 1024;
    char *buffer = malloc(size);
    if (buffer == NULL) {
        perror("[Disk Thread] Buffer allocation failed");
        pthread_exit(NULL);
    }

    memset(buffer, 'A', size);

    while (1) {
        int fd = open(DISK_TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            perror("[Disk Thread] File open failed");
            free(buffer);
            pthread_exit(NULL);
        }

        if (write(fd, buffer, size) < 0) {
            perror("[Disk Thread] Write failed");
            close(fd);
            free(buffer);
            pthread_exit(NULL);
        }
        close(fd);
    }
    free(buffer);
    return NULL;
}

int main() {
    pthread_t cpu_threads[NUM_CPU_THREADS];
    pthread_t ram_threads[NUM_RAM_THREADS];
    pthread_t disk_threads[NUM_DISK_THREADS];

    // Create CPU stress threads
    for (int i = 0; i < NUM_CPU_THREADS; i++) {
        if (pthread_create(&cpu_threads[i], NULL, cpu_stress, NULL) != 0) {
            perror("[Main] Failed to create CPU thread");
            return 1;
        }
    }

    // Create RAM stress threads
    for (int i = 0; i < NUM_RAM_THREADS; i++) {
        if (pthread_create(&ram_threads[i], NULL, ram_stress, NULL) != 0) {
            perror("[Main] Failed to create RAM thread");
            return 1;
        }
    }

    // Create Disk stress threads
    for (int i = 0; i < NUM_DISK_THREADS; i++) {
        if (pthread_create(&disk_threads[i], NULL, disk_stress, NULL) != 0) {
            perror("[Main] Failed to create Disk thread");
            return 1;
        }
    }

    // Join all threads (they run indefinitely)
    for (int i = 0; i < NUM_CPU_THREADS; i++) {
        pthread_join(cpu_threads[i], NULL);
    }
    for (int i = 0; i < NUM_RAM_THREADS; i++) {
        pthread_join(ram_threads[i], NULL);
    }
    for (int i = 0; i < NUM_DISK_THREADS; i++) {
        pthread_join(disk_threads[i], NULL);
    }

    return 0;
}
