#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

pthread_mutex_t chipsBagMutex;
const int numPlayers = 6;
int chipsInBag = 30;
float seed =0;
int rounds = 1; // Global variable to track the number of completed rounds
volatile int keepPlaying = 1; // Global flag
int currentPlayer = 1; // Global variable to track the current player, start from 1
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; // Condition variable






pthread_mutex_t cardDeckMutex;
// Structure to represent a card
typedef struct {
    char *suit;
    char *value;
    int numericValue; // This will help in comparing cards
} Card;


//deck stuff
#define DECK_SIZE 52
Card ourDeck[DECK_SIZE];
Card greasyCard;

int deckSize = DECK_SIZE; // Initialize with the total number of cards in the deck



void initializeDeck(Card *deck) {
    char *suits[] = {"Hearts", "Diamonds", "Clubs", "Spades"};
    char *values[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King", "Ace"};

    for (int i = 0; i < DECK_SIZE; i++) {
        deck[i].suit = suits[i / 13];
        deck[i].value = values[i % 13];

        // Assigning numeric values
        if (i % 13 >= 0 && i % 13 <= 8) {
            // For cards 2-10, the numeric value is the card number
            deck[i].numericValue = i % 13 + 2;
        } else if (i % 13 == 9) {
            // Jack
            deck[i].numericValue = 11;
        } else if (i % 13 == 10) {
            // Queen
            deck[i].numericValue = 12;
        } else if (i % 13 == 11) {
            // King
            deck[i].numericValue = 13;
        } else if (i % 13 == 12) {
            // Ace, can be treated as 1 or 14 depending on game rules
            deck[i].numericValue = 14; // or 1
        }
    }

    printf("Deck initialized.\n");
}



void shuffleDeck(Card *deck) {
    for (int i = 0; i < DECK_SIZE; i++) {
        int range = DECK_SIZE - i;
        if (range <= 0) {
            // Prevents divide by zero if range is 0 or negative
            continue;
        }
        int r = i + rand() % range;  // Using modulus operator to avoid division
        Card temp = deck[i];
        deck[i] = deck[r];
        deck[r] = temp;
    }
}


// Print the deck
void printDeck(Card *deck) {
    printf("Deck size NOW %d \n", deckSize);
    for (int i = 0; i < DECK_SIZE; i++) {
        printf("%s of %s\n", deck[i].value, deck[i].suit);
    }
}


Card drawGreasyCard(Card *deck) {
    int index = rand() % (deckSize);
    Card greasyCard = deck[index];

    // Shift remaining cards
    for (int i = index; i < (deckSize) - 1; i++) {
        deck[i] = deck[i + 1];
    }
    (deckSize)--; // Reduce the deck size

    return greasyCard;
}


Card drawCard(Card *deck) {
    Card drawnCard = deck[0]; // Draw the top card

    // Shift remaining cards
    for (int i = 0; i < (deckSize) - 1; i++) {
        deck[i] = deck[i + 1];
    }
    (deckSize)--; // Reduce the deck size

    return drawnCard;
}


int isMatch(Card playerCard, Card greasyCard) {
    return playerCard.numericValue == greasyCard.numericValue;
}







//Multi threading stuff here turn taking------------
void *routine(void *args) { 
    int roundWinner = 0;
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


        //Logic here for cards
        //this functions only happens
        // Check if current player is the dealer, if so proceed, shuffle, draw the greasy.
        if (playerNumber == ((rounds % numPlayers) + 1)) {
            printf("Player %d is the dealer for this round\n", playerNumber);
            pthread_mutex_lock(&cardDeckMutex);
            
            shuffleDeck(ourDeck);  
            greasyCard = drawGreasyCard(ourDeck); 
            //printDeck(ourDeck);
            
            
            pthread_mutex_unlock(&cardDeckMutex);
            // Additional dealer responsibilities can be added here
        }

        //Card drawing, comparing, and potato Chip taking logic
        Card currentPlayerCard;
        currentPlayerCard = drawCard(ourDeck);
        printf("Player %d: hand (%d, %d) <> Greasy Card is %d \n", playerNumber, rounds, 
                                        currentPlayerCard.numericValue, greasyCard.numericValue);

        // Ensure the if condition is enclosed in parentheses
        if (isMatch(currentPlayerCard, greasyCard)) {
            printf("Player %d: wins round %d \n", playerNumber, rounds);
            roundWinner = 1;
           
            // Additional logic for when the player wins

            //Greasy card won't match with any other in round now:
            greasyCard.suit = "Greasy"; // Arbitrary suit name
            greasyCard.value = "Greasy"; // Arbitrary value name
            greasyCard.numericValue = -1; // Absurd number that doesn't match any card SHOWING A WIN IF -1


            // Notify other players they lost the round
            for (int i = 1; i <= numPlayers; i++) {
                if (i != playerNumber) {
                    printf("Player %d: lost round %d \n", i, rounds);
                }
            }
            rounds++; 
        } 

        int chipsNeeded = (rand_r(&localSeed) % 5) + 1;
        printf("Player %d needs %d chips\n", playerNumber, chipsNeeded);

        if (chipsInBag - chipsNeeded < 0) {
            printf("Player %d found no chips, refilling bag\n", playerNumber);
            chipsInBag = 30;
        }

        chipsInBag -= chipsNeeded;
        printf("Player %d took %d chips, Chips left %d\n", playerNumber, chipsNeeded, chipsInBag);

        // Check for round completion and move to the next player
        
            //rounds++;
            //printf("Player %d completed a round. Total rounds completed: %d\n", playerNumber, rounds);
            if (rounds >= numPlayers) {
                //rintf("Player %d found that enough rounds are completed, stopping the game\n", playerNumber);
                keepPlaying = 0;
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
        printf("No seed provided, using default seed\n");
    }

    srand(seed);
    pthread_t players[numPlayers];

    pthread_mutex_init(&chipsBagMutex, NULL);
    pthread_mutex_init(&cardDeckMutex, NULL);

    //init the card deck
    initializeDeck(ourDeck);
    //printDeck(ourDeck);
    //shuffleDeck(ourDeck);
    //printf("------------------");
    //printDeck(ourDeck);
    

    


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
    pthread_mutex_destroy(&cardDeckMutex);
    
    return 0;
}