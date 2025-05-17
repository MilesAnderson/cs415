#include<stdio.h>
#include<sys/types.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<string.h>
#include<errno.h>
#include<signal.h>

int main(int argc, char *argv[]){
    //Usage
    if(argc != 3){
        printf("Wrong number of arguments\n");
        exit(0);
    }
    else if(strcmp(argv[1], "-f") != 0){
        printf("No file inputed\n");
        exit(0);
    }
    
    //Opening file
    FILE* input;
    input = fopen(argv[2], "r");
    if(input == NULL){
        printf("The input file failed to open.\n");
        return 1;
    }

    //For waiting
    int n_children = 0;
    pid_t child_pids[256];

    //For each command
    char line[1024];
    while(fgets(line, sizeof(line), input) != NULL){

        line[strcspn(line, "\n")] = '\0';

        //Create the input for execvp
        char* cmd_argv[64];
        int i = 0;
        char* token = strtok(line, " \t");
        while(token != NULL && i < (int)(sizeof(cmd_argv)/sizeof(cmd_argv[0]))-1){
            cmd_argv[i++] = token;
            token = strtok(NULL, " \t");
        }
        cmd_argv[i] = NULL;

        //Launch and run separate process
        pid_t pid = fork();
        if(pid < 0){
            perror("fork");
            continue;
        }
        else if(pid == 0){
            //Block SIGUSR1
            sigset_t mask;
            sigemptyset(&mask);
            sigaddset(&mask, SIGUSR1);
            if(sigprocmask(SIG_BLOCK, &mask, NULL) < 0){
                perror("sigprocmask");
                _exit(EXIT_FAILURE);
            }

            //Wait for parent to sent SIGUSR1
            int sig;
            if(sigwait(&mask, &sig) != 0){
                perror("sigwait");
                _exit(EXIT_FAILURE);
            }

            //Unblock SIGUSR1
            sigemptyset(&mask);
            if(sigprocmask(SIG_SETMASK, &mask, NULL) < 0){
                perror("sigprocmask");
                _exit(EXIT_FAILURE);
            }

            //exec
            execvp(cmd_argv[0], cmd_argv);
            perror("execvp");
            _exit(EXIT_FAILURE);
        }
        else{
            child_pids[n_children++] = pid;
            printf("Forked child %d to run: %s\n", pid, cmd_argv[0]);
        }
    }

    fclose(input);

    //Let children run
    for(int i = 0; i < n_children; i++){
        printf("pid %d: SIGUSR1\n", child_pids[i]);
        if(kill(child_pids[i], SIGUSR1) < 0){
            perror("kill(SIGUSR1)");
        }
    }

    //Stop the children
    for(int i = 0; i < n_children; i++){
        printf("pid %d: SIGSTOP\n", child_pids[i]);
        if(kill(child_pids[i], SIGSTOP) < 0){
            perror("kill(SIGSTOP)");
        }
    }

    sleep(1);

    //Wake the children
    for(int i = 0; i < n_children; i++){
        printf("pid %d: SIGCONT\n", child_pids[i]);
        if(kill(child_pids[i], SIGCONT) < 0){
            perror("kill(SIGCONT)");
        }
    }

    //Wait for processes
    for(int j = 0; j < n_children; j++){
        int status;
        pid_t pid = waitpid(child_pids[j], &status, 0);
        if(pid < 0){
            perror("waitpid");
        }
        printf("Child %d terminated (status %d)\n", pid, WEXITSTATUS(status));
    }

    exit(EXIT_SUCCESS);
}