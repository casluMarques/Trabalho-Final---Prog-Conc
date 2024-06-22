#include <stdio.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include<time.h>
#include "timer.h"


#define MAX_PATH 2032 // Usando tamanho máximo de path 2032 caracteres

pthread_mutex_t mutex; // Mutex para sincronização
int NUM_THREADS; // Número de threads, a ser inicializado por linha de comando

// Struct que associa cada Path a um Hash
typedef struct {
    char path[MAX_PATH];
    unsigned char hash[MD5_DIGEST_LENGTH];
} FileInfo;

// Struct para criação dinâmica de array contendo todos os arquivos do diretório
typedef struct {
    FileInfo *files; // Array de ponteiros para a estrutura que contém o path do arquivo e seu hash
    size_t size; // Quantidade de arquivos presentes no array 'files'
    size_t capacity; // Capacidade máxima atual do array 'files'
} FileList;

// Estrutura para passar argumentos para as threads
typedef struct {
    FileList *fileList;
    char path[MAX_PATH];
    int threadIndex;
} t_Args;

// Função para inicializar a lista de arquivos
void initFileList(FileList *list, size_t capacity) {
    list->files = malloc(capacity * sizeof(FileInfo));
    list->size = 0;
    list->capacity = capacity;
}

// Função para adicionar um arquivo à lista
void addFile(FileList *list, const char *path) {
    pthread_mutex_lock(&mutex); // Lock mutex antes de modificar a lista
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->files = realloc(list->files, list->capacity * sizeof(FileInfo));
    }
    strncpy(list->files[list->size].path, path, MAX_PATH);
    list->size++;
    pthread_mutex_unlock(&mutex); // Unlock mutex após modificar a lista
}

// Função para calcular o hash MD5 de um arquivo, recebe o path que deve ler e onde deve escrever o hash
void calculateHash(const char *path, unsigned char *output) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("fopen");
        return;
    }

    MD5_CTX md5;
    MD5_Init(&md5);
    unsigned char *buffer = malloc(32768);
    int bytesRead = 0;
    if (!buffer) {
        perror("malloc");
        fclose(file);
        return;
    }

    while ((bytesRead = fread(buffer, 1, 32768, file))) {
        MD5_Update(&md5, buffer, bytesRead);
    }

    MD5_Final(output, &md5);
    fclose(file);
    free(buffer);
}

// Função para percorrer o diretório e armazenar caminhos de arquivos
void *walkDirectory(void *arg) {
    t_Args *threadArgs = (t_Args *)arg;
    FileList *fileList = threadArgs->fileList;
    char *basePath = threadArgs->path;
    
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(basePath)) == NULL) {
        perror("opendir");
        pthread_exit(NULL);
    }

    while ((entry = readdir(dir)) != NULL) {
        char path[MAX_PATH];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);
        struct stat statbuf;
        if (stat(path, &statbuf) == -1) {
            perror("stat");
            continue;
        }
        
        //Testa se é diretŕio, para criar nova thread
        if (S_ISDIR(statbuf.st_mode)) {
            // Cria uma nova thread para percorrer o subdiretório
            pthread_t thread;
            t_Args *newArgs = malloc(sizeof(t_Args));
            strncpy(newArgs->path, path, MAX_PATH);
            newArgs->fileList = fileList;
            pthread_create(&thread, NULL, walkDirectory, newArgs);
            pthread_detach(thread); // Thread independente, não precisa fazer join
        } else {
            addFile(fileList, path);
        }
    }
    closedir(dir);
    pthread_exit(NULL);
}

// Função que será executada por cada thread para calcular o hash
void *hashThread(void *arg) {
    t_Args *threadArgs = (t_Args *)arg;
    FileList *fileList = threadArgs->fileList;
    int threadIndex = threadArgs->threadIndex;
    
    // Cada thread processa um índice específico do vetor de files
    for (int i = threadIndex; i < fileList->size; i += NUM_THREADS) {
        calculateHash(fileList->files[i].path, fileList->files[i].hash);
    }

    pthread_exit(NULL);
}

