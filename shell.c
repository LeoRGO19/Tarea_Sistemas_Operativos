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
int cantidad_comandos(char **args)
{
    return sizeof(c_list) / sizeof(char *);
}
int (*command_list[])(char **) = {&exec_exit, &exec_help};

int exec_exit(char **args)
{
    exit(0);
}

int exec_help(char **args)
{
    printf("Esta es una shell básica\nLa lista de comandos es:\n");
    for (int i = 0; i < cantidad_comandos(args); i++)
    {
        printf("%s: \n", c_list[i]);
    }
}

char **parsear_entrada()
{
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
            matriz_args[arg_num] = strdup(token); // copiar string
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
        matriz_args[arg_num] = NULL; // terminador
    }

    free(linea);
    return matriz_args;
}

int ejecutar(char **args)
{
    pid_t pid = fork();
    if (pid == 0)
    { // hijo
        if (execvp(args[0], args) == -1)
        {
            perror("execvp");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    { // padre
        int status;
        wait(&status); // esperamos al hijo y guardamos su estado
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

int command_admin(char **args)
{
    for (int i = 0; i < cantidad_comandos(args); i++)
    {
        if (strcmp(args[0], c_list[i]) == 0)
        {
            return (*command_list[i])(args);
        }
    }
    return ejecutar(args);
}
int main()
{
    char **argss;

    while (1)
    {
        printf(">");
        argss = parsear_entrada();
        if (argss[0] == NULL)
        {
            free(argss);
            continue;
        }
        else
        {
            command_admin(argss);

            for (int i = 0; argss[i] != NULL; i++)
            {
                free(argss[i]);
            }
            free(argss);
        }
    }

    return 0;
}
