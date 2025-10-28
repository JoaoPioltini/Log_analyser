/*
Alexandre Eiji Tomimura 10371680
João Pedro Pioltini de Oliveira 10425643
Luiz Eduardo Bacha dos Santos 10425296
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    char **lines;
    long start;
    long end;
} ThreadArgs;

typedef struct {
    long long erros404;
    long long total200;
} GlobalStats;

GlobalStats gstats = {0, 0};
pthread_mutex_t gstats_mutex = PTHREAD_MUTEX_INITIALIZER;

//
int parse_status_bytes(char *linha, int *status, long long *bytes);
size_t load_lines(const char *path, char ***out_lines);
void* worker(void *arg);
void process_lines_par(char **lines, size_t n, long long *erros404, long long *total200, int n_threads);

// Leitura do arquivo para memória
size_t load_lines(const char *path, char ***out_lines) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror("Error opening file!");   
        exit(EXIT_FAILURE);
    }

    size_t cap = 0, n = 0;               // capacidade do vetor e linhas lidas
    char **lines = NULL;                 
    char *buf = NULL;                
    size_t buflen = 0;                  
    ssize_t r;

    // Lê o arquivo todo
    while ((r = getline(&buf, &buflen, fp)) != -1) {
        if (n == cap) {
            // Crescimento do vetor
            cap = cap ? cap * 2 : 1024;
            char **tmp = realloc(lines, cap * sizeof(char*));
            if (!tmp) { perror("realloc"); exit(EXIT_FAILURE); }
            lines = tmp;
        }
        lines[n++] = buf;   // transfere do buffer para o vetor
        buf = NULL;         
        buflen = 0;
    }

    free(buf);            
    fclose(fp);
    *out_lines = lines;     // entrega o vetor preenchido
    return n;               
}


int parse_status_bytes(char *linha, int *status, long long *bytes) {
   
    char *quote_ptr = strstr(linha, "\" "); // \ representa aspas duplas
    if (!quote_ptr){
        return 0; 
    }

    int status_code;
    long long bytes_sent;

    if (sscanf(quote_ptr + 2, "%d %lld", &status_code, &bytes_sent) == 2) {
        *status = status_code;
        *bytes = bytes_sent;
        return 1; 
    }
    return 0;     
}

// Acumula localmente
void* worker(void *arg) {
    ThreadArgs *a = (ThreadArgs*)arg;

    long long local404 = 0; // contador local de 404
    long long local200 = 0; // somatório local de bytes (status 200)

    for (long k = a->start; k < a->end; k++) {
        int status; long long bytes;
        if (parse_status_bytes(a->lines[k], &status, &bytes)) {
            if (status == 404) {
                local404++;
            } else if (status == 200) {
                local200 += bytes;
            }
        }
    }

    pthread_mutex_lock(&gstats_mutex);
    gstats.erros404 += local404;
    gstats.total200 += local200;
    pthread_mutex_unlock(&gstats_mutex);

    free(a); 
    return NULL;
}

// Divide o vetor de linhas em T fatias para cada thread
// Cada thread processa sua fatia e salva na global via mutex.
void process_lines_par(char **lines, size_t n, long long *erros404, long long *total200, int n_threads) {
    if (n_threads <= 0) {
        fprintf(stderr, "Número de threads deve ser > 0\n");
        return;
    }
    if ((size_t)n_threads > n && n > 0) {
        n_threads = (int)n; 
    }

    pthread_t threads[n_threads];

    long long chunk = (long long)(n / (size_t)n_threads);
    size_t rest = n % (size_t)n_threads;
    size_t start = 0;

    // Zera as estatísticas globais antes de iniciar
    gstats.erros404 = 0;
    gstats.total200 = 0;

    // Criação das threads e distribuição das fatias
    for (size_t i = 0; i < (size_t)n_threads; i++) {
        ThreadArgs *args = malloc(sizeof(ThreadArgs));     
        size_t size = (size_t)chunk + (i < rest ? 1 : 0);  
        args->lines = lines;
        args->start = (long)start;
        args->end   = (long)(start + size);
        start += size;

        pthread_create(&threads[i], NULL, worker, args);
    }

    for (size_t i = 0; i < (size_t)n_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    *erros404 = gstats.erros404;
    *total200 = gstats.total200;
}


int main(int argc, char *argv[]) {
    struct timespec start, end;
    const char *path = "/Users/joaopioltini/Documents/Mackenzie/Paralela/Projeto_1/access_log_large.txt";

    char **lines = NULL;
    size_t n = load_lines(path, &lines);

    long long erros404 = 0;
    long long total200 = 0;

    // Número de threads: padrão 4
    int n_threads = 4;
    if (argc >= 2) {
        n_threads = atoi(argv[1]);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    process_lines_par(lines, n, &erros404, &total200, n_threads);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double t_par = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Total 404 (par): %lld\n", erros404);
    printf("Total bytes (200) (par): %lld\n", total200);
    printf("Tempo de processamento par (memória): %.4f segundos\n", t_par);

    for (size_t i = 0; i < n; i++) free(lines[i]);
    pthread_mutex_destroy(&gstats_mutex);
    free(lines);
    return 0;
}