#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "constants.h"
#include "client.h"

/**
 * Client's copy of the current game state
 */
GameState *gameState;

/**
 * Send message to server
 * @param socketID ID of the socket to communicate over
 * @param message message to send to the server
 */
void sendString(int socketID, char *message) {
    if (send(socketID, message, MAXDATASIZE, 0) == -1) {
        perror("send");
    }
}

/**
 * Receive message from the server
 * @param socketID ID of the socket to communicate over
 * @param output buffer to store what the server has sent
 */
void receiveString(int socketID, char *output) {
    int number_of_bytes = 0;

    if ((number_of_bytes = recv(socketID, output, MAXDATASIZE, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    output[number_of_bytes] = '\0';
}

/**
 * Send a string to the server and get a response
 * @param socketID ID of the socket to communicate over
 * @param message message to send
 * @param outputBuf buffer to store output
 */
void sendStringAndReceive(int socketID, char *message, char *outputBuf) {
    // send message to server
    sendString(socketID, message);

    // Receive message back from server
    receiveString(socketID, outputBuf);
}

/**
 * Displays the current state of the board to the user.
 * Covered mines are labelled with '+', hit mines with '*' and all other tiles are either blank if covered or the
 * number of adjacent mines if revealed.
 */
void showBoard() {
    // The vertical String to access the y coordinate
    char *vertical = "ABCDEFGHI";
    // Display remaining mines
    printf("Remaining mines: %d\n\n", gameState->remainingMines);

    // Display top coordinate
    printf("    1 2 3 4 5 6 7 8 9\n");
    printf("  -------------------\n");
    for (int i = 0; i < NUM_TILES_Y; ++i) {
        // Display the current vertical coordinate
        printf("%c | ", vertical[i]);
        for (int j = 0; j < NUM_TILES_X; ++j) {
            if (tileContainsMine(i, j)) {
                if (gameState->tiles[i][j].revealed) {
                    printf("+ ");
                } else {
                    printf("* ");
                }
            } else if (gameState->tiles[i][j].adjacentMines != -1) {
                printf("%d ", gameState->tiles[i][j].adjacentMines);
            } else {
                printf("  ");
            }
        }
        // Takes to the next horizontal line
        printf("\n");
    }
    printf("\n");
}

/**
 * Checks if a there is a mine at this location.
 * @param x x location to check
 * @param y y location to check
 * @return true if there is a mine at this location
 */
bool tileContainsMine(int x, int y) {
    return gameState->tiles[x][y].isMine;
}

/**
 * Attempts to connect to the given server address and port and returns the ID of this socket
 * @param argv array of arguments given to the program, where address is the second one and port is the third
 * @return ID of the socket for all future communication
 */
int setupConnection(char *argv[]) {
    // initialise socket
    int socketID;
    struct hostent *he;
    // Client's address information
    struct sockaddr_in their_addr;

    if ((he = gethostbyname(argv[1])) == NULL) {  /* get the host info */
        herror("gethostbyname");
        exit(1);
    }

    if ((socketID = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET;      /* host byte order */
    their_addr.sin_port = htons(atoi(argv[2]));    /* short, network byte order */
    their_addr.sin_addr = *((struct in_addr *) he->h_addr);
    bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */

    if (connect(socketID, (struct sockaddr *) &their_addr, sizeof(struct sockaddr)) == -1) {
        printf("FAIL\n");
        perror("connect");
        exit(1);
    }

    return socketID;
}

/**
 * Handle the log in of the user on the initial connection and return whether they have provided correct
 * authentication details.
 * @param socketID ID of the socket to communicate over
 * @param inputBuff buffer to store what is sent to server
 * @param outputBuff buffer to store what is received from server
 * @return
 */
bool handleLogin(int socketID, char inputBuff[], char outputBuff[]) {
    // Show message
    printf("============================================\n");
    printf("Welcome to the online Minesweeper gaming system\n");
    printf("============================================\n\n");

    // Prompt for user name and send to server
    printf("You are required to log on with your registered user name and password.\n\n");
    printf("Username: ");
    scanf("%s", inputBuff);
    sendStringAndReceive(socketID, inputBuff, outputBuff);

    // Prompt for password and send to server
    printf("Password: ");
    scanf("%s", inputBuff);
    sendStringAndReceive(socketID, inputBuff, outputBuff);

    // If server has not authenticated user
    if (strncmp(outputBuff, FAIL, 10) == 0) {
        return false;
    }

    return true;
}

/**
 * Starts the game for the client, handles input and updates the state of the game according to what it receives from
 * the server.
 * @param socketID ID of the socket to communciate over
 * @param inputBuff buffer to store what is to be sent to server
 * @param outputBuff buffer to store what is received from server
 */
void startGame(int socketID, char inputBuff[], char outputBuff[]) {
    printf("You have chosen to play.\n");
    // Tell server that we wish to play a game
    sendStringAndReceive(socketID, START_GAME, outputBuff);
    // Set up the game with initial values
    setupGame();
    // Boolean to maintain if still want to play and haven't lost yet
    bool playing = true;

    while (playing) {
        // Show the current state of the board
        showBoard();
        // Show help on what the options are and get user input
        printf("Choose an option:\n");
        printf("<R> Reveal tile\n");
        printf("<P> Place flag\n");
        printf("<Q> Quit game\n\n");
        printf("Option (R,P,Q): ");
        scanf("%s", inputBuff);

        if (strncmp(inputBuff, "R", MAXDATASIZE) == 0) {
            // User wants to reveal tile

            // Prompt for coordinates to reveal and store in inputBuff
            printf("Enter tile coordinates: ");
            scanf("%s", inputBuff);
            // Check that they have entered exactly 2 characters
            int length = strnlen(inputBuff, MAXDATASIZE);
            if (length != 2) {
                // Show an error and return back to beginning of input loop
                printf("Coordinates must be two characters i.e. B2\n");
            } else {
                // Check if inputs are within game bound
                int y = inputBuff[0] - Y_OFFSET;
                int x = inputBuff[1] - X_OFFSET;

                if (y < 0 || y >= NUM_TILES_Y || x < 0 || x >= NUM_TILES_X) {
                    printf("These coordinates are outside the game grid. Please try again.\n");
                    continue;
                }

                // Append the character 'R' so the server knows they wish to reveal whatever is being sent.
                inputBuff[2] = 'R';
                inputBuff[3] = '\0';
                sendStringAndReceive(socketID, inputBuff, outputBuff);

                // Check what server sent back
                if (strncmp(outputBuff, MINE_MESSAGE, MAXDATASIZE) == 0) {
                    // This tile was a mine, update game state and terminate game
                    printf("You hit a mine! Game Over!\n");

                    while (strncmp(outputBuff, END_OF_MESSAGE, MAXDATASIZE) != 0) {
                        receiveString(socketID, outputBuff);
                        if (strncmp(outputBuff, END_OF_MESSAGE, MAXDATASIZE) != 0) {
                            int i = outputBuff[0] - '0';
                            int j = outputBuff[1] - '0';
                            gameState->tiles[i][j].isMine = true;
                        }
                    }
                    playing = false;
                    showBoard();
                } else {
                    // Didn't hit a mine
                    // Keep listening until the server stops (used for recursive requirements for revealing adjacent
                    // tiles
                    while (strncmp(outputBuff, END_OF_MESSAGE, MAXDATASIZE) != 0) {
                        // TODO this should be cleaned up but is ok for now

                        // Get the x and y location explicitly from the server and update based on what it says
                        int x = outputBuff[0] - '0';
                        int y = outputBuff[1] - '0';
                        int adjacent = outputBuff[2] - '0';

                        gameState->tiles[y][x].adjacentMines = adjacent;

                        // Keep receiving until END_OF_MESSAGE received
                        receiveString(socketID, outputBuff);
                    }
                }
            }
        } else if (strncmp(inputBuff, "P", MAXDATASIZE) == 0) {
            // User wants to place a flag

            // Prompt for coordinates to reveal and store in inputBuff
            printf("Enter tile coordinates: ");
            scanf("%s", inputBuff);
            // Check that they have entered exactly 2 characters
            int length = strnlen(inputBuff, MAXDATASIZE);
            if (length != 2) {
                // Show an error and return back to beginning of input loop
                printf("Coordinates must be two characters i.e. B2\n");
            } else {
                // Check if inputs are within game bound
                int y = inputBuff[0] - Y_OFFSET;
                int x = inputBuff[1] - X_OFFSET;

                if (y < 0 || y >= NUM_TILES_Y || x < 0 || x >= NUM_TILES_X) {
                    printf("These coordinates are outside the game grid. Please try again.\n");
                    continue;
                }

                // Append the character 'P" so the server knows they wish to place a flag at this location
                inputBuff[2] = 'P';
                inputBuff[3] = '\0';

                // Check if they have already placed a mine here
                if (gameState->tiles[y][x].isMine == true) {
                    printf("Already placed flag at this location.\n");
                } else {
                    sendStringAndReceive(socketID, inputBuff, outputBuff);

                    // Check what the server sent back
                    if (strncmp(outputBuff, MINE_MESSAGE, MAXDATASIZE) == 0) {
                        // This tile was a mine, update game state
                        printf("Mine located at this point!\n");
                        gameState->remainingMines--;
                        gameState->tiles[y][x].isMine = true;
                        gameState->tiles[y][x].revealed = true;
                    } else if (strncmp(outputBuff, WIN_MESSAGE, MAXDATASIZE) == 0) {
                        // That was the last remaining mine and the player has won.
                        gameState->remainingMines--;
                        gameState->tiles[y][x].isMine = true;
                        gameState->tiles[y][x].revealed = true;
                        printf("Congratulations you have won!\n");
                        receiveString(socketID, outputBuff);
                        while (strncmp(outputBuff, END_OF_MESSAGE, MAXDATASIZE) != 0) {
                            // Get the x and y location explicitly from the server and update based on what it says
                            int x = outputBuff[0] - '0';
                            int y = outputBuff[1] - '0';
                            int adjacent = outputBuff[2] - '0';

                            gameState->tiles[y][x].adjacentMines = adjacent;

                            // Keep receiving until END_OF_MESSAGE received
                            receiveString(socketID, outputBuff);
                        }
                        showBoard();
                        playing = false;
                    } else {
                        // No mine at this location
                        printf("NO MINE\n");
                    }
                }
            }
        } else if (strncmp(inputBuff, "Q", MAXDATASIZE) == 0) {
            // Player wishes to quit
            playing = false;
            // Tell server that we wish to quit
            sendString(socketID, "Q");
        } else {
            // User has chosen an invalid option
            printf("That is not a valid option.\n");
        }
    }

    // Clear memory used by game state
    free(gameState);
}

/**
 * Sets up the client's copy of the game with initial values
 */
void setupGame() {
    // Allocate memory
    gameState = malloc(sizeof(GameState));
    // Set remaining mines to be initial number of mines
    gameState->remainingMines = NUM_MINES;

    for (int i = 0; i < NUM_TILES_Y; ++i) {
        for (int j = 0; j < NUM_TILES_X; ++j) {
            gameState->tiles[i][j].adjacentMines = -1;
            gameState->tiles[i][j].isMine = false;
            gameState->tiles[i][j].revealed = false;
        }
    }
}

/**
 * Sends request to the server to see the current state of the leaderboard and displays to the user.
 * @param socketID ID of the socket to communicate over
 * @param outputBuff the buffer to store the output from the server
 */
void viewLeaderBoard(int socketID, char outputBuff[]) {
    printf("You have chosen to view the Leaderboard.\n");
    printf("===================================================================\n");
    // Tell server that we want to see the leaderboard
    sendStringAndReceive(socketID, SHOW_LEADERBOARD, outputBuff);

    // TODO potentially clean this up

    // While we haven't received end of communication, receive and display the output
    while(strncmp(outputBuff, END_OF_MESSAGE, MAXDATASIZE) != 0) {
        receiveString(socketID, outputBuff);
        if (strncmp(outputBuff, END_OF_MESSAGE, MAXDATASIZE) != 0) {
            printf("%s", outputBuff);
        }
    }

    printf("===================================================================\n");
}

/**
 * Displays welcome/help information and accepts their choice
 * @param inputBuff
 */
void getSelection(char inputBuff[]) {
    printf("Welcome to the Minesweeper gaming system.\n\n");
    printf("Please enter a selection:\n");
    printf("<1> Play Minesweeper\n");
    printf("<2> Show Leaderboard\n");
    printf("<3> Quit\n\n");
    printf("Selection option (1-3): ");
    scanf("%s", inputBuff);
    printf("\n");
}


int main(int argc, char *argv[]) {
    // Check if correct number of arguments have been given to the program
    if (argc != 3) {
        fprintf(stderr, "usage: server_ip_address port_number\n");
        exit(1);
    }

    // Buffer to store what is sent to server
    char inputBuff[MAXDATASIZE];
    // Buffer to store what is returned from server
    char outputBuff[MAXDATASIZE];

    // Connect to server and return the socketID
    int socketID = setupConnection(argv);
    // Boolean to maintain whether this should keep running
    bool running = true;

    // Handle the login and return true if they are authenticated
    bool authenticated = handleLogin(socketID, inputBuff, outputBuff);

    // If they were not authenticated, tell them, close the connection and exit.
    if (!authenticated) {
        printf("You entered either an incorrect username or password. Disconnecting.\n");
        close(socketID);
        return 0;
    }

    // While they are attempting to use the program
    while (running) {
        // Display a welcome message and get choice of what they want to do
        getSelection(inputBuff);

        if (strncmp(inputBuff, "1", 10) == 0) {
            // User selected to play a game
            startGame(socketID, inputBuff, outputBuff);
        } else if (strncmp(inputBuff, "2", 10) == 0) {
            // User selected to view leaderboard
            viewLeaderBoard(socketID, outputBuff);
        } else if (strncmp(inputBuff, "3", 10) == 0) {
            // User selected to quit
            printf("You have chosen to quit.\n");
            // Tell server that client wishes to quit and close connection
            sendStringAndReceive(socketID, QUIT, outputBuff);
            running = false;
        } else {
            printf("You have chosen an invalid option.\n");
        }
    }

    // Close connection
    close(socketID);
    return 0;
}
