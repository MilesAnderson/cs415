#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "thread_queue.h"

#define SNAPSHOT_MAX_LEN 512

#define MAX_RIDES 3

#define MAX_CARS 100
#define MAX_ATTENDANTS 100

typedef struct{
    int elapsed_time;
    int ticket_queue_len;
    int ride_queue_len;

    int num_cars;
    int car_states[MAX_CARS];
    int car_load_counts[MAX_CARS];

    int num_attendants;
    int attendant_states[MAX_ATTENDANTS];
} snapshot_t;

int monitor_pipe_fd[2];
pid_t monitor_pid = -1;

pthread_mutex_t car_state_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t attendant_state_mutex = PTHREAD_MUTEX_INITIALIZER;
int car_current_state[MAX_CARS];
int car_current_load[MAX_CARS];
int attendant_current_state[MAX_ATTENDANTS];

int g_num_cars;
int g_num_attendants;

thread_queue_t ticket_line;
thread_queue_t ride_line;

void monitor_loop(void);
void parent_sampler_loop(void);
void* car(void* arg);
void* attendant(void* arg);
static int elapsed_seconds(void);

int n = 1;
int c = 1;
int p = 1;
int w = 10;
int r = 8;

static int simulation_done = 0;

pthread_mutex_t ticket_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ride_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t *board_mutex;
pthread_mutex_t *unboard_mutex;
pthread_cond_t ride_arrived_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t board_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t board_ack_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t unboard_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t *board_cv;
pthread_cond_t *unboard_cv;
pthread_cond_t *board_ack_cv;
int *board_flag;
int *board_ack;
int *unboard_flag;

static struct timespec start_time;

