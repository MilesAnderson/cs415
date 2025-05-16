#include<stdio.h>
#include<sys/types.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<string.h>
#include<errno.h>
#include<signal.h>
#include<sys/time.h>

#define MAX_CHILDREN 256

static pid_t child_pids[MAX_CHILDREN];
static int n_children = 0;
static int current = 0;
static int timeslice_sec = 1;

void scheduler(int signum){
    if(n_children > 1){
        printf("Scheduling process %d\n", child_pids[current]);
    }
    kill(child_pids[current], SIGSTOP);
    current = (current + 1) % n_children;
    kill(child_pids[current], SIGCONT);
    struct itimerval tv = {
        .it_interval = { timeslice_sec, 0},
        .it_value = { timeslice_sec, 0}
    };
    if(setitimer(ITIMER_REAL, &tv, NULL) < 0) perror("setitimer");
}

void setup_scheduler(void){
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = scheduler;
    if(sigaction(SIGALRM, &sa, NULL) < 0) perror("sigaction");
    struct itimerval tv = {
        .it_interval = { timeslice_sec, 0 },
        .it_value = { timeslice_sec, 0 }
    };
    if(setitimer(ITIMER_REAL, &tv, NULL) < 0) perror("setitimer");
}

void sigchld_handler(int sig){
    int status, pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        int i;
        for(i = 0; i < n_children; i++){
            if(child_pids[i] == pid) break;
        }
        if( i == n_children){
            continue;
        }

        memmove(&child_pids[i],
                &child_pids[i+1],
                (n_children - i - 1) * sizeof(pid_t));

        n_children--;
        if(current >= n_children){
            current = 0;
        }
        else if(n_children > 0 && i <= current){
            current = (current + n_children - 1) % n_children;
        }
    }
}

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
        }
    }

    fclose(input);

    //Let children run
    for(int i = 0; i < n_children; i++){
        if(kill(child_pids[i], SIGUSR1) < 0){
            perror("kill(SIGUSR1)");
        }
    }

    //Stop all but first child
    for(int i = 1; i < n_children; i++){
        kill(child_pids[i], SIGSTOP);
    }

    //let the first child run
    kill(child_pids[0], SIGCONT);

    //determine if process has terminated
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, NULL) < 0) perror("sigaction");

    //setup the timer
    setup_scheduler();

    //clean up children and remove them from rotation
    int status;
    while(n_children > 0){
        pid_t pid = wait(&status);
        for(int i = 0; i < n_children; i++){
            if(child_pids[i] == pid){
                memmove(&child_pids[i],
                        &child_pids[i+1],
                        (n_children - i - 1) * sizeof(pid_t));
                n_children--;
                if(n_children > 0 && i <= current){
                    current = (current + n_children - 1) % n_children;
                }
                break;
            }
        }
    }

    if(setitimer(ITIMER_REAL, &(struct itimerval){0}, NULL) < 0) perror("setitimer");
    exit(EXIT_SUCCESS);
}