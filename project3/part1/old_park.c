#include "old_park.h"

static int elapsed_seconds(void){
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    time_t sec_diff = now.tv_sec - start_time.tv_sec;
    long nsec_diff = now.tv_nsec - start_time.tv_nsec;
    return (int)sec_diff + (int)nsec_diff / 1e9;
}

void enterTicketLine(void){
    unsigned long my_ticket = atomic_fetch_add(&next_ticket, 1UL);

    while(atomic_load(&now_serving) != my_ticket){
        struct timespec ts = {0, 100000};
        nanosleep(&ts, NULL);
    }
}

void exitTicketLine(void){
    atomic_fetch_add(&now_serving, 1UL);
}

void *monitor(void *arg){
    for(int i = 1; i <= 6; i++){
        sleep(5);

        int e = atomic_load(&exploringCount);
        int wT = atomic_load(&waitingTicketCount);
        int wB = atomic_load(&waitingBoardCount);
        int rC = atomic_load(&ridingCount);
        int wU = atomic_load(&waitingUnboardCount);

        printf("\n");
        printf("[Monitor] System State at %.3d s\n", elapsed_seconds());
        printf("Exploring: %d\n", e);
        printf("Waiting For Tickets: %d\n", wT);
        printf("Waiting To Board: %d\n", wB);
        printf("Riding: %d\n", rC);
        printf("Waiting to Unboard: %d\n", wU);
        printf("\n");
    }
    atomic_store(&running, 0);
    return NULL;
}

void* attendant(void* arg){
    int *id = (int *)arg;

    atomic_fetch_add(&exploringCount, 1);

    while(atomic_load(&running)){
        printf("[Time: %.3d s] Thread %d is exploring the park.\n", elapsed_seconds(), *id);
        int min = 2,max = 5;
        int range = max - min +1;
        int r2to5 = (rand() % range) + min;
        sleep(r2to5);

        atomic_fetch_sub(&exploringCount, 1);

        printf("[Time: %.3d s] Thread %d finished exploring, heading to ticket booth.\n", elapsed_seconds(), *id);
        atomic_fetch_add(&waitingTicketCount, 1);
        enterTicketLine();
        atomic_fetch_sub(&waitingTicketCount, 1);
        printf("[Time: %.3d s] Thread %d waiting in ticket line.\n", elapsed_seconds(), *id);
        exitTicketLine();
        printf("[Time: %.3d s] Thread %d acquired a ticket.\n", elapsed_seconds(), *id);

        atomic_fetch_add(&waitingBoardCount, 1);
        sem_wait(&sem_boardQueue);
        atomic_fetch_sub(&waitingBoardCount, 1);
        atomic_fetch_add(&ridingCount, 1);
        pthread_mutex_lock(&ride_mutex);
            boardedCount++;
            printf("[Time: %.3d s] Thread %d boarded (seat %d/%d).\n", elapsed_seconds(), *id, boardedCount, p);
        pthread_mutex_unlock(&ride_mutex);

        atomic_fetch_sub(&ridingCount, 1);
        atomic_fetch_add(&waitingUnboardCount, 1);
        sem_wait(&sem_unboardQueue);
        atomic_fetch_sub(&waitingUnboardCount, 1);
        pthread_mutex_lock(&ride_mutex);
            unboardedCount++;
            printf("[Time: %.3d s] Thread %d unboarded (count %d/%d).\n", elapsed_seconds(), *id, unboardedCount, p);
        pthread_mutex_unlock(&ride_mutex);
        
        atomic_fetch_add(&exploringCount, 1);
    }
    return NULL;
}

void *car(void *arg){
    int round = 0;

    while(atomic_load(&running)){
        round++;

        pthread_mutex_lock(&ride_mutex);
        boardedCount = 0;
        unboardedCount = 0;
        pthread_mutex_unlock(&ride_mutex);
        printf("[Time: %.3d s] Car load phase started. Waiting for up to %d passengers...\n", elapsed_seconds(), p);
        for(int i = 0; i < p && atomic_load(&running); i++){
            sem_post(&sem_boardQueue);
        }
    

        while(atomic_load(&running)){
            pthread_mutex_lock(&ride_mutex);
            int bc = boardedCount;
            pthread_mutex_unlock(&ride_mutex);
            if(bc == p){
                break;
            }
            struct timespec ts = {0, 100000};
            nanosleep(&ts, NULL);
        }
        printf("[Time: %.3d s] All %d passenger(s) boarded. Departing (round %d).\n", elapsed_seconds(), p, round);
        sleep(r);
        printf("[Time: %.3d s] Ride has ended, car is unloading.\n", elapsed_seconds());
        for(int i = 0; i < p; i++){
            sem_post(&sem_unboardQueue);
        }

        while(atomic_load(&running)){
            pthread_mutex_lock(&ride_mutex);
            int uc = unboardedCount;
            pthread_mutex_unlock(&ride_mutex);
            if(uc == p){
                break;
            }
            struct timespec ts = {0, 100000};
            nanosleep(&ts, NULL);
        }
        printf("[Time: %.3d s] All %d passengers have unboarded. Ride complete (round %d).\n", elapsed_seconds(), p, round);
    }

    return NULL;
}

int main(int argc, char* argv[]){
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

    n = 1;
    c = 1;

    sem_init(&sem_boardQueue, 0, 0);
    sem_init(&sem_unboardQueue, 0, 0);

    atomic_store(&exploringCount, 0);
    atomic_store(&waitingTicketCount, 0);
    atomic_store(&waitingBoardCount, 0);
    atomic_store(&ridingCount, 0);
    atomic_store(&waitingUnboardCount, 0);
    atomic_store(&running, 1);

    pthread_t monitor_tid;
    if(pthread_create(&monitor_tid, NULL, monitor, NULL) != 0){
        perror("pthread_create(monitor)");
        exit(EXIT_FAILURE);
    }

    cars = malloc(sizeof(pthread_t) * c);
    if(!cars){
        perror("malloc(cars)");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < c; i++){
        if(pthread_create(&cars[i], NULL, car, NULL) != 0){
            perror("pthread_create(car)");
            exit(EXIT_FAILURE);
        }
    }

    attendants = malloc(sizeof(pthread_t) * n);
    if(!attendants){
        perror("malloc(attendants)");
        exit(EXIT_FAILURE);
    }
    int *numbers = malloc(sizeof(int) * n);
    if(!numbers){
        perror("malloc(numbers)");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < n; i++){
        numbers[i] = i+1;
        if(pthread_create(&attendants[i], NULL, attendant, &numbers[i]) != 0){
            perror("pthread_create(attendant)");
            exit(EXIT_FAILURE);
        }
    }

    pthread_join(monitor_tid, NULL);

    for(int i = 0; i < n; i++){
        pthread_join(cars[i], NULL);
    }
    for(int i = 0; i < n; i++){
        pthread_join(attendants[i], NULL);
    }

    free(cars);
    free(attendants);
    free(numbers);
    sem_destroy(&sem_boardQueue);
    sem_destroy(&sem_unboardQueue);
    return 0;
}


