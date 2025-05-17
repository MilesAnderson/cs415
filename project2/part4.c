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
static long clk_tck;
static int slice_count = 0;

typedef struct{
    double cpu_sec;
    long rss_kb;
    unsigned long read_bytes, write_bytes;
} proc_metrics_t;

void init_proc_utils(void){
    clk_tck = sysconf(_SC_CLK_TCK);
}

int read_proc_metrics(pid_t pid, proc_metrics_t *m){
    char path[64];
    char buf[1024];
    FILE *f;

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    if(!(f = fopen(path, "r"))){
        return -1;
    }

    long ignored;
    char comm[32];
    char state;
    long utime;
    long stime;
    fscanf(f,
        "%ld %31s %c"
        "%*d %*d %*d %*d %*d"
        "%*u %*u %*u %*u %*u"
        "%ld %ld",
        &ignored, comm, &state,
        &utime, &stime);
    fclose(f);
    m->cpu_sec = (utime + stime) / (double)clk_tck;

    snprintf(path, sizeof(path), "/proc/%d/statm", pid);
    if (!(f = fopen(path, "r"))) return -1;
    long size, resident;
    fscanf(f, "%ld %ld", &size, &resident);
    fclose(f);
    long page_kb = sysconf(_SC_PAGESIZE) / 1024;
    m->rss_kb = resident * page_kb;

    m->read_bytes = m->write_bytes = 0;
    snprintf(path, sizeof(path), "/proc/%d/io", pid);
    if ((f = fopen(path, "r"))) {
        while (fgets(buf, sizeof(buf), f)) {
            unsigned long val;
            if (sscanf(buf, "read_bytes: %lu", &val) == 1) {
                m->read_bytes = val;
            } else if (sscanf(buf, "write_bytes: %lu", &val) == 1) {
                m->write_bytes = val;
            }
        }
        fclose(f);
    }

    return 0;
}

void print_metrics_table(void){
    printf("\n PID    CPU(s)   RSS(KB)   R(bytes)  W(bytes)\n");
    printf("──────────────────────────────────────────────\n");
    for (int i = 0; i < n_children; i++) {
        proc_metrics_t m;
        if (read_proc_metrics(child_pids[i], &m) == 0) {
            printf("%5d  %8.2f  %8ld  %9lu  %9lu\n",
                   child_pids[i],
                   m.cpu_sec,
                   m.rss_kb,
                   m.read_bytes,
                   m.write_bytes);
        }
    }
    printf("\n");
}

void scheduler(int signum){
    if(n_children == 0) return;
    kill(child_pids[current], SIGSTOP);
    current = (current + 1) % n_children;
    kill(child_pids[current], SIGCONT);
    if(++slice_count >= n_children){
        slice_count = 0;
        print_metrics_table();
    }
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
    int status;
    int pid;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        if(WIFEXITED(status)){
            printf("Child %d exited normally\n", pid);
        }
        else if(WIFSIGNALED(status)){
            printf("Child %d killed\n", pid);
        }
        else{
            printf("Child %d ende unexpectedly\n", pid);
        }

        for(int i = 0; i < n_children; i++){
            if(child_pids[i] == pid){
                memmove(&child_pids[i],
                        &child_pids[i+1],
                        (n_children - i - 1) * sizeof(pid_t));
                n_children--;
                if(current >= n_children && n_children > 0){
                    current = current % n_children;
                }
                break;
            }
        }
    }

    if(pid < 0 && errno != ECHILD){
        perror("waitpid");
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

    init_proc_utils();

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
    struct sigaction sa_chld = {0};
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa_chld, NULL) < 0){
        perror("sigaction(SIGCHLD)");
    }

    //setup the timer
    setup_scheduler();

    while (n_children > 0){
        pause();
    }

    setitimer(ITIMER_REAL, &(struct itimerval){0}, NULL);

    printf("All children have terminated; exiting.\n");
    exit(EXIT_SUCCESS);
}