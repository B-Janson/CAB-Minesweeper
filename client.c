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
}

void sendStringAndReceive(int socketID, char *message, char *outputBuf) {
    // send message to server
    sendString(socketID, message);

    // Receive message back from server
    receiveString(socketID, outputBuf);
}

void showBoard() {
    char *vertical = "ABCDEFGHI";
    printf("Remaining mines: %d\n", 10);
    printf("\n");
    printf("    1 2 3 4 5 6 7 8 9\n");
    printf("  -------------------\n");
    for (int i = 0; i < NUM_TILES_Y; ++i) {
        printf("%c | ", vertical[i]);
        for (int j = 0; j < NUM_TILES_X; ++j) {
            if (tile_contains_mine(gameState, i, j)) {
                printf("* ");
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }
    printf("\n");
}

bool tile_contains_mine(GameState *gameState, int x, int y) {
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


int main(int argc, char *argv[]) {
    char inputBuff[MAXDATASIZE];
    char outputBuf[MAXDATASIZE];
    int socketID = setupConnection(argc, argv);
    gameState = malloc(sizeof(GameState));
    bool running = true;

    printf("============================================\n");
    printf("Welcome to the online Minesweeper gaming system\n");
    printf("============================================\n\n");

    printf("You are required to log on with your registered user name and password.\n\n");
    printf("Username: ");
    scanf("%s", inputBuff);
    sendStringAndReceive(socketID, inputBuff, outputBuf);

    printf("Password: ");
    scanf("%s", inputBuff);
    sendStringAndReceive(socketID, inputBuff, outputBuf);

    // if server has not authenticated user
    if (strncmp(outputBuf, "0", 10) == 0) {
        printf("You entered either an incorrect username or password. Disconnecting.\n");
        close(socketID);
        return 0;
    }

    printf("Welcome to the Minesweeper gaming system.\n\n");
    printf("Please enter a selection:\n");
    printf("<1> Play Minesweeper\n");
    printf("<2> Show Leaderboard\n");
    printf("<3> Quit\n\n");
    printf("Selection option (1-3): ");
    scanf("%s", inputBuff);
    printf("\n");

    if (strncmp(inputBuff, "1", 10) == 0) {
        printf("You have chosen to play.\n");
        sendStringAndReceive(socketID, "Game", outputBuf);
    }

    if (strncmp(inputBuff, "2", 10) == 0) {
        printf("You have chosen to view Leaderboard.\n");
        sendStringAndReceive(socketID, "LeaderBoard", outputBuf);
    }

    if (strncmp(inputBuff, "3", 10) == 0) {
        printf("You have chosen to quit.\n");
        running = 0;
    }

    while (running) {
        showBoard();
        printf("Choose an option:\n");
        printf("<R> Reveal tile\n");
        printf("<P> Place flag\n");
        printf("<Q> Quit game\n\n");
        printf("Option (R,P,Q): ");
        scanf("%s", inputBuff);

        if (strncmp(inputBuff, "R", MAXDATASIZE) == 0) {
            printf("Enter tile coordinates: ");
            scanf("%s", inputBuff);
            int length = strnlen(inputBuff, MAXDATASIZE);
            if (length != 2) {
                perror("Coordinates must be two characters i.e. B2");
            } else {
                inputBuff[2] = 'R';
                inputBuff[3] = '\0';
                sendString(socketID, inputBuff);
            }
        }

        if (strncmp(inputBuff, "-1", MAXDATASIZE) == 0) {
            running = 0;
        }

        sendStringAndReceive(socketID, inputBuff, outputBuf);

        printf("%s\n", outputBuf);
    }

    close(socketID);

    return 0;
}
