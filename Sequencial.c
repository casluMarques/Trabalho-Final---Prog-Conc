#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/md5.h>
#include<time.h>
#include "timer.h"

#define MAX_PATH 1024
int c = 0;

typedef struct {
    char path[MAX_PATH];
    unsigned char hash[MD5_DIGEST_LENGTH];
} FileInfo;

typedef struct {
    FileInfo *files;
    size_t size;
    size_t capacity;
} FileList;

// Função para inicializar a lista de arquivos
void initFileList(FileList *list, size_t capacity) {
    list->files = malloc(capacity * sizeof(FileInfo));
    list->size = 0;
    list->capacity = capacity;
}

// Função para adicionar um arquivo à lista
void addFile(FileList *list, const char *path) {
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->files = realloc(list->files, list->capacity * sizeof(FileInfo));
    }
    strncpy(list->files[list->size].path, path, MAX_PATH);
    list->size++;
}

// Função para calcular o hash MD5 de um arquivo
void calculateHash(const char *path, unsigned char *output) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("fopen");
        return;
    }

    MD5_CTX md5;
    MD5_Init(&md5);
    const int bufSize = 32768;
    unsigned char *buffer = malloc(bufSize);
    int bytesRead = 0;
    if (!buffer) {
        perror("malloc");
        fclose(file);
        return;
    }

    while ((bytesRead = fread(buffer, 1, bufSize, file))) {
        MD5_Update(&md5, buffer, bytesRead);
    }

    MD5_Final(output, &md5);
    fclose(file);
    free(buffer);
}

// Função para percorrer o diretório e armazenar caminhos de arquivos
void walkDirectory(const char *basePath, FileList *list) {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(basePath)) == NULL) {
        perror("opendir");
        return;
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

        if (S_ISDIR(statbuf.st_mode)) {
            walkDirectory(path, list);
        } else {
            addFile(list, path);
            c++;
        }
    }
    closedir(dir);
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
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    double inicio_main, inicio_leitura, inicio_hash, inicio_exclusao, fim_leitura, fim_hash, fim_exclusao, fim_main;

    GET_TIME(inicio_main);

    FileList fileList;
    initFileList(&fileList, 10);

    GET_TIME(inicio_leitura);
    walkDirectory(argv[1], &fileList);
    GET_TIME(fim_leitura);

    GET_TIME(inicio_hash);
    for (size_t i = 0; i < fileList.size; i++) {
        calculateHash(fileList.files[i].path, fileList.files[i].hash);
    }
    GET_TIME(fim_hash);


    //exibindo resultados
    for (size_t i = 0; i < fileList.size; i++) {
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

    free(fileList.files);

    GET_TIME(fim_main);

    double tempo_main = fim_main - inicio_main;
    double tempo_leitura = fim_leitura - inicio_leitura;
    double tempo_hash = fim_hash - inicio_hash;
    double tempo_exclusao = fim_exclusao - inicio_exclusao;
    
    printf("Tempo de execução total do programa: %f\n", tempo_main);
    printf("Tempo de execução da leitura dos arquivos no diretório: %f\n", tempo_leitura);
    printf("Tempo de execução da função hash: %f\n", tempo_hash);
    printf("Tempo de execução da função de exclusão: %f\n", tempo_exclusao);
    printf("Entrou em %d arquivos\n", c);

    return 0;
}
