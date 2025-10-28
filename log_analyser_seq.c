/*
Alexandre Eiji Tomimura 10371680
João Pedro Pioltini de Oliveira 10425643
Luiz Eduardo Bacha dos Santos 10425296
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int parse_status_bytes(char *linha, int *status, long long *bytes);
size_t load_lines(const char *path, char ***out_lines);
void process_lines_seq(char **lines, size_t start, size_t end, long long *erros404, long long *total200);

// Leitura do arquivo inteiro para memória, linha a linha
size_t load_lines(const char *path, char ***out_lines) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror("Error opening file!");   // Falha ao abrir o arquivo
        exit(EXIT_FAILURE);
    }

    size_t cap = 0, n = 0;               // cap = capacidade do vetor; n = linhas já lidas
    char **lines = NULL;                 // vetor de ponteiros para cada linha
    char *buf = NULL;                    // buffer que getline() aloca/realoca
    size_t buflen = 0;                   // tamanho alocado do buffer
    ssize_t r;

    //  getline aloca/expande 'buffer' e coloca a linha completa nele
    while ((r = getline(&buf, &buflen, fp)) != -1) {
        if (n == cap) {
            // Cresce capacidade
            cap = (cap == 0) ? 1024 : cap * 2;
            char **tmp = realloc(lines, cap * sizeof(char*)); //novo tamanho do vetor
            if (!tmp) { perror("realloc"); exit(EXIT_FAILURE); }
            lines = tmp;
        }
        lines[n++] = buf;   // Guarda a linha no vetor
        buf = NULL;         // Força o próximo getline a alocar novo buffer
        buflen = 0;         //zera o tamanho, novo buffer vai ser independente
    }

    free(buf);           
    fclose(fp);
    *out_lines = lines;     // Devolve o vetor preenchido 
    return n;               // Número total de linhas lidas
}

// Extrai status HTTP e bytes enviados a partir de uma linha 
int parse_status_bytes(char *linha, int *status, long long *bytes) {
    char *quote_ptr = strstr(linha, "\" ");
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

void process_lines_seq(char **lines, size_t start, size_t end, long long *erros404, long long *total200) {
    for (size_t k = start; k < end; k++) {
        int status; long long bytes;
        if (parse_status_bytes(lines[k], &status, &bytes)) {
            if (status == 404) (*erros404)++;     // Conta erros 404
            else if (status == 200) (*total200) += bytes; // Soma bytes das respostas 200 OK
        }
    }
}

int main(void){
    struct timespec start,end;  
    const char *path = "/Users/joaopioltini/Documents/Mackenzie/Paralela/Projeto_1/access_log_large.txt";

    // Lê todo o arquivo para memória
    char **lines = NULL;
    size_t n = load_lines(path, &lines);

    long long erros404 = 0;
    long long total200 = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    process_lines_seq(lines, 0, n, &erros404, &total200);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double t_seq = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // Imprime estatísticas e tempo
    printf("Total 404 (seq): %lld\n", erros404);
    printf("Total bytes (200) (seq): %lld\n", total200);
    printf("Tempo de processamento seq (memória): %.4f segundos\n", t_seq);

    // Libera todas as linhas e o vetor
    for (size_t i = 0; i < n; i++) free(lines[i]);
    free(lines);
    return 0;
}