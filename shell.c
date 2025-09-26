#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

volatile pid_t hijoCode; 

int exec_exit(char **args);
int exec_cd(char **args);
int miprof(char**args);
int exec_help(char **args);
char *c_list[] = {"exit", "help", "miprof"};
int cantidad_comandos(){
    return sizeof(c_list) / sizeof(char *);
}
int (*command_list[])(char **) = {&exec_exit, &exec_help, &miprof};

int exec_exit(char **args) {
    exit(0);
}

void limpiar_matriz(char **input){
  for (int i = 0; input[i] != NULL; i++) {
    free(input[i]);
  }
  free(input);
}

void cancelar(int signum){
  kill(hijoCode,SIGKILL);
}

int miprof(char **args){
    if (args[1] == NULL) {
        printf("Uso:\n-miprof ejec <comandos y argumentos>\n-miprof ejecsave <Direccion archivo> <comandos y argumentos>\n");
    } else if (strcmp(args[1],"ejec")==0) {
        char **nuevosargs = &args[2];
        struct rusage usage;
	struct timespec inicio, final;

        pid_t procesoHijo_id = fork();
	clock_gettime(CLOCK_MONOTONIC,&inicio);
        if(procesoHijo_id == 0){
          execvp(nuevosargs[0],nuevosargs);
          perror("Ha sucedido un error");
          limpiar_matriz(nuevosargs);
          exit(EXIT_FAILURE);
        }else{
          int estadoHijo;
          if (wait4(procesoHijo_id,&estadoHijo,0,&usage) != -1) {
	    clock_gettime(CLOCK_MONOTONIC,&final);
	    if(WIFEXITED(estadoHijo) && WEXITSTATUS(estadoHijo) == EXIT_FAILURE){
	      return -1;
	    }
	    double tiempoReal = (final.tv_sec - inicio.tv_sec) + (final.tv_nsec - inicio.tv_nsec) / 1000000000.0;
            printf("Tiempos de ejecucion:\n");
            printf("  Tiempo de usuario: %ld segundos, %ld microsegundos\n",
                  usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
            printf("  Tiempo de sistema: %ld segundos, %ld microsegundos\n",
                usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
            printf("  Tiempo real: %f s\n",tiempoReal);
	    printf("Peak de memoria maxima residente: %ld\n", usage.ru_maxrss);
          } else {
              perror("Error al obtener recursos");
          }
	  
        }
	
    }else if(strcmp(args[1],"ejecsave")==0){
        if(args[3]==NULL || args[2]==NULL){
          printf("Error: no se rellenaron los argumentos minimos. (direccion y/o nombre del archivo, comando)\n");
	  return -1;
        }
        char **nuevosargs = &args[3];
        FILE *archivoEscritura = fopen(args[2], "a");

        struct rusage usage;
	struct timespec inicio, final;

        pid_t procesoHijo_id = fork();
	clock_gettime(CLOCK_MONOTONIC,&inicio);
        if(procesoHijo_id == 0){
          execvp(nuevosargs[0],nuevosargs);
          perror("Ha sucedido un error");
          limpiar_matriz(args);
          exit(EXIT_FAILURE);
        }else{
          int estadoHijo;
	  if ((wait4(procesoHijo_id,&estadoHijo,0,&usage) != -1)) {
	    clock_gettime(CLOCK_MONOTONIC,&final);
	    if(WIFEXITED(estadoHijo) && WEXITSTATUS(estadoHijo) == EXIT_FAILURE){
              fclose(archivoEscritura);
	      return -1;
	    }
	    double tiempoReal = (final.tv_sec - inicio.tv_sec) + (final.tv_nsec - inicio.tv_nsec) / 1000000000.0;
	    fprintf(archivoEscritura,"Comando en evaluacion: %s\n", nuevosargs[0]);
	    fprintf(archivoEscritura,"Tiempos de ejecucion:\n");
	    fprintf(archivoEscritura,"  Tiempo de usuario: %ld segundos, %ld microsegundos\n",
                  usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
	    fprintf(archivoEscritura,"  Tiempo de sistema: %ld segundos, %ld microsegundos\n",
                  usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
	    fprintf(archivoEscritura,"  Tiempo en modo kernel: %f s\n\n",tiempoReal);
	    fprintf(archivoEscritura,"Peak de memoria maxima residente: %ld\n", usage.ru_maxrss);
        } else {
            perror("Error al obtener recursos\n");
        }
      }
      fclose(archivoEscritura);
    }else if(strcmp(args[1],"ejecutar")==0){
        char *sobrante;
        long tiempo = strtol(args[2],&sobrante,10);

        struct itimerval tiempoMax;    
        
	if(*sobrante == '\0'){
          printf("No es valido el tiempo ingresado.\n");
	  return -1;
        }

        if (strcmp(sobrante, "s") == 0) {
            tiempoMax.it_value.tv_sec = tiempo;
        } else if (strcmp(sobrante, "us") == 0) {
            tiempoMax.it_value.tv_usec = tiempo;
        } else {
            printf("Tiempo no valido. Debe especificar una unidad: 's' para segundos o 'ms' para milisegundos.\n");
            return -1;
        }


        struct sigaction cancelacion;
	sigemptyset(&cancelacion.sa_mask);
	cancelacion.sa_flags = 0;
	sigaddset(&cancelacion.sa_mask,SIGALRM);
        sigaddset(&cancelacion.sa_mask,SIGCHLD);
	cancelacion.sa_handler = cancelar;
	int señales = sigaction(SIGALRM,&cancelacion,NULL);
	if(señales==-1){
          printf("Error al configurar el comando, intente nuevamente.\n");
	  return -1;
	}
	
        char **nuevosargs = &args[3];
        struct rusage usage;
	struct timespec inicio, final;


        pid_t procesoHijo_id = fork();
	hijoCode = procesoHijo_id;
	clock_gettime(CLOCK_MONOTONIC,&inicio);
	
        if(procesoHijo_id == 0){
          struct sigaction sa_child;
          memset(&sa_child, 0, sizeof(sa_child));
          sa_child.sa_handler = SIG_DFL;
          sigaction(SIGALRM, &sa_child, NULL);
	  
          execvp(nuevosargs[0],nuevosargs);
          perror("Ha sucedido un error\n");
          limpiar_matriz(nuevosargs);
          exit(EXIT_FAILURE);
        }else{
	  if(setitimer(ITIMER_REAL,&tiempoMax,NULL)==-1){
            printf("Error al configurar el temporizador, intente nuevamente.\n");
	    return -1;
	  }
          int estadoHijo;

	  while (wait4(procesoHijo_id, &estadoHijo, 0, &usage) == -1) {
            if (errno != EINTR) {
              perror("Error al esperar al proceso hijo\n");
              return -1;
           }
          }

	  memset(&tiempoMax, 0, sizeof(tiempoMax));
          setitimer(ITIMER_REAL, &tiempoMax, NULL);
          clock_gettime(CLOCK_MONOTONIC, &final);
          hijoCode = -1;
	  
	  clock_gettime(CLOCK_MONOTONIC,&final);
	    
	  if(WIFEXITED(estadoHijo) && WEXITSTATUS(estadoHijo) == EXIT_FAILURE){
	    return -1;
          }else if(WIFSIGNALED(estadoHijo) && WTERMSIG(estadoHijo) == SIGKILL){
	    printf("El proceso no pudo realizarse antes del tiempo propuesto.\n");
            return 0;
	  }
	  double tiempoReal = (final.tv_sec - inicio.tv_sec) + (final.tv_nsec - inicio.tv_nsec) / 1000000000.0;
          printf("Tiempos de ejecucion:\n");
          printf("  Tiempo de usuario: %ld segundos, %ld microsegundos\n",
                usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
          printf("  Tiempo de sistema: %ld segundos, %ld microsegundos\n",
                usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
          printf("  Tiempo real: %f s\n",tiempoReal);
          printf("Peak de memoria maxima residente: %ld\n", usage.ru_maxrss);
          
	  
        }

      
      
    }else {
        printf("Uso: miprof <argumento>\n");
    }
    return 0;
}
int exec_help(char **args) {
    printf("Esta es una shell básica\nLa lista de comandos es:\n");
    for (int i = 0; i < cantidad_comandos(args); i++)
    {
        printf("%s: \n", c_list[i]);
    }
    return 0;
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
        
        char *token = strtok(linea, " \n\t");
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
            token = strtok(NULL, " \n\t");
        }
        matriz_args[arg_num] = NULL; 
    }

    free(linea);
    return matriz_args;
}

int ejecutar(char **args) {
  args[0];
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
        wait(NULL); 
    }
    else
    {
        perror("fork");
    }
    return 0;
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