// Função para comparar dois hashes
int compareHashes(const unsigned char *hash1, const unsigned char *hash2) {
    return memcmp(hash1, hash2, MD5_DIGEST_LENGTH);
}

// Função para deletar arquivos duplicados com base no hash
void deleteDuplicates(FileList *fileList) {
    for (size_t i = 0; i < fileList->size; i++) {
        for (size_t j = i + 1; j < fileList->size; j++) {
            if (compareHashes(fileList->files[i].hash, fileList->files[j].hash) == 0) {
                if (remove(fileList->files[j].path) == 0) {
                    printf("Deleted duplicate file: %s\n", fileList->files[j].path);
                } else {
                    perror("remove");
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Chamada correta: %s <path> <num_threads>\n", argv[0]);
        return 0;
    }

    double inicio_main,fim_main, inicio_leitura, fim_leitura, inicio_hash, fim_hash, inicio_exclusao, fim_exclusao;
    GET_TIME(inicio_main);

    const char *directoryPath = argv[1];
    NUM_THREADS = atoi(argv[2]);

    if (NUM_THREADS <= 0) {
        printf("Número de Threads inválido.\n");
        return 0;
    }

    FileList fileList;
    initFileList(&fileList, 10);

    pthread_t initialThread;
    t_Args initialArgs;
    strncpy(initialArgs.path, directoryPath, MAX_PATH);
    initialArgs.fileList = &fileList;

    // Inicializando o mutex
    pthread_mutex_init(&mutex, NULL);

    GET_TIME(inicio_leitura);
    // Criando a thread inicial para percorrer o diretório raiz
    if(pthread_create(&initialThread, NULL, walkDirectory, &initialArgs)){
        printf("Erro na criação de thread para percorrer o diretório");
        return -1;
    }

    // Esperando a thread inicial terminar
    pthread_join(initialThread, NULL);
    GET_TIME(fim_leitura);

    pthread_t *hashThreads = malloc(NUM_THREADS * sizeof(pthread_t));
    t_Args *hashArgs = malloc(NUM_THREADS * sizeof(t_Args));


    GET_TIME(inicio_hash);
    // Criando threads para calcular hashes
    for (int i = 0; i < NUM_THREADS; i++) {
        hashArgs[i].fileList = &fileList;
        hashArgs[i].threadIndex = i;
        if (pthread_create(&hashThreads[i], NULL, hashThread, &hashArgs[i])) {
            printf("Erro na criação da thread %d\n", i);
            return -1;
        }
    }

    // Esperando todas as threads de hash terminarem
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(hashThreads[i], NULL)) {
            printf("--ERRO: pthread_join() \n");
            return -1;
        }
    }
    GET_TIME(fim_hash);

    // Exibindo os resultados
    for (int i = 0; i < fileList.size; i++) {
        printf("File: %s\nHash: ", fileList.files[i].path);
        for (int j = 0; j < MD5_DIGEST_LENGTH; j++) {
            printf("%02x", fileList.files[i].hash[j]);
        }
        printf("\n");
    }
    GET_TIME(inicio_exclusao);
    // Deletando arquivos duplicados
    deleteDuplicates(&fileList);
    GET_TIME(fim_exclusao);

    // Liberação de memória
    free(hashThreads);
    free(hashArgs);
    free(fileList.files);

    // Destruição do mutex
    pthread_mutex_destroy(&mutex);

    GET_TIME(fim_main);

    double tempo_main = fim_main - inicio_main;
    double tempo_leitura = fim_leitura - inicio_leitura;
    double tempo_hash = fim_hash - inicio_hash;
    double tempo_exclusao = fim_exclusao - inicio_exclusao;
    
    printf("Tempo de execução total do programa: %f\n", tempo_main);
    printf("Tempo de execução da leitura dos arquivos no diretório: %f\n", tempo_leitura);
    printf("Tempo de execução da função hash: %f\n", tempo_hash);
    printf("Tempo de execução da função de exclusão: %f\n", tempo_exclusao);

    return 0;
}
