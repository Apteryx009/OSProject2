#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

pthread_mutex_t chipsBagMutex;
int numPlayers = 6;
int chipsInBag = 30;
int chipsInBagREFILL = 0;
int totalBagOfChips = 0;
float seed = 0;
int rounds = 1;                                 // Global variable to track the number of completed rounds
volatile int keepPlaying = 1;                   // Global flag
int currentPlayer = 1;                          // Global variable to track the current player, start from 1
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; // Condition variable
FILE *logFile;
pthread_mutex_t cardDeckMutex;
pthread_mutex_t greasyCardMutex;

typedef struct
{
    char *suit;
    char *value;
    int numericValue; // This will help in comparing cards
} Card;

// deck stuff
#define DECK_SIZE 52
Card ourDeck[DECK_SIZE];
Card greasyCard;
int deckSize = DECK_SIZE; // Initialize with the total number of cards in the deck

// Just basic card info you would expect in a set without jokers
void *initializeDeck(Card *deck)
{
    char *suits[] = {"Hearts", "Diamonds", "Clubs", "Spades"};
    char *values[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King", "Ace"};

    for (int i = 0; i < DECK_SIZE; i++)
    {
        deck[i].suit = suits[i / 13];
        deck[i].value = values[i % 13];

        // Assigning numeric values
        if (i % 13 >= 0 && i % 13 <= 8)
        {
            // For cards 2-10, the numeric value is the card number
            deck[i].numericValue = i % 13 + 2;
        }
        else if (i % 13 == 9)
        {
            // Jack
            deck[i].numericValue = 11;
        }
        else if (i % 13 == 10)
        {
            // Queen
            deck[i].numericValue = 12;
        }
        else if (i % 13 == 11)
        {
            // King
            deck[i].numericValue = 13;
        }
        else if (i % 13 == 12)
        {
            // Ace, can be treated as 1 or 14 depending on game rules
            deck[i].numericValue = 14; // or 1
        }
    }

    fprintf(logFile, "Deck initialized.\n");
}

// Shuffle the deck using Fisher-Yates algo
void *shuffleDeck(Card *deck)
{
    for (int i = 0; i < DECK_SIZE; i++)
    {
        int range = DECK_SIZE - i;
        if (range <= 0)
        {
            // Prevents divide by zero if range is 0 or negative
            continue;
        }
        int r = i + rand() % range; // Using modulus operator to avoid division
        Card temp = deck[i];
        deck[i] = deck[r];
        deck[r] = temp;
    }
}

// Print the deck
void *printDeck(Card *deck)
{
    fprintf(logFile, "REMAINING DECK SIZE: %d \n", deckSize);
    for (int i = 0; i < DECK_SIZE; i++)
    {
        fprintf(logFile, " %s of %s ", deck[i].value, deck[i].suit);
    }
    fprintf(logFile, "\n");
}

// Remove a card at random from the deck to be the greasy card
void *drawGreasyCard(Card *deck)
{
    pthread_mutex_lock(&greasyCardMutex);
    //fprintf(logFile, "greasyyy %d ", deckSize);

    if (deckSize < numPlayers)
    {
        fprintf(logFile, "\nNot enough remaining cards!!! GAME OVER \n");

        // fprintf(logFile, ("Thus, it's not mathematically possible to reach n rounds. Game over.");
        exit(0);
    }

    int index = rand() % (deckSize);
    greasyCard = deck[index];

    // Shift remaining cards
    for (int i = index; i < (deckSize)-1; i++)
    {
        deck[i] = deck[i + 1];
    }

    (deckSize)--; // Reduce the deck size

    pthread_mutex_unlock(&greasyCardMutex);
    // greasyCard = greasyCard01;
}

// Remove card from deck to give to a player
Card drawCard(Card *deck)
{
    pthread_mutex_lock(&greasyCardMutex);
    Card drawnCard = deck[0]; // Draw the top card

    // Shift remaining cards
    for (int i = 0; i < (deckSize)-1; i++)
    {
        deck[i] = deck[i + 1];
    }
    (deckSize)--; // Reduce the deck size
    pthread_mutex_unlock(&greasyCardMutex);
    return drawnCard;
}

// Simple comparsion function: determine if 2 cards of the of same value
// e.g., two kings, two queens, two jacks or any two numbers of the same value
int isMatch(Card playerCard, Card greasyCard)
{
    return playerCard.numericValue == greasyCard.numericValue;
}

void *routine(void *args)
{
    int roundWinner = 0;
    int playerNumber = *(int *)args;

    // Log the start of the thread for a player
    fprintf(logFile, "Thread, Player %d starting\n", playerNumber);

    // Seed for random number generation, unique per thread
    unsigned int localSeed = seed + (unsigned long)pthread_self();

    // Main loop for game play
    while (keepPlaying)
    {
        pthread_mutex_lock(&chipsBagMutex);

        // Wait for the player's turn
        while (playerNumber != currentPlayer)
        {
            pthread_cond_wait(&cond, &chipsBagMutex);
        }

        // Dealer actions: shuffle and draw the greasy card
        if (playerNumber == ((rounds % numPlayers) + 1))
        {
            pthread_mutex_lock(&cardDeckMutex);

            shuffleDeck(ourDeck);
            fprintf(logFile, "DECK SHUFFLED---\n");
            printDeck(ourDeck); // Assuming printDeck function exists
            drawGreasyCard(ourDeck);

            pthread_mutex_unlock(&cardDeckMutex);
        }

        // Drawing a card and comparing with the greasy card
        Card currentPlayerCard = drawCard(ourDeck);
        fprintf(logFile, "Player %d: hand (%d, %d) <> Greasy Card is %d \n",
                playerNumber, rounds, currentPlayerCard.numericValue, greasyCard.numericValue);

        // Check if the player wins the round
        if (isMatch(currentPlayerCard, greasyCard))
        {
            fprintf(logFile, "Player %d: WINS round %d \n", playerNumber, rounds);
            roundWinner = 1;

            // Update the greasy card to prevent further matches in this round
            greasyCard.suit = "Greasy";
            greasyCard.value = "Greasy";
            greasyCard.numericValue = -1;

            // Notify other players of the round result
            for (int i = 1; i <= numPlayers; i++)
            {
                if (i != playerNumber)
                {
                    fprintf(logFile, "Player %d: lost round %d \n", i, rounds);
                }
            }
            rounds++;
        }

        // Chips handling logic
        int chipsNeeded = (rand_r(&localSeed) % 5) + 1;
        if (chipsInBag - chipsNeeded < 0)
        {
            fprintf(logFile, "Player %d found no chips, refilling bag\n", playerNumber);
            chipsInBag = chipsInBagREFILL;
            totalBagOfChips++;
        }

        chipsInBag -= chipsNeeded;
        fprintf(logFile, "Player %d took %d chips from bag %d, chips left %d\n",
                playerNumber, chipsNeeded, totalBagOfChips, chipsInBag);

        // End of player's turn
        currentPlayer = (currentPlayer % numPlayers) + 1;

        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&chipsBagMutex);
    }

    // Log the end of the thread for a player
    fprintf(logFile, "Thread, Player %d ending\n", playerNumber);
    return NULL;
}

