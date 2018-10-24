#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "server.h"

#include "constants.h"

/**
 * LeaderBoard struct to store all server information about scores.
 */
LeaderBoard leaderBoard;

/**
 * Checked to see if it should continue waiting for new connections.
 */
bool serverRunning;


/**
 * Receive input from client. Will sit and wait until receives communication.
 * @param socket_id ID of the socket to communicate over
 * @param buf buffer to store what is received into
 * @return buffer containing message received
 */
char *receiveString(int socket_id, char *buf) {
    int number_of_bytes = 0;

    if ((number_of_bytes = recv(socket_id, buf, MAXDATASIZE, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[number_of_bytes] = '\0';

    return buf;
}

/**
 * Send a string to a given socket
 * @param socket_id ID of the socket to communicate over
 * @param message message to send
 */
void sendString(int socket_id, char *message) {
    if (send(socket_id, message, MAXDATASIZE, 0) == -1) {
        perror("send");
    }
    //printf("%s", message);
}

/**
 * Waits for input from client and then sends back a confirmation straight away
 * @param socket_id ID of socket to communicate over
 * @param buf buffer to receive into
 * @return recieved message
 */
char *receiveStringAndReply(int socket_id, char *buf) {
    receiveString(socket_id, buf);

    sendString(socket_id, "Received");

    return buf;
}

/**
 * Recursive algorithm to determine the state of all adjacent tiles. Whilst the number of adjacent mines at the given
 * x, y is 0, traverses all directly adjacent (including diagonals) tiles until an edge is hit or a non-zero amount
 * is discovered. Sends each update as it traverses.
 * @param gameState pointer to current game
 * @param surrounding integer representing discovered numAdjacent at the location, fills up as game board is traversed
 * @param x x location to check
 * @param y y location to check
 * @param socketID ID of the socket to communicate on
 * @param inputBuff buffer used for communication
 */
void getSurrounding(GameState *gameState, int surrounding[NUM_TILES_X][NUM_TILES_Y], int x, int y, int socketID,
                    char *inputBuff) {
    // If current location is outside of game board, return
    if (x < 0 || x >= NUM_TILES_X || y < 0 || y >= NUM_TILES_Y) {
        return;
    }

    // If we have already been here, return
    if (gameState->tiles[y][x].revealed) {
        return;
    }

    // Update the number adjacent for the current location based on actual amount
    surrounding[y][x] = gameState->tiles[y][x].adjacentMines;
    // Set this to be revealed so we don't keep checking the same location
    gameState->tiles[y][x].revealed = true;

    // Store the information needed for the client
    sprintf(inputBuff, "%d%d%d", x, y, surrounding[y][x]);

    // Send this information to the client
    sendString(socketID, inputBuff);

    // If we hit a zero, need to do the same process for all 8 surrounding locations
    if (surrounding[y][x] == 0) {
        getSurrounding(gameState, surrounding, x - 1, y - 1, socketID, inputBuff);
        getSurrounding(gameState, surrounding, x, y - 1, socketID, inputBuff);
        getSurrounding(gameState, surrounding, x + 1, y - 1, socketID, inputBuff);
        getSurrounding(gameState, surrounding, x - 1, y, socketID, inputBuff);
        getSurrounding(gameState, surrounding, x + 1, y, socketID, inputBuff);
        getSurrounding(gameState, surrounding, x - 1, y + 1, socketID, inputBuff);
        getSurrounding(gameState, surrounding, x, y + 1, socketID, inputBuff);
        getSurrounding(gameState, surrounding, x + 1, y + 1, socketID, inputBuff);
    }
}

/**
 * Method to handle an individual game of Minesweeper for the given current player
 * @param socketID ID of the socket to communicate over
 * @param inputBuff buffer to store what is being received from the client
 * @param curr_player pointer to a player struct containing information about current player that is playing
 */
void handleGame(int socketID, char inputBuff[], Player *curr_player) {
    // TODO remove print statements
    // TODO possibly extract to methods
    // Get a new game with mines placed
    GameState *gameState = setupGame();
    // Currently playing (hasn't lost or quit)
    bool playing = true;
    // Timing variables
    time_t startTime = time(NULL);
    time_t endTime;
    time_t totalTime;

    curr_player->gamesPlayed++;

    char outputBuff[MAXDATASIZE];

    while (playing) {
        // Wait and receive input from client
        receiveString(socketID, inputBuff);

        if (strncmp(inputBuff, "Q", MAXDATASIZE) == 0) {
            // If they want to quit
            playing = false;
        } else if (inputBuff[2] == 'R') {
            // If they want to reveal a tile

            // Get the coordinates they want to reveal
            int y = inputBuff[0] - Y_OFFSET;
            int x = inputBuff[1] - X_OFFSET;
            printf("User wants to reveal tile %c%c %d\n", y + Y_OFFSET, x + X_OFFSET, gameState->tiles[y][x].isMine);

            if (tileContainsMine(gameState, y, x)) {
                // They tried to reveal a mine, so store that information in buffer to send
                sprintf(outputBuff, MINE_MESSAGE);

                sendString(socketID, outputBuff);

                for (int i = 0; i < NUM_TILES_Y; ++i) {
                    for (int j = 0; j < NUM_TILES_X; ++j) {
                        if (gameState->tiles[i][j].isMine) {
                            sprintf(outputBuff, "%d%d", i, j);
                            sendString(socketID, outputBuff);
                        }
                    }
                }

                sprintf(outputBuff, END_OF_MESSAGE);

                // Stop the game
                playing = false;
            } else {
                // No mine at location, send information about chosen location and recursive surroundings if needed.
                int surrounding[NUM_TILES_Y][NUM_TILES_X] = {{0}};
                getSurrounding(gameState, surrounding, x, y, socketID, outputBuff);
                printf("y:%c x:%c adjacent:%d\n", y + Y_OFFSET, x + X_OFFSET, gameState->tiles[y][x].adjacentMines);
                // Send end of message so client knows to stop receiving
                sprintf(outputBuff, END_OF_MESSAGE);
            }

            // Send result to client
            sendString(socketID, outputBuff);
        } else if (inputBuff[2] == 'P') {
            // If they want to place a mine

            // Get the coordinates they want to place a mine at
            int y = inputBuff[0] - Y_OFFSET;
            int x = inputBuff[1] - X_OFFSET;
            printf("User wants to place flag at %d%d %d\n", y + Y_OFFSET, x + X_OFFSET, gameState->tiles[y][x].isMine);

            // If there is a mine at this location and they haven't already revealed it
            if (tileContainsMine(gameState, y, x) && !gameState->tiles[y][x].revealed) {
                gameState->remainingMines--;
                gameState->tiles[y][x].revealed = true;

                // Win condition
                if (gameState->remainingMines == 0) {
                    // Store win message in buffer to send
                    sprintf(inputBuff, WIN_MESSAGE);
                    // Stop the game
                    playing = false;
                    // Calculate the score
                    endTime = time(NULL);
                    totalTime = endTime - startTime;
                    // Add the score to the leaderboard
                    addScore(curr_player, totalTime);
                } else {
                    // Tell user that there was a mine at this location
                    sprintf(inputBuff, MINE_MESSAGE);
                }
            } else {
                // No mine at this location
                sprintf(inputBuff, NO_MINE_MESSAGE);
            }

            // Send relevant information to user
            sendString(socketID, inputBuff);
        } else {
            // User has disconnected improperly
            playing = false;
        }
    }

    // Free the memory taken up by the game
    free(gameState);
}

/**
 * Sends the copy of the server's leaderboard to the user
 * @param socketID ID of the socket to send over
 */
void sendLeaderBoard(int socketID) {
    // Take the head of the list
    Score *current = leaderBoard.head;

    // Initialise buffer to send output to client
    char outputBuff[MAXDATASIZE];

    if (current == NULL) {
        sprintf(outputBuff, "\nThere is no information stored in the leaderboard. Try again later.\n\n");
        sendString(socketID, outputBuff);
    }

    // While it's not null
    while (current != NULL) {
        // Get the player pointer associated with this score's name
        Player *curr_player = getPlayer(current->name);

        // Store necessary information in buffer
        sprintf(outputBuff, "%s\t\t%d seconds\t\t%d games won, %d games played\n", current->name, current->time,
                curr_player->gamesWon, curr_player->gamesPlayed);

        // Send buffer
        sendString(socketID, outputBuff);
        // Move to next score in the list
        current = current->next;
    }

    // Send an end of message string to tell client to stop listening
    sendString(socketID, END_OF_MESSAGE);
}

/**
 * Sets up a new GameState struct for a new game session and returns the pointer to that.
 * @return pointer to new GameState struct ready to play a game
 */
GameState *setupGame() {
    // Allocate memory for the game state
    GameState *gameState = malloc(sizeof(GameState));

    // Initialise all variables
    clearGame(gameState);

    // Place mines randomly
    placeMines(gameState);

    // Pre-calculate the number of adjacent mines for each tile
    calculateAdjacent(gameState);

    // Show the mines on the field
    showBoard(gameState);

    return gameState;
}

/**
 * Check if there is a mine at coordinates (x, y)
 * @param gameState the gamestate struct to check for
 * @param x the x location
 * @param y the y location
 * @return true if there is a mine at (x, y), otherwise false
 */
bool tileContainsMine(GameState *gameState, int x, int y) {
    return gameState->tiles[x][y].isMine;
}

/**
 * Sets all GameState variables to 0 or false.
 * @param gameState Pointer to GameState struct to clear
 */
void clearGame(GameState *gameState) {
    // Initialise everything to false or 0
    for (int i = 0; i < NUM_TILES_Y; ++i) {
        for (int j = 0; j < NUM_TILES_X; ++j) {
            gameState->tiles[i][j].isMine = false;
            gameState->tiles[i][j].adjacentMines = 0;
            gameState->tiles[i][j].revealed = false;
        }
    }
}

/**
 * Places mines randomly around the board for the given GameState.
 * @param gameState GameState struct to place the mines onto/
 */
void placeMines(GameState *gameState) {
    gameState->remainingMines = NUM_MINES;

    for (int i = 0; i < NUM_MINES; ++i) {
        int x, y;
        do {
            x = rand() % NUM_TILES_X;
            y = rand() % NUM_TILES_Y;
        } while (tileContainsMine(gameState, x, y));
        // place mine at x, y
        gameState->tiles[x][y].isMine = true;
    }
}

/**
 * Calculates and sets the number of adjacent mines for each (x, y) in the grid
 * @param gameState pointer to the GameState struct to calculate
 */
void calculateAdjacent(GameState *gameState) {
    // Loop through each tile
    for (int i = 0; i < NUM_TILES_Y; ++i) {
        for (int j = 0; j < NUM_TILES_X; ++j) {
            // Get an array of 8 coordinates of surrounding tiles
            int *adjacentTiles = getAdjacentTiles(i, j);
            int numAdjacent = 0;

            // Since there are maximum of 8 surrounding tiles, iterate through them all
            for (int k = 0; k < 8; ++k) {
                // if the square actually exists (i.e. not outside field of play)
                if (adjacentTiles[k] != -1) {
                    // eg. (3, 3) would be 33
                    // want x = 3, y = 3
                    // get back the x, y grid coordinate
                    int x = adjacentTiles[k] % 10;
                    int y = adjacentTiles[k] / 10;

                    // increment the number of adjacent mines
                    if (gameState->tiles[y][x].isMine) {
                        numAdjacent++;
                    }
                }
            }

            // Set the number of adjacent at this point to the calculated amount
            gameState->tiles[i][j].adjacentMines = numAdjacent;
        }
    }
}

/**
 * Returns an array of 8 integers, indicating the xy location of all surrounding tiles of point (i, j).
 * Eg. if i = 3, j = 3, returns {22, 23, 24, 32, 34, 42, 43, 44}
 * If on the edge of the playing field, returns -1 for all locations outside of game board.
 * @param y y location to check surrounding
 * @param x x location to check surrounding
 * @return array of 8 integers as outlined above.
 */
int *getAdjacentTiles(int y, int x) {
    static int tiles[8] = {-1};

    // top left
    tiles[0] = (y - 1) * 10 + x - 1;
    // top
    tiles[1] = (y - 1) * 10 + x;
    // top right
    tiles[2] = (y - 1) * 10 + x + 1;
    // left
    tiles[3] = (y) * 10 + x - 1;
    // right
    tiles[4] = (y) * 10 + x + 1;
    // bottom left
    tiles[5] = (y + 1) * 10 + x - 1;
    // bottom
    tiles[6] = (y + 1) * 10 + x;
    // bottom right
    tiles[7] = (y + 1) * 10 + x + 1;

    // Left hand border
    if (x == 0) {
        tiles[0] = -1;
        tiles[3] = -1;
        tiles[5] = -1;
    }

    // Right hand border
    if (x == NUM_TILES_X - 1) {
        tiles[2] = -1;
        tiles[4] = -1;
        tiles[7] = -1;
    }

    // Top border
    if (y == 0) {
        tiles[0] = -1;
        tiles[1] = -1;
        tiles[2] = -1;
    }

    // Top border
    if (y == NUM_TILES_Y - 1) {
        tiles[5] = -1;
        tiles[6] = -1;
        tiles[7] = -1;
    }

    return tiles;
}

/**
 * Debug function to show the current state of the game on the server side.
 * @param gameState pointer to GameState struct, containing all game information
 */
void showBoard(GameState *gameState) {
    char *vertical = "ABCDEFGHI";
    printf("Remaining mines: %d\n", 10);
    printf("\n");
    printf("    1 2 3 4 5 6 7 8 9\n");
    printf("  -------------------\n");
    for (int i = 0; i < NUM_TILES_Y; ++i) {
        printf("%c | ", vertical[i]);
        for (int j = 0; j < NUM_TILES_X; ++j) {
            if (tileContainsMine(gameState, i, j)) {
                printf("* ");
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }
    printf("\n");
}

/**
 * Adds the given score to the given player and increments the number of games won.
 * This method inserts the player's score in the correct order (shortest at the end).
 * @param currentPlayer pointer to a player struct
 * @param time time taken to complete the game
 */
void addScore(Player *currentPlayer, int time) {
    // Allocate memory for this new score
    Score *newScore = malloc(sizeof(Score));
    // If the allocation failed, return and deal with this.
    if (newScore == NULL) {
        printf("%s\n", "Error");
        return;
    }

    // Increment number of games won by player
    currentPlayer->gamesWon++;

    // Set all values for this new score
    newScore->name = currentPlayer->name;
    newScore->time = time;
    newScore->next = NULL;

    // Get the current head of the list (longest time taken)
    Score *current = leaderBoard.head;

    // If this is the first score added to the leaderboard
    if (current == NULL) {
        // Set the head of the leaderboard to be this score
        leaderBoard.head = newScore;

        // We are done so exit out
        return;
    }

    // There are currently scores already in this list (and they are sorted by longest time taken to shortest time
    // taken.

    // If the new time is longer than the current (head of the list)
    if (time > current->time) {
        // Set the new score's next to be the current head of the list
        newScore->next = current;

        // Set the head of the leaderboard to be this new score
        leaderBoard.head = newScore;

        // We are done so exit out
        return;
    }

    // Need to find where this new score should be located. Needs to be so that prev_time > time > next_time or end
    // of list.

    // So iterate through list until the next is either null or has a lower time than new score.
    while (current->next != NULL && current->next->time > time) {
        current = current->next;
    }

    // At this point, new score needs to go in between current and current->next

    // Set next of new score to be what is currently next.
    newScore->next = current->next;

    // Set next of current to new score.
    current->next = newScore;
}

/**
 * Returns a pointer to the player struct that has the given name as input. If no player with this name, returns NULL.
 * @param name string that is player's name
 * @return pointer to this player object, or NULL if no player exists with that name.
 */
Player *getPlayer(char *name) {
    // Go through all stored players and if the name matches, return that player
    for (int i = 0; i < MAX_USERS; ++i) {
        if (strncmp(leaderBoard.players[i]->name, name, MAXDATASIZE) == 0) {
            return leaderBoard.players[i];
        }
    }

    // If we couldn't find a player with that name, return NULL
    return NULL;
}

/**
 * The thread to run when recieving a new connection.
 * @param socketID the ID of the socket that the user has connected on
 */
void *runThread(void *arg) {

    int socketID = *(int *)arg;
    // Boolean to keep track of whether this thread should continue to run
    bool running = true;
    // Buffer to store what is received from the client
    char inputBuff[MAXDATASIZE];
    // Buffer to store what is sent to the client
    char outputBuff[MAXDATASIZE];

    // This lets the process know that when the thread dies that it should take care of itself.
    pthread_detach(pthread_self());

    // A check to see if login details were correct
    bool isAuthenticated = false;

    // Get the username and put into inputBuff
    receiveStringAndReply(socketID, inputBuff);

    // Check if a user exists with that name (i.e. if NULL, no player exists with that name)
    Player *curr_player = getPlayer(inputBuff);

    // Get the password and put into inputBuff
    receiveString(socketID, inputBuff);

    // If we found a player with that name AND the password matches, authenticate.
    if (curr_player != NULL && strncmp(curr_player->password, inputBuff, MAXDATASIZE) == 0) {
        isAuthenticated = true;
        // Message that we will send to client is login success
        sprintf(outputBuff, SUCCESS);
    }

    if (!isAuthenticated) {
        // Won't enter the running loop
        running = false;
        // Message that we will send to client is login failure
        sprintf(outputBuff, FAIL);
    }

    // Send the authentication outcome to client
    sendString(socketID, outputBuff);

    // If login successful and haven't terminated connection
    while (running) {
        // Receive input from client and store in inputBuff
        receiveStringAndReply(socketID, inputBuff);

        if (strncmp(inputBuff, START_GAME, MAXDATASIZE) == 0) {
            // If they want to start a new game
            handleGame(socketID, inputBuff, curr_player);
        } else if (strncmp(inputBuff, SHOW_LEADERBOARD, MAXDATASIZE) == 0) {
            // If they want to see the leaderboard
            sendLeaderBoard(socketID);
        } else if (strncmp(inputBuff, QUIT, MAXDATASIZE) == 0) {
            // If they chose to quit
            running = false;
        } else {
            // User has disconnected improperly.
            running = false;
        }
    }

    printf("Socket %d disconnected\n", socketID);
    close(socketID);
    pthread_exit(NULL);
}

/**
 * Sets up the login information for players as read in from Authentication.txt
 * Puts these players into the leaderboard and makes them available to log in as.
 */
void setupPlayers() {
    // Set the head of the score list to be null
    leaderBoard.head = NULL;

    // Get an array of users initialised to empty values
    Player *players = calloc(MAX_USERS, sizeof(Player));

    // Input buffer to read each line from file
    char buff[100] = "";
    // Count of which user we are currently at
    int currentPlayer = 0;
    // Open the file
    FILE *file;
    file = fopen("Authentication.txt", "r");

    // If an error occurred while opening file, error out and exit
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Ignore the first line of the file.
    fgets(buff, 255, file);

    // Read each line
    while ((fgets(buff, 255, file)) != NULL) {
        // String to capture each token
        char *tok;
        // Get first string (username) and copy to player struct
        tok = strtok(buff, "\n\t\r ");
        strcpy(players[currentPlayer].name, tok);

        // Get second string (password) and copy to player struct
        tok = strtok(NULL, "\n\t\r ");
        strcpy(players[currentPlayer].password, tok);

        // Move to next player
        currentPlayer++;
    }

    fclose(file);

    // TODO this could probably be improved but is ok for now
    // Store these players in the leaderboard struct
    for (currentPlayer = 0; currentPlayer < MAX_USERS; ++currentPlayer) {
        leaderBoard.players[currentPlayer] = &players[currentPlayer];
    }
}

void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("Received CTRL-C\n");
        serverRunning = false;
    }

    signal(signo, SIG_DFL);
    raise(signo);
}

int main(int argc, char *argv[]) {
    // Set up SIGINT
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        printf("Won't catch SIGINT\n");
    }

    /* Thread and thread attributes */
    pthread_t *client_thread = malloc(sizeof(pthread_t));

    int sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
    struct sockaddr_in my_addr;    /* my address information */
    struct sockaddr_in their_addr; /* connector's address information */
    socklen_t sin_size;
    int portNumber = 12345; // Default port number

    if (argc == 2) {
        portNumber = atoi(argv[1]);
    }

    /* Get port number for server to listen on */
    if (argc > 2) {
        fprintf(stderr, "usage: ./server port_number\n");
        exit(1);
    }

    /* generate the socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    printf("Attempting to open socket on port %d.\n", portNumber);

    /* generate the end point */
    my_addr.sin_family = AF_INET;         /* host byte order */
    my_addr.sin_port = htons(portNumber);     /* short, network byte order */
    my_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */
    /* bzero(&(my_addr.sin_zero), 8);   ZJL*/     /* zero the rest of the struct */

    /* bind the socket to the end point */
    if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    /* start listnening */
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Setting up leaderboard.\n");

    setupPlayers();

    printf("Seeding\n");

    // Set the seed
    srand(RANDOM_NUMBER_SEED);

    printf("server starts listening ...\n");

    serverRunning = true;

    /* repeat: accept, send, close the connection */
    /* for every accepted connection, use a separate process or thread to serve it */
    while (serverRunning) {  /* main accept() loop */
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size)) == -1) {
            perror("accept");
            continue;
        }
        printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));

        // Create a thread to accept client
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(client_thread, &attr, runThread, (void *)&new_fd);
        pthread_join(*client_thread, NULL);
    }

    printf("Exited cleanly\n");

    return 0;
}
