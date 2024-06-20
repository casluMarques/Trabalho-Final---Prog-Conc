#include <stdio.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#define MAX_PATH 2032 // usando tamanho máximo de path 254 caracteres, e um byte para cada caractere

// Variável com o número de threads, a ser inicializada por linha de comando
int NUM_THREADS;

// Struct que associa cada Path a um Hash
typedef struct {
    char path[MAX_PATH];
    unsigned char hash[MD5_DIGEST_LENGTH];
} FileInfo;

// Struct para criação dinâmica de array contendo todos os arquivos do diretório
typedef struct {
    FileInfo *files; // array de ponteiros para a estrutura que contém o path do arquivo, e seu hash
    size_t size; // quantidade de arquivos presentes no array 'files'
    size_t capacity; // capacidade máxima atual do array files 
} FileList;

// Estrutura para passar argumentos para as threads
typedef struct {
    FileList *fileList;
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
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->files = realloc(list->files, list->capacity * sizeof(FileInfo));
    }
    // passando para o files.path posição [size], o caminho para o arquivo
    strncpy(list->files[list->size].path, path, MAX_PATH);
    list->size++;
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
        }
    }
    closedir(dir);
}

// Função que será executada por cada thread para calcular o hash
void *hashThread(void *arg) {
    t_Args *threadArgs = (t_Args *)arg;
    FileList *fileList = threadArgs->fileList;
    int threadIndex = threadArgs->threadIndex;
    
    // Cada thread processa um índice específico do vetor de files, baseado no threadIndex
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

    const char *directoryPath = argv[1];
    NUM_THREADS = atoi(argv[2]);

    if (NUM_THREADS <= 0) {
        printf("Número de Threads inválido.\n");
        return 0;
    }

    FileList fileList; // criando array fileList

    // Inicializando a fileList, com 10
    initFileList(&fileList, 10);

    // Andando pelo diretório e salvando os arquivos no array fileList
    walkDirectory(directoryPath, &fileList);

    // Criando threads e inicializando argumentos
    pthread_t *threads = malloc(NUM_THREADS * sizeof(pthread_t));
    t_Args *args = malloc(NUM_THREADS * sizeof(t_Args));

    for(int i = 0; i < NUM_THREADS; i++) { // loop para criação e inicialização das threads
        args[i].fileList = &fileList;
        args[i].threadIndex = i;
        if(pthread_create(&threads[i], NULL, hashThread, &args[i])) {
            printf("Erro na criação da thread %d\n", i);
            return -1;
        }
    }

    // Espera todas as threads terminarem
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL)) {
            printf("--ERRO: pthread_join() \n");
            return -1;
        }
    }

    // Exibindo os resultados
    for (int i = 0; i < fileList.size; i++) {
        printf("File: %s\nHash: ", fileList.files[i].path);
        for (int j = 0; j < MD5_DIGEST_LENGTH; j++) {
            printf("%02x", fileList.files[i].hash[j]);
        }
        printf("\n");
    }

    // Deletando arquivos duplicados
    deleteDuplicates(&fileList);

    // Liberação de memória
    free(threads);
    free(args);
    free(fileList.files);

    return 0;
}