int main(int argc, char *argv[])
{
    // Check if a seed value is provided
    if (argc > 1)
    {
        seed = atoi(argv[1]); // Convert the first argument to an integer
        numPlayers = atoi(argv[2]);
        chipsInBag = atoi(argv[3]);
    }
    else
    {
        // If no seed is provided, you could default to the current time
        // seed = time(NULL);
        printf("Please use format: int{seed} int{num_of_players} int{chips_in_each_bag}\n");
        exit(0);
    }

    chipsInBagREFILL = chipsInBag;

    logFile = fopen("OSlog.txt", "w"); // Open for writing, overwrites existing content
    if (logFile == NULL)
    {
        perror("Error opening log file");
        return 1;
    }

    srand(seed);
    pthread_t players[numPlayers];
    pthread_mutex_init(&chipsBagMutex, NULL);
    pthread_mutex_init(&cardDeckMutex, NULL);
    pthread_mutex_init(&greasyCardMutex, NULL);

    // Initialize the card deck
    initializeDeck(ourDeck);

    fprintf(logFile, "Note that (3, 9), this would mean round 3, card value of 9 \n");
    fprintf(logFile, "The game ENDS when we have reached n rounds OR out of cards! \n");

    int *playerNumbers = malloc(numPlayers * sizeof(int)); // Dynamic allocation
    if (playerNumbers == NULL)
    {
        perror("Failed to allocate memory for player numbers\n");
        exit(1);
    }

    for (int i = 0; i < numPlayers; i++)
    {
        playerNumbers[i] = i + 1; // Assign player number (1-based indexing)
        if (pthread_create(&players[i], NULL, &routine, &playerNumbers[i]) != 0)
        {
            perror("Failed to create player, that is, thread \n");
            free(playerNumbers);
            exit(1);
        }
    }

    for (int i = 0; i < numPlayers; i++)
    {
        if (pthread_join(players[i], NULL) != 0)
        {
            perror("Failed to join player, that is, thread");
        }
    }

    free(playerNumbers); // Free the dynamically allocated memory

    pthread_mutex_destroy(&chipsBagMutex);
    pthread_mutex_destroy(&cardDeckMutex);
    pthread_mutex_destroy(&greasyCardMutex);

    fclose(logFile); // Close the file at the end of your program
    return 0;
}