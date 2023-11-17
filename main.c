#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

pthread_mutex_t chipsBagMutex;
const int numPlayers = 6;
int chipsInBag = 30;
int seed = 0;
int roundsCompleted = 0; // Global variable to track the number of completed rounds
volatile int keepPlaying = 1; // Global flag
int currentPlayer = 1; // Global variable to track the current player, start from 1
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; // Condition variable



void *routine(void *args) {
    int playerNumber = *(int *)args;

    printf("Thread, Player %d starting\n", playerNumber);

    unsigned int localSeed = seed + (unsigned long)pthread_self();

    while (keepPlaying) {
        printf("Player %d trying to lock mutex\n", playerNumber);
        pthread_mutex_lock(&chipsBagMutex);
        printf("Player %d acquired the mutex\n", playerNumber);

        while (playerNumber != currentPlayer) {
            printf("Player %d waiting for turn. Current player: %d\n", playerNumber, currentPlayer);
            pthread_cond_wait(&cond, &chipsBagMutex);
            printf("Player %d woke up from cond_wait\n", playerNumber);
        }

        // Chip taking logic
        int chipsNeeded = (rand_r(&localSeed) % 5) + 1;
        printf("Player %d needs %d chips\n", playerNumber, chipsNeeded);

        if (chipsInBag - chipsNeeded < 0) {
            printf("Player %d found no chips, refilling bag\n", playerNumber);
            chipsInBag = 30;
        }

        chipsInBag -= chipsNeeded;
        printf("Player %d took %d chips, Chips left %d\n", playerNumber, chipsNeeded, chipsInBag);

        // Check for round completion and move to the next player
        if (playerNumber == numPlayers) {
            roundsCompleted++;
            printf("Player %d completed a round. Total rounds completed: %d\n", playerNumber, roundsCompleted);
            if (roundsCompleted >= numPlayers) {
                printf("Player %d found that enough rounds are completed, stopping the game\n", playerNumber);
                keepPlaying = 0;
            }
        }

        currentPlayer = (currentPlayer % numPlayers) + 1;
        printf("Player %d finished turn. Next player: %d\n", playerNumber, currentPlayer);

        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&chipsBagMutex);
        printf("Player %d unlocked mutex and finished turn\n", playerNumber);
    }

    printf("Thread, Player %d ending\n", playerNumber);
    return NULL;
}






int main(int argc, char *argv[])
{
    // Check if a seed value is provided
    if (argc > 1)
    {
        seed = atoi(argv[1]); // Convert the first argument to an integer
    }
    else
    {
        // If no seed is provided, you could default to the current time
        // seed = time(NULL);
        printf("No seed provided, using default seed 0\n");
    }

    srand(seed);
    pthread_t players[numPlayers];

    pthread_mutex_init(&chipsBagMutex, NULL);
    int playerNumbers[numPlayers]; // Array to hold player numbers
    for (int i = 0; i < numPlayers; i++)
    {
        playerNumbers[i] = i + 1; // Assign player number (1-based indexing)
        if (pthread_create(&players[i], NULL, &routine, &playerNumbers[i]) != 0)
        {
            perror("Failed to create player, that is, thread \n");
        }
    }

    for (int i = 0; i < numPlayers; i++)
    {
        if (pthread_join(players[i], NULL) != 0)
        {
            perror("Failed to join player, that is, thread");
        }
    }

    pthread_mutex_destroy(&chipsBagMutex);
    return 0;
}
