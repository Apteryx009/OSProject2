#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

pthread_mutex_t chipsBagMutex;
const int numPlayers = 6;
int chipsInBag = 30;
int seed = 0;

void *routine(void *args) {
    printf("Thread starting\n");

    unsigned int localSeed = seed + (unsigned long)pthread_self();
    if (pthread_mutex_lock(&chipsBagMutex) == 0) {
        int chipsNeeded = (rand_r(&localSeed) % 5) + 1;
        if (chipsInBag - chipsNeeded < 0)
        {
            printf("No more chips... opening new Bag\n");
            // Open new Bag
            chipsInBag = 30;
            chipsInBag -= chipsNeeded;
            printf("Chips left %d\n", chipsInBag);
        }
        else
        {
            chipsInBag -= chipsNeeded;
            sleep(1);
            printf("Chips left %d\n", chipsInBag);
        }
        printf("Thread ending\n"); // Print statement at the end of the routine
        pthread_mutex_unlock(&chipsBagMutex);
        
    }
}

int main(int argc, char* argv[]) {
    // Check if a seed value is provided
    if (argc > 1) {
        seed = atoi(argv[1]); // Convert the first argument to an integer
    } else {
        // If no seed is provided, you could default to the current time
        // seed = time(NULL);
        printf("No seed provided, using default seed 0\n");
    }
    
    srand(seed);
    pthread_t players[numPlayers];
    
    
    pthread_mutex_init(&chipsBagMutex, NULL);
    
    for (int i = 0; i < numPlayers; i++) {
        if (pthread_create(&players[i], NULL, &routine, NULL) != 0) {
            perror("Failed to create player, that is, thread \n");
        }
    }

    for (int i = 0; i < numPlayers; i++) {
        if (pthread_join(players[i], NULL) != 0) {
            perror("Failed to join player, that is, thread");
        }
    }
    
    pthread_mutex_destroy(&chipsBagMutex);
    return 0;
}
