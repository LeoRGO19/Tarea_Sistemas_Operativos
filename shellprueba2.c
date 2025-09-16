#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

char **parsear_entrada() {
    char *linea = NULL;
    size_t buffersize = 0;

    size_t msize = 64;
    char **matriz_args = malloc(msize * sizeof(char *));
    int arg_num = 0;

    if (!matriz_args) {
        fprintf(stderr, "Error al guardar memoria\n");
        exit(EXIT_FAILURE);
    }

    if (getline(&linea, &buffersize, stdin) != -1) {
        char *token = strtok(linea, " \n\a\t");
        while (token != NULL) {
            matriz_args[arg_num] = strdup(token);  // copiar string
            if (arg_num >= (int)msize - 1) {
                msize += 60;
                matriz_args = realloc(matriz_args, msize * sizeof(char *));
                if (!matriz_args) {
                    fprintf(stderr, "Error de memoria\n");
                    exit(EXIT_FAILURE);
                }
            }
            arg_num++;
            token = strtok(NULL, " \n\a\t");
        }
        matriz_args[arg_num] = NULL; // terminador
    }

    free(linea);
    return matriz_args;
}

int main() {
    char **argss;

    while(1){
        printf(">");
        argss = parsear_entrada();
        if(argss[0] == NULL){
            for (int i = 0; argss[i] != NULL; i++) {
                free(argss[i]);
            }
            free(argss);
            continue;
        } else {
            pid_t pid = fork();

            if (pid == 0) { // hijo
                if (execvp(argss[0], argss) == -1) {
                    perror("execvp");
                }
                exit(EXIT_FAILURE);
            } else if (pid > 0) { // padre
                int status;
                wait(&status);  // esperamos al hijo y guardamos su estado

                if (WIFEXITED(status)) {
                    printf("Código de salida: %d\n", WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    printf("Hijo terminado por señal: %d\n", WTERMSIG(status));
                }
            } else {
                perror("fork");
            }
            
            if (strcmp(argss[0], "exit") == 0) {
                for (int i = 0; argss[i] != NULL; i++) {
                    free(argss[i]);
                }
                free(argss);
                exit(0);
            }
            
            for (int i = 0; argss[i] != NULL; i++) {
                free(argss[i]);
            }
            free(argss);
        }
    }

    return 0;
}
