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

GameState *gameState;

void sendString(int socketID, char *message) {
    if (send(socketID, message, MAXDATASIZE, 0) == -1) {
        perror("send");
    }
}

void receiveString(int socketID, char *output) {
    int number_of_bytes = 0;

    if ((number_of_bytes = recv(socketID, output, MAXDATASIZE, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    output[number_of_bytes] = '\0';

    //printf("Received: '%s'\n", output);
}

void sendStringAndReceive(int socketID, char *message, char *outputBuf) {
    // send message to server
    sendString(socketID, message);

    // Receive message back from server
    receiveString(socketID, outputBuf);
}

void showBoard() {
    char *vertical = "ABCDEFGHI";
    printf("Remaining mines: %d\n", gameState->remainingMines);
    printf("\n");
    printf("    1 2 3 4 5 6 7 8 9\n");
    printf("  -------------------\n");
    for (int i = 0; i < NUM_TILES_Y; ++i) {
        printf("%c | ", vertical[i]);
        for (int j = 0; j < NUM_TILES_X; ++j) {
            if (tileContainsMine(i, j)) {
                printf("* ");
            } else if (gameState->tiles[i][j].adjacentMines != -1) {
                printf("%d ", gameState->tiles[i][j].adjacentMines);
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }
    printf("\n");
}

bool tileContainsMine(int x, int y) {
    return gameState->tiles[x][y].isMine;
}

int setupConnection(int argc, char *argv[]) {
    int socketID;
    struct hostent *he;
    struct sockaddr_in their_addr; /* connector's address information */

    if (argc != 3) {
        fprintf(stderr, "usage: server_ip_address port_number\n");
        exit(1);
    }

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

bool handleLogin(int socketID, char inputBuff[], char outputBuff[]) {
    printf("============================================\n");
    printf("Welcome to the online Minesweeper gaming system\n");
    printf("============================================\n\n");

    printf("You are required to log on with your registered user name and password.\n\n");
    printf("Username: ");
    scanf("%s", inputBuff);
    sendStringAndReceive(socketID, inputBuff, outputBuff);

    printf("Password: ");
    scanf("%s", inputBuff);
    sendStringAndReceive(socketID, inputBuff, outputBuff);

    // if server has not authenticated user
    if (strncmp(outputBuff, "0", 10) == 0) {
        return false;
    }

    return true;
}

void startGame(int socketID, char inputBuff[], char outputBuff[]) {
    printf("You have chosen to play.\n");
    sendStringAndReceive(socketID, START_GAME, outputBuff);
    setupGame();
    bool playing = true;

    while (playing) {
        showBoard();
        printf("Choose an option:\n");
        printf("<R> Reveal tile\n");
        printf("<P> Place flag\n");
        printf("<Q> Quit game\n\n");
        printf("Option (R,P,Q): ");
        scanf("%s", inputBuff);

        if (strncmp(inputBuff, "R", MAXDATASIZE) == 0) {
            // User wants to reveal tile

            printf("Enter tile coordinates: ");
            scanf("%s", inputBuff);
            int length = strnlen(inputBuff, MAXDATASIZE);
            if (length != 2) {
                perror("Coordinates must be two characters i.e. B2");
            } else {
                inputBuff[2] = 'R';
                inputBuff[3] = '\0';
                sendStringAndReceive(socketID, inputBuff, outputBuff);

                int y = inputBuff[0] - 65;
                int x = inputBuff[1] - 49;

                if (strncmp(outputBuff, "MINE", MAXDATASIZE) == 0) {
                    printf("HIT A MINE\n");
                    gameState->tiles[y][x].isMine = true;
                    playing = false;
                } else {
                    int numAdjacent = atoi(&outputBuff[0]);
                    printf("%d %d %d \n", x, y, numAdjacent);
                    gameState->tiles[y][x].adjacentMines = numAdjacent;
                }


            }
        } else if (strncmp(inputBuff, "P", MAXDATASIZE) == 0) {
            // User wants to place a flag

            printf("Enter tile coordinates: ");
            scanf("%s", inputBuff);
            int length = strnlen(inputBuff, MAXDATASIZE);
            if (length != 2) {
                perror("Coordinates must be two characters i.e. B2");
            } else {
                inputBuff[2] = 'P';
                inputBuff[3] = '\0';
                sendString(socketID, inputBuff);
            }
        } else if (strncmp(inputBuff, "Q", MAXDATASIZE) == 0) {
            playing = false;
            sendStringAndReceive(socketID, "Q", outputBuff);
        } else {
            // User has chosen an invalid option
            printf("That is not a valid option.\n");
        }
    }

    free(gameState);
}

void setupGame() {
    // Allocate memory
    gameState = malloc(sizeof(GameState));
    // Set remaining mines to be initial number of mines
    gameState->remainingMines = NUM_MINES;

    for (int i = 0; i < NUM_TILES_Y; ++i) {
        for (int j = 0; j < NUM_TILES_X; ++j) {
            gameState->tiles[i][j].adjacentMines = -1;
        }
    }
}

void viewLeaderBoard(int socketID, char inputBuff[], char outputBuff[]) {
    printf("You have chosen to view Leaderboard.\n");
    printf("=======================================================\n");
    sendStringAndReceive(socketID, SHOW_LEADERBOARD, outputBuff);

    while(strncmp(outputBuff, "-1", MAXDATASIZE) != 0) {
        receiveString(socketID, outputBuff);
        if (strncmp(outputBuff, "-1", MAXDATASIZE) != 0) {
            printf("%s", outputBuff);
        }
    }

    printf("=======================================================\n");

}


int main(int argc, char *argv[]) {
    char inputBuff[MAXDATASIZE];
    char outputBuff[MAXDATASIZE];
    int socketID = setupConnection(argc, argv);
    bool running = true;

    bool authenticated = handleLogin(socketID, inputBuff, outputBuff);

    if (!authenticated) {
        printf("You entered either an incorrect username or password. Disconnecting.\n");
        close(socketID);
        return 0;
    }

    while (running) {
        printf("Welcome to the Minesweeper gaming system.\n\n");
        printf("Please enter a selection:\n");
        printf("<1> Play Minesweeper\n");
        printf("<2> Show Leaderboard\n");
        printf("<3> Quit\n\n");
        printf("Selection option (1-3): ");
        scanf("%s", inputBuff);
        printf("\n");

        // User selected to play a game
        if (strncmp(inputBuff, "1", 10) == 0) {
            startGame(socketID, inputBuff, outputBuff);
        } else if (strncmp(inputBuff, "2", 10) == 0) {
            viewLeaderBoard(socketID, inputBuff, outputBuff);
        } else if (strncmp(inputBuff, "3", 10) == 0) {
            printf("You have chosen to quit.\n");
            sendStringAndReceive(socketID, QUIT, outputBuff);
            running = false;
        } else {
            printf("You have chosen an invalid option.\n");
        }
    }

    close(socketID);

    return 0;
}
