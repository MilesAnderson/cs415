#include "park.h"

#define NUM_WORKERS 1

pthread_t *attendants;

static struct timespec start_time;

static int elapsed_seconds(void){
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    time_t sec_diff = now.tv_sec - start_time.tv_sec;
    long nsec_diff = now.tv_nsec - start_time.tv_nsec;
    return (int)sec_diff + (int)nsec_diff / 1e9;
}

void* attendant(void* arg){
    int *id = (int *)arg;
    printf("[Time: %.3d s] Thread %d has entered the park.\n", elapsed_seconds(), *id);
    printf("[Time: %.3d s] Thread %d is exploring the park.\n", elapsed_seconds(), *id);
    int min = 2,max = 5;
    int range = max - min +1;
    int r2to5 = (rand() % range) + min;
    sleep(r2to5);
    printf("[Time: %.3d s] Thread %d finished exploring, heading to ticket booth.\n", elapsed_seconds(), *id);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]){
    if(clock_gettime(CLOCK_MONOTONIC, &start_time) < 0){
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    int opt;
    int n = NUM_WORKERS;
    int c = 1;
    int p = 1;
    int w = 3;
    int r = 6;

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
            case ':':
                printf("option needs a value\n");
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
    printf("- Park exploration time: 2-5 seconds\n");
    printf("- Car waiting period: %d seconds\n", w);
    printf("- Ride duration: %d seconds\n", r);
    printf("\n");

    attendants = (pthread_t *)malloc(sizeof(pthread_t) * n);

    int *numbers = (int*)malloc(sizeof(int) * n);
    for(int i = 0; i < n; i++){
        numbers[i] = i+1;
    }


    for(int i = 0; i < n; ++i){
        pthread_create(&attendants[i], NULL, attendant, (void*)&(numbers[i]));
    }

    for(int j = 0; j < n; ++j){
        pthread_join(attendants[j], NULL);
    }

    free(attendants);
    free(numbers);
}


