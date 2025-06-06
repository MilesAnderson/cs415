#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "thread_queue.h"

#define MAX_RIDES 3

int n = 1;
int c = 1;
int p = 1;
int w = 10;
int r = 8;

void* car(void* arg);
void* attendant(void* arg);
static int elapsed_seconds(void);
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

thread_queue_t ticket_line;
thread_queue_t ride_line;

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

    tq_init(&ticket_line);
    tq_init(&ride_line);

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

    pthread_mutex_destroy(&ticket_mutex);
    pthread_mutex_destroy(&ride_mutex);
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
    return (int)sec_diff + (int)nsec_diff / 1e9;

}

void* attendant(void* arg){
    int pid = *(int*)arg;
    free(arg);

    for(int i = 0; i < MAX_RIDES; i++){
        //Explore
        printf("[Time: %.3d s] Attendant %d is exploring the park.\n", elapsed_seconds(), pid);
        int explore_time = (rand() % 4) + 2;
        sleep(explore_time);

        //Get Ticket
        pthread_mutex_lock(&ticket_mutex);
        printf("[Time: %.3d s] Attendant %d waiting in ticket queue.\n", elapsed_seconds(), pid);
        tq_enqueue(&ticket_line, pid);
        int my_ticket = tq_dequeue(&ticket_line);
        printf("[Time: %.3d s] Attendant %d acquired ticket %d.\n", elapsed_seconds(), pid, my_ticket);
        pthread_mutex_unlock(&ticket_mutex);

        //Board
        printf("[Time: %.3d s] Attendant %d joined the ride queue.\n", elapsed_seconds(), pid);
        tq_enqueue(&ride_line, pid);
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

            int next_pid;
            if(tq_timed_dequeue(&ride_line, &next_pid, &deadline_rt) == 0){
                pids[j] = next_pid;
                riders_collected++;

                pthread_mutex_lock(&board_mutex[next_pid]);
                board_flag[next_pid] = 1;
                pthread_cond_signal(&board_cv[next_pid]);
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

        sleep(r);

        pthread_mutex_lock(&ride_mutex);
        for(int i = 0; i < riders_collected; i++){
            int pid_to_unload = pids[i];
            printf("[Time: %.3d s] Car %d invoked unload() for passenger %d.\n", elapsed_seconds(), car_id, pid_to_unload);
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
    }
    return NULL;
}