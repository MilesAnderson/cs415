#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define MAX_RIDES 5

void* car(void* arg);
void* attendant(void* arg);
static int elapsed_seconds(void);

pthread_mutex_t ticket_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ride_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ride_arrived_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t board_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t board_ack_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t unboard_cond = PTHREAD_COND_INITIALIZER;
int boarded = 0;
int board_ack = 0;
int unboarded = 0;
int ride_queue_size = 0;

static struct timespec start_time;

int n = 1;
int c = 1;
int p = 1;
int w = 5;
int r = 3;

int main(int argc, char *argv[]){
    if(clock_gettime(CLOCK_MONOTONIC, &start_time) < 0){
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

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
    printf("- Simulation Duration: 5 rides\n");
    printf("\n");

    n = 1;
    c = 1;
    p = 1;

    pthread_t cars;
    pthread_t attendants;

    int *a_arg = malloc(sizeof(int)); 
    *a_arg = 1;
    int *c_arg = malloc(sizeof(int));
    *c_arg = 1;

    pthread_create(&cars, NULL, car, c_arg);
    pthread_create(&attendants, NULL, attendant, a_arg);

    pthread_join(attendants, NULL);
    pthread_join(cars, NULL);

    pthread_mutex_destroy(&ticket_mutex);
    pthread_mutex_destroy(&ride_mutex);

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
        printf("[Time: %.3d s] Attendant %d is exploring the park.\n", elapsed_seconds(), pid);
        int explore_time = (rand() % 4) + 2;
        sleep(explore_time);

        pthread_mutex_lock(&ticket_mutex);
        printf("[Time: %.3d s] Attendant %d waiting in ticket queue.\n", elapsed_seconds(), pid);
        printf("[Time: %.3d s] Attendant %d acquired a ticket.\n", elapsed_seconds(), pid);
        pthread_mutex_unlock(&ticket_mutex);

        pthread_mutex_lock(&ride_mutex);
        ride_queue_size++;
        printf("[Time: %.3d s] Attendant %d joined the ride queue.\n", elapsed_seconds(), 1);
        pthread_cond_signal(&ride_arrived_cond);

        while(!boarded){
            pthread_cond_wait(&board_cond, &ride_mutex);
        }
        printf("[Time: %.3d s] Attendant %d boarded car 1.\n", elapsed_seconds(), 1);
        boarded = 0;
        board_ack = 1;
        pthread_cond_signal(&board_ack_cond);
        pthread_mutex_unlock(&ride_mutex);

        pthread_mutex_lock(&ride_mutex);
        while(!unboarded){
            pthread_cond_wait(&unboard_cond, &ride_mutex);
        }
        printf("[Time: %.3d s] Attendant %d unboarded car 1.\n", elapsed_seconds(), 1);
        unboarded = 0;
        pthread_mutex_unlock(&ride_mutex);
    }
    return NULL;
}

void* car(void* arg){
    int car_id = *(int*)arg;
    free(arg);

    for(int i = 0; i < MAX_RIDES; i++){
        pthread_mutex_lock(&ride_mutex);
        while(ride_queue_size == 0){
            pthread_cond_wait(&ride_arrived_cond, &ride_mutex);
        }
        printf("[Time: %.3d s] Car %d invoked load().\n", elapsed_seconds(), car_id);
        ride_queue_size--;
        boarded = 1;
        pthread_cond_signal(&board_cond);

        while(!board_ack){
            pthread_cond_wait(&board_ack_cond, &ride_mutex);
        }
        board_ack = 0;

        printf("[Time: %.3d s] Car %d departed with 1 passenger(s).\n", elapsed_seconds(), car_id);
        pthread_mutex_unlock(&ride_mutex);
        sleep(r);

        printf("[Time: %.3d s] Car %d invoked unload().\n", elapsed_seconds(), car_id);
        pthread_mutex_lock(&ride_mutex);
        unboarded = 1;
        pthread_cond_signal(&unboard_cond);
        pthread_mutex_unlock(&ride_mutex);
    }
    return NULL;
}