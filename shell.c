#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int exec_exit(char **args);
int exec_cd(char **args);
int exec_help(char **args);
char *c_list[] = {"exit", "help"};
int cantidad_comandos(){
    return sizeof(c_list) / sizeof(char *);
}
int (*command_list[])(char **) = {&exec_exit, &exec_help};

int exec_exit(char **args) {
    exit(0);
}

int exec_help(char **args) {
    printf("Esta es una shell básica\nLa lista de comandos es:\n");
    for (int i = 0; i < cantidad_comandos(args); i++)
    {
        printf("%s: \n", c_list[i]);
    }
}

char **parsear_entrada() {
    char *linea = NULL;
    size_t buffersize = 0;

    size_t msize = 64;
    char **matriz_args = malloc(msize * sizeof(char *));
    int arg_num = 0;

    if (!matriz_args)
    {
        fprintf(stderr, "Error al guardar memoria\n");
        exit(EXIT_FAILURE);
    }

    if (getline(&linea, &buffersize, stdin) != -1)
    {
        char *token = strtok(linea, " \n\a\t");
        while (token != NULL)
        {
            matriz_args[arg_num] = strdup(token); 
            if (arg_num >= (int)msize - 1)
            {
                msize += 60;
                matriz_args = realloc(matriz_args, msize * sizeof(char *));
                if (!matriz_args)
                {
                    fprintf(stderr, "Error de memoria\n");
                    exit(EXIT_FAILURE);
                }
            }
            arg_num++;
            token = strtok(NULL, " \n\a\t");
        }
        matriz_args[arg_num] = NULL; 
    }

    free(linea);
    return matriz_args;
}

int ejecutar(char **args) {
    pid_t pid = fork();
    if (pid == 0)
    {
        if (execvp(args[0], args) == -1)
        {
            perror("execvp");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    { 
        int status;
        wait(&status); 
        if (WIFEXITED(status))
        {
            printf("Código de salida: %d\n", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("Hijo terminado por señal: %d\n", WTERMSIG(status));
        }
    }
    else
    {
        perror("fork");
    }
}

int command_admin(char **args) {
    for (int i = 0; i < cantidad_comandos(); i++)
    {
        if (strcmp(args[0], c_list[i]) == 0)
        {
            return (*command_list[i])(args);
        }
    }
    return ejecutar(args);
}


int ejecutar_pipes(char *linea) {
    
    char *comandos[64];
    int n = 0;
    char *token = strtok(linea, "|\n");
    while (token != NULL) {
        comandos[n++] = token;
        token = strtok(NULL, "|\n");
    }

    int pipefd[2*(n-1)]; // necesitamos n-1 pipes
    for (int i = 0; i < n-1; i++) {
        if (pipe(pipefd + i*2) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == 0) { 
            // si no es el primer comando, redirigir entrada
            if (i != 0) {
                dup2(pipefd[(i-1)*2], STDIN_FILENO);
            }
            // si no es el último comando, redirigir salida
            if (i != n-1) {
                dup2(pipefd[i*2 + 1], STDOUT_FILENO);
            }

            // cerrar todos los pipes en el hijo
            for (int j = 0; j < 2*(n-1); j++)
                close(pipefd[j]);

            char *args[64];
            int m = 0;
            char *arg_tok = strtok(comandos[i], " \t\n");
            while (arg_tok != NULL) {
                args[m++] = arg_tok;
                arg_tok = strtok(NULL, " \t\n");
            }
            args[m] = NULL;

            //comando si no es pipe
            command_admin(args); 
            exit(0);
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }
    
    for (int i = 0; i < 2*(n-1); i++)
        close(pipefd[i]);

    for (int i = 0; i < n; i++) //Padre espera a todos los hijos
        wait(NULL);

    return 0;
}

int main() {
    char **argss;

    while (1) {
        printf(">");
        argss = parsear_entrada();
        if (argss[0] == NULL) {
            free(argss);
            continue;
        }

        int hay_pipe = 0;
        for (int i = 0; argss[i] != NULL; i++) {
            if (strchr(argss[i], '|')) {
                hay_pipe = 1;
                break;
            }
        }

        if (hay_pipe) {
            char linea[1024] = "";
            for (int i = 0; argss[i] != NULL; i++) {
                strcat(linea, argss[i]);
                strcat(linea, " ");
            }
            ejecutar_pipes(linea);
        } else {
            command_admin(argss);
        }

        for (int i = 0; argss[i] != NULL; i++)
            free(argss[i]);
        free(argss);
    }

    return 0;
}