int main(int argc, char *argv[]){
    if(clock_gettime(CLOCK_MONOTONIC, &start_time) < 0){
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    srand((unsigned)time(NULL));

    int opt;
    while((opt = getopt(argc, argv, ":n:c:p:w:r:h")) != -1){
        switch(opt){
            case 'n':
                n = atoi(optarg);
                break;
            case 'c':
                c = atoi(optarg);
                break;
            case 'p':
                p = atoi(optarg);
                break;
            case 'w':
                w = atoi(optarg);
                break;
            case 'r':
                r = atoi(optarg);
                break;
            case 'h':
                printf("To set your own paramaters run with any of the following flags:\n");
                printf("-n <int>    number of passenger threads\n");
                printf("-c <int>    number of cars\n");
                printf("-p <int>    capacity per car\n");
                printf("-w <int>    car waiting period in seconds\n");
                printf("-r <int>    ride duration in seconds\n");
                printf("-h          help board\n");
                break;
            case ':':
                printf("option needs a value\n");
                break;
            case '?':
                printf("unknown option %c\n", optopt);
                break;
        }
    }
    for(; optind < argc; optind++){
        printf("Given extra arguments: %s\n", argv[optind]);
    }

    g_num_attendants = n;
    g_num_cars = c;

    printf("===== DUCK PARK SIMULATION =====\n");
    printf("[Monitor] Simulation started with parameters:\n");
    printf("- Number of passenger threads: %d\n", n);
    printf("- Number of cars: %d\n", c);
    printf("- Capacity per car: %d\n", p);
    printf("- Park exploration time: 3 seconds\n");
    printf("- Car waiting period: %d seconds\n", w);
    printf("- Ride duration: %d seconds\n", r);
    printf("- Simulation Duration: %d rides\n", MAX_RIDES);
    printf("\n");

    if(pipe(monitor_pipe_fd) < 0){
        perror("pipe");
        exit(1);
    }

    monitor_pid = fork();
    if(monitor_pid < 0){
        perror("fork");
        exit(2);
    }
    if(monitor_pid == 0){
        close(monitor_pipe_fd[1]);
        monitor_loop();
        close(monitor_pipe_fd[0]);
        exit(0);
    }

    close(monitor_pipe_fd[0]);

    tq_init(&ticket_line);
    tq_init(&ride_line);

    pthread_mutex_lock(&car_state_mutex);
    for(int i = 0; i < g_num_cars; i++){
        car_current_state[i] = 0;
        car_current_load[i] = 0;
    }
    pthread_mutex_unlock(&car_state_mutex);

    pthread_mutex_lock(&attendant_state_mutex);
    for(int i = 0; i < g_num_attendants; i++){
        attendant_current_state[i] = 0;
    }
    pthread_mutex_unlock(&attendant_state_mutex);

    board_flag = malloc(sizeof(int) * n);
    board_ack = malloc(sizeof(int) * n);
    unboard_flag = malloc (sizeof(int) * n);

    board_mutex = malloc(sizeof(pthread_mutex_t) * n);
    board_cv = malloc(sizeof(pthread_cond_t) * n);
    unboard_mutex = malloc(sizeof(pthread_mutex_t) * n);
    unboard_cv = malloc(sizeof(pthread_cond_t) * n);
    board_ack_cv = malloc(sizeof(pthread_cond_t) * n);

    for(int i = 0; i < n; i++){
        board_flag[i] = 0;
        board_ack[i] = 0;
        unboard_flag[i] = 0;

        pthread_mutex_init(&board_mutex[i], NULL);
        pthread_cond_init(&board_cv[i], NULL);
        pthread_mutex_init(&unboard_mutex[i], NULL);
        pthread_cond_init(&unboard_cv[i], NULL);
        pthread_cond_init(&board_ack_cv[i], NULL);
    }

    pthread_t attendants[n];
    for(int i = 0; i < n; i++){
        int *arg = malloc(sizeof(int));
        *arg = i;
        pthread_create(&attendants[i], NULL, attendant, arg);
    }

    pthread_t cars[c];
    for(int i = 0; i < c; i++){
        int *arg = malloc(sizeof(int));
        *arg = i;
        pthread_create(&cars[i], NULL, car, arg);
    }

    pthread_t sampler_thread;
    pthread_create(&sampler_thread, NULL, (void*(*)(void *))parent_sampler_loop, NULL);

    for(int i = 0; i < c; i++){
        pthread_join(cars[i], NULL);
    }
    tq_close(&ride_line);

    simulation_done = 1;
    for(int pid = 0; pid < n; pid++){
        pthread_mutex_lock(&board_mutex[pid]);
        pthread_cond_signal(&board_cv[pid]);
        pthread_mutex_unlock(&board_mutex[pid]);

        pthread_mutex_lock(&unboard_mutex[pid]);
        pthread_cond_signal(&unboard_cv[pid]);
        pthread_mutex_unlock(&unboard_mutex[pid]);
    }
    for(int i = 0; i < n; i++){
        pthread_join(attendants[i], NULL);
    }

    pthread_join(sampler_thread, NULL);

    close(monitor_pipe_fd[1]);

    int status;
    waitpid(monitor_pid, &status, 0);

    pthread_mutex_destroy(&ticket_mutex);
    pthread_mutex_destroy(&ride_mutex);
    pthread_mutex_destroy(&car_state_mutex);
    pthread_mutex_destroy(&attendant_state_mutex);
    tq_destroy(&ticket_line);
    tq_destroy(&ride_line);

    for(int i = 0; i < n; i++){
        pthread_mutex_destroy(&board_mutex[i]);
        pthread_cond_destroy(&board_cv[i]);
        pthread_mutex_destroy(&unboard_mutex[i]);
        pthread_cond_destroy(&unboard_cv[i]);
        pthread_cond_destroy(&board_ack_cv[i]);
    }
    free(board_flag);
    free(board_ack);
    free(unboard_flag);
    free(board_mutex);
    free(board_cv);
    free(unboard_mutex);
    free(unboard_cv);
    free(board_ack_cv);

    return 0;
}

static int elapsed_seconds(void){
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    time_t sec_diff = now.tv_sec - start_time.tv_sec;
    long nsec_diff = now.tv_nsec - start_time.tv_nsec;
    return (int)sec_diff + (int)(nsec_diff / 1e9);

}

void* attendant(void* arg){
    int pid = *(int*)arg;
    free(arg);

    for(int i = 0; i < MAX_RIDES; i++){
        //Explore
        printf("[Time: %.3d s] Attendant %d is exploring the park.\n", elapsed_seconds(), pid);
        int explore_time = (rand() % 4) + 2;
        sleep(explore_time);
        //Monitor Update (attendant exploring)
        pthread_mutex_lock(&attendant_state_mutex);
        attendant_current_state[pid] = 0;
        pthread_mutex_unlock(&attendant_state_mutex);
        //====================================

        //Get Ticket
        pthread_mutex_lock(&ticket_mutex);
        printf("[Time: %.3d s] Attendant %d waiting in ticket queue.\n", elapsed_seconds(), pid);
        tq_enqueue(&ticket_line, pid);
        //Monitor Update (waiting in ticket line)
        pthread_mutex_lock(&attendant_state_mutex);
        attendant_current_state[pid] = 1;
        pthread_mutex_unlock(&attendant_state_mutex);
        //=======================================
        int my_ticket = tq_dequeue(&ticket_line);
        printf("[Time: %.3d s] Attendant %d acquired ticket %d.\n", elapsed_seconds(), pid, my_ticket);
        pthread_mutex_unlock(&ticket_mutex);

        //Board
        printf("[Time: %.3d s] Attendant %d joined the ride queue.\n", elapsed_seconds(), pid);
        tq_enqueue(&ride_line, pid);
        //Monitor Update (waiting in ride line)
        pthread_mutex_lock(&attendant_state_mutex);
        attendant_current_state[pid] = 2;
        pthread_mutex_unlock(&attendant_state_mutex);
        //=====================================
        pthread_mutex_lock(&board_mutex[pid]);
        while(!board_flag[pid] && !simulation_done){
            pthread_cond_wait(&board_cv[pid], &board_mutex[pid]);
        }
        if(!board_flag[pid] && simulation_done){
            pthread_mutex_unlock(&board_mutex[pid]);
            return NULL;
        }
        printf("[Time: %.3d s] Attendant %d boarded.\n", elapsed_seconds(), pid);
        board_flag[pid] = 0;
        board_ack[pid] = 1;
        //Monitor Update (boarded ride)
        pthread_mutex_lock(&attendant_state_mutex);
        attendant_current_state[pid] = 3;
        pthread_mutex_unlock(&attendant_state_mutex);
        //============================
        pthread_cond_signal(&board_ack_cv[pid]);
        pthread_mutex_unlock(&board_mutex[pid]);

        //Unboard
        pthread_mutex_lock(&unboard_mutex[pid]);
        while(!unboard_flag[pid]){
            pthread_cond_wait(&unboard_cv[pid], &unboard_mutex[pid]);
        }
        printf("[Time: %.3d s] Attendant %d unboarded.\n", elapsed_seconds(), pid);
        unboard_flag[pid] = 0;
        pthread_mutex_unlock(&unboard_mutex[pid]);
    }
    return NULL;
}

void* car(void* arg){
    int car_id = *(int*)arg;
    free(arg);

    for(int i = 0; i < MAX_RIDES; i++){
        pthread_mutex_lock(&ride_mutex);

        //Monitor Update(car dequeueing ride line)
        pthread_mutex_lock(&car_state_mutex);
        car_current_state[car_id] = 1;
        car_current_load[car_id] = 0;
        pthread_mutex_unlock(&car_state_mutex);
        //=======================================
        int first_pid = tq_dequeue(&ride_line);
        if(first_pid < 0){
            pthread_mutex_unlock(&ride_mutex);
            break;
        }

        struct timespec load_start_rt;
        clock_gettime(CLOCK_REALTIME, &load_start_rt);

        int riders_collected = 1;
        int pids[p];
        pids[0] = first_pid;

        printf("[Time: %.3d s] Car %d invoked load().\n", elapsed_seconds(), car_id);

        pthread_mutex_lock(&board_mutex[first_pid]);
        board_flag[first_pid] = 1;
        pthread_cond_signal(&board_cv[first_pid]);
        while(!board_ack[first_pid]){
            pthread_cond_wait(&board_ack_cv[first_pid], &board_mutex[first_pid]);
        }
        board_ack[first_pid] = 0;
        pthread_mutex_unlock(&board_mutex[first_pid]);

        for(int j = 1; j < p; j++){
            struct timespec now_rt;
            clock_gettime(CLOCK_REALTIME, &now_rt);

            int secs_waited = now_rt.tv_sec - load_start_rt.tv_sec;
            if(secs_waited >= w){
                break;
            }

            struct timespec deadline_rt = load_start_rt;
            deadline_rt.tv_sec +=w;

            //Monitor Update(car dequeueing ride line)
            pthread_mutex_lock(&car_state_mutex);
            car_current_state[car_id] = 1;
            car_current_load[car_id] = 1;
            pthread_mutex_unlock(&car_state_mutex);
            //=======================================
            int next_pid;
            if(tq_timed_dequeue(&ride_line, &next_pid, &deadline_rt) == 0){
                pids[j] = next_pid;
                riders_collected++;

                pthread_mutex_lock(&board_mutex[next_pid]);
                board_flag[next_pid] = 1;
                pthread_cond_signal(&board_cv[next_pid]);
                //Monitor Update (increment car load)
                pthread_mutex_lock(&car_state_mutex);
                car_current_load[car_id]++;
                pthread_mutex_unlock(&car_state_mutex);
                //===================================
                while(!board_ack[next_pid]){
                    pthread_cond_wait(&board_ack_cv[next_pid], &board_mutex[next_pid]);
                }
                board_ack[next_pid] = 0;
                pthread_mutex_unlock(&board_mutex[next_pid]);
            }
            else{
                break;
            }
        }

        printf("[Time: %.3d s] Car %d departed with %d attendants(s).\n", elapsed_seconds(), car_id, riders_collected);
        for(int i = 0; i < riders_collected; i++){
            printf("Attendant: %d\n", pids[i]);
        }
        pthread_mutex_unlock(&ride_mutex);
        //Monitor Update (can departed)
        pthread_mutex_lock(&car_state_mutex);
        car_current_state[car_id] = 2;
        pthread_mutex_unlock(&car_state_mutex);
        //============================

        sleep(r);

        pthread_mutex_lock(&ride_mutex);
        for(int i = 0; i < riders_collected; i++){
            int pid_to_unload = pids[i];
            printf("[Time: %.3d s] Car %d invoked unload() for passenger %d.\n", elapsed_seconds(), car_id, pid_to_unload);
            //Monitor Update(car unloading)
            pthread_mutex_lock(&car_state_mutex);
            car_current_state[car_id] = 3;
            pthread_mutex_unlock(&car_state_mutex);
            //=============================
            pthread_mutex_lock(&unboard_mutex[pid_to_unload]);
            unboard_flag[pid_to_unload] = 1;
            pthread_cond_signal(&unboard_cv[pid_to_unload]);
            while(unboard_flag[pid_to_unload] != 0){
                pthread_mutex_unlock(&unboard_mutex[pid_to_unload]);
                sched_yield();
                pthread_mutex_lock(&unboard_mutex[pid_to_unload]);
            }
            pthread_mutex_unlock(&unboard_mutex[pid_to_unload]);
        }
        pthread_mutex_unlock(&ride_mutex);
        //Monitor Update(car idle)
        pthread_mutex_lock(&car_state_mutex);
        car_current_state[car_id] = 0;
        car_current_load[car_id] = 0;
        pthread_mutex_unlock(&car_state_mutex);
        //========================
    }
    return NULL;
}

void monitor_loop(void){
    snapshot_t snap;
    ssize_t nbytes;

    printf("[Monitor] started. Waiting for snapshots...\n");
    
    while((nbytes = read(monitor_pipe_fd[0], &snap, sizeof(snap))) > 0){
        if(snap.elapsed_time < 0) break;

        char buf[4096];
        int pos = 0;

        pos += snprintf(buf + pos, sizeof(buf) - pos, "\n");

        pos += snprintf(buf + pos, sizeof(buf) - pos, "[Monitor] System State at %03d s\n", snap.elapsed_time);

        pos += snprintf(buf + pos, sizeof(buf) - pos, "Ticketqueue=%d RideQueue=%d\n", snap.ticket_queue_len, snap.ride_queue_len);

        for(int i = 0; i < snap.num_cars; i++){
            const char *st;
            switch(snap.car_states[i]){
                case 0: st = "idle"; break;
                case 1: st = "loading"; break;
                case 2: st = "riding"; break;
                case 3: st = "unloading"; break;
                default: st = "unknown"; break;
            }
            pos += snprintf(buf + pos, sizeof(buf) - pos, " Car%02d: %-8s (%2d riders)\n", i, st, snap.car_load_counts[i]);
        }

        for(int j = 0; j < snap.num_attendants; j++){
            const char *ast;
            switch(snap.attendant_states[j]){
                case 0: ast = "exploring"; break;
                case 1: ast = "waiting_ticket"; break;
                case 2: ast = "waiting_ride"; break;
                case 3: ast = "in_car"; break;
                default: ast = "unknown"; break;
            }
            pos += snprintf(buf + pos, sizeof(buf) - pos, " Attendant%02d: %-12s\n", j, ast);

        }

        pos += snprintf(buf + pos, sizeof(buf) - pos, "\n");

        write(STDOUT_FILENO, buf, pos);
    }
    close(monitor_pipe_fd[0]);
    exit(0);
}

void parent_sampler_loop(void){
    snapshot_t snap;
    memset(&snap, 0, sizeof(snap));

    snap.num_cars = g_num_cars;
    snap.num_attendants = g_num_attendants;

    while(!simulation_done){
        sleep(5);

        snap.elapsed_time = elapsed_seconds();

        pthread_mutex_lock(&ticket_line.lock);
        snap.ticket_queue_len = ticket_line.size;
        pthread_mutex_unlock(&ticket_line.lock);

        pthread_mutex_lock(&ride_line.lock);
        snap.ride_queue_len = ride_line.size;
        pthread_mutex_unlock(&ride_line.lock);

        pthread_mutex_lock(&car_state_mutex);
        for(int i = 0; i < g_num_cars; i++){
            snap.car_states[i] = car_current_state[i];
            snap.car_load_counts[i] = car_current_load[i];
        }
        pthread_mutex_unlock(&car_state_mutex);

        pthread_mutex_lock(&attendant_state_mutex);
        for(int i = 0; i < g_num_attendants; i++){
            snap.attendant_states[i] = attendant_current_state[i];
        }
        pthread_mutex_unlock(&attendant_state_mutex);

        ssize_t ww = write(monitor_pipe_fd[1], &snap, sizeof(snap));
        if(ww < 0){
            perror("[Sampler] write");
            break;
        }
    }
    snapshot_t final_snap;
    memset(&final_snap, 0, sizeof(final_snap));
    final_snap.elapsed_time = -1;
    write(monitor_pipe_fd[1], &final_snap, sizeof(final_snap));
    close(monitor_pipe_fd[1]);
}