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
#include "server.h"

#include "constants.h"

LeaderBoard leaderBoard;

Player *getPlayer(char *name) {
    for (int i = 0; i < MAX_USERS; ++i) {
        if (strncmp(leaderBoard.players[i]->name, name, MAXDATASIZE) == 0) {
            return leaderBoard.players[i];
        }
    }
    return NULL;
}

char *receiveString(int socket_id, char *buf) {
    int number_of_bytes = 0;

    if ((number_of_bytes = recv(socket_id, buf, MAXDATASIZE, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[number_of_bytes] = '\0';

    return buf;
}

void sendString(int socket_id, char *message) {
    if (send(socket_id, message, MAXDATASIZE, 0) == -1) {
        perror("send");
    }
    //printf("%s", message);
}

char *receiveStringAndReply(int socket_id, char *buf) {
    receiveString(socket_id, buf);

    sendString(socket_id, "Received");

    return buf;
}

void getSurrounding(GameState *gameState, int surrounding[9][9], int x, int y, int socketID, char *inputBuff) {
    if (x < 0 || x >= NUM_TILES_X || y < 0 || y >= NUM_TILES_Y) {
        return;
    }

    if (gameState->tiles[y][x].revealed) {
        return;
    }

    surrounding[y][x] = gameState->tiles[y][x].adjacentMines;
    gameState->tiles[y][x].revealed = true;

//    printf("%d%d%d\n", x, y, surrounding[y][x]);
    sprintf(inputBuff, "%d%d%d", x, y, surrounding[y][x]);

    sendString(socketID, inputBuff);

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

void handleGame(int socketID, char inputBuff[], Player *curr_player) {
    printf("User %s has chosen to play a game.\n", curr_player->name);
    GameState *gameState = setupGame();
    bool playing = true;
    time_t startTime;
    time_t endTime;
    time_t totalTime;
    startTime = time(NULL);
    curr_player->gamesPlayed++;

    while (playing) {
        receiveString(socketID, inputBuff);

        printf("User entered: %s\n", inputBuff);

        if (strncmp(inputBuff, "Q", MAXDATASIZE) == 0) {
            playing = false;
            printf("Player wants to exit game\n");
        } else if (inputBuff[2] == 'R') {
            int y = inputBuff[0] - 65;
            int x = inputBuff[1] - 49;
            printf("User wants to reveal tile %c%c %d\n", y, x, gameState->tiles[y][x].isMine);

            if (tileContainsMine(gameState, y, x)) {
                printf("Mine hit at %d %d\n", x, y);
                sprintf(inputBuff, "MINE");
                playing = false;
            } else {
                int surrounding[9][9] = {{0}};
                getSurrounding(gameState, surrounding, x, y, socketID, inputBuff);
                printf("y:%d x:%d %d\n", y, x, gameState->tiles[y][x].adjacentMines);
                sprintf(inputBuff, "-1");
            }

            sendString(socketID, inputBuff);

            printf("\n");

        } else if (inputBuff[2] == 'P') {
            int y = inputBuff[0] - 65;
            int x = inputBuff[1] - 49;
            printf("User wants to place flag at %d%d %d\n", y, x, gameState->tiles[y][x].isMine);

            if (tileContainsMine(gameState, y, x) && !gameState->tiles[y][x].revealed) {
                printf("Mine covered at %d %d\n", x, y);
                gameState->remainingMines--;
                gameState->tiles[y][x].revealed = true;
                if (gameState->remainingMines == 0) {
                    sprintf(inputBuff, "WON");
                    playing = false;
                    endTime = time(NULL);
                    totalTime = endTime - startTime;
                    addScore(curr_player, totalTime);
                } else {
                    sprintf(inputBuff, "MINE");
                }
            } else {
                printf("Not mine hit at %d %d\n", x, y);
                sprintf(inputBuff, "NONE");
            }

            sendString(socketID, inputBuff);
        } else {
            printf("This should not happen.\n");
        }
    }

    free(gameState);
}

void sendLeaderBoard(int socketID) {
    Score *current = leaderBoard.head;

    while (current != NULL) {
        Player *curr_player = getPlayer(current->name);
        char inputBuff[MAXDATASIZE];
        sprintf(inputBuff, "%s\t\t%d seconds\t\t%d games won, %d games played\n", current->name, current->time,
                curr_player->gamesWon, curr_player->gamesPlayed);
        sendString(socketID, inputBuff);
        current = current->next;
    }
    sendString(socketID, "-1");
}

int *getAdjacentTiles(int i, int j) {
    static int tiles[8] = {-1};

    // top left
    tiles[0] = (i - 1) * 10 + j - 1;
    // top
    tiles[1] = (i - 1) * 10 + j;
    // top right
    tiles[2] = (i - 1) * 10 + j + 1;
    // left
    tiles[3] = (i) * 10 + j - 1;
    // right
    tiles[4] = (i) * 10 + j + 1;
    // bottom left
    tiles[5] = (i + 1) * 10 + j - 1;
    // bottom
    tiles[6] = (i + 1) * 10 + j;
    // bottom right
    tiles[7] = (i + 1) * 10 + j + 1;

    // Left hand border
    if (j == 0) {
        tiles[0] = -1;
        tiles[3] = -1;
        tiles[5] = -1;
    }

    // Right hand border
    if (j == NUM_TILES_X - 1) {
        tiles[2] = -1;
        tiles[4] = -1;
        tiles[7] = -1;
    }

    // Top border
    if (i == 0) {
        tiles[0] = -1;
        tiles[1] = -1;
        tiles[2] = -1;
    }

    // Top border
    if (i == NUM_TILES_Y - 1) {
        tiles[5] = -1;
        tiles[6] = -1;
        tiles[7] = -1;
    }

    return tiles;
}

GameState *setupGame() {
    // allocate memory for the game state
    GameState *gameState = malloc(sizeof(GameState));
    // place mines randomly
    placeMines(gameState);

    // show the mines on the field
    showBoard(gameState);

    // loop to show the number of mines at a location
    char *vertical = "ABCDEFGHI";
    printf("\n");
    printf("    1 2 3 4 5 6 7 8 9\n");
    printf("  -------------------\n");
    for (int i = 0; i < NUM_TILES_Y; ++i) {
        printf("%c | ", vertical[i]);
        for (int j = 0; j < NUM_TILES_X; ++j) {
            // this gets a list of coordinates of surrounding squares
            int *adjacentTiles = getAdjacentTiles(i, j);
            int numAdjacent = 0;
            for (int k = 0; k < 8; ++k) {
                // if the square actually exists (i.e. not outside field of play)
                if (adjacentTiles[k] != -1) {
                    // get back the x, y grid coordinate
                    int x = adjacentTiles[k] % 10;
                    int y = adjacentTiles[k] / 10;

                    // increment the number of adjacent mines
                    if (gameState->tiles[y][x].isMine) {
                        numAdjacent++;
                    }
                }
            }
            gameState->tiles[i][j].adjacentMines = numAdjacent;
            printf("%d ", numAdjacent);
        }
        printf("\n");
    }
    printf("\n");

    gameState->remainingMines = NUM_MINES;

    return gameState;
}

void Run_Thread(int socketID) {
    bool running = true;
    char inputBuff[MAXDATASIZE];
    char *outputBuff = "";

    // This lets the process know that when the thread dies that it should take care of itself.
    pthread_detach(pthread_self());

    // Set the seed
    srand(RANDOM_NUMBER_SEED);

    bool isAuthenticated = false;

    // Get the username
    receiveStringAndReply(socketID, inputBuff);

    // Check if a user exists with that name
    Player *curr_player = getPlayer(inputBuff);

    // Get the password
    receiveString(socketID, inputBuff);

    // If we found a player with that name AND the password matches, authenticate.
    if (curr_player != NULL && strncmp(curr_player->password, inputBuff, MAXDATASIZE) == 0) {
        isAuthenticated = true;
        outputBuff = "1"; // message of 1 means that it was a successful login
        printf("User is authenticated\n");
    }

    if (!isAuthenticated) {
        running = false;
        outputBuff = "0";
        printf("Player logged in with wrong credentials.\n");
    }

    sendString(socketID, outputBuff);


    while (running) {
        receiveStringAndReply(socketID, inputBuff);

        if (strncmp(inputBuff, START_GAME, MAXDATASIZE) == 0) {
            handleGame(socketID, inputBuff, curr_player);
        } else if (strncmp(inputBuff, SHOW_LEADERBOARD, MAXDATASIZE) == 0) {
            sendLeaderBoard(socketID);
        } else if (strncmp(inputBuff, QUIT, MAXDATASIZE) == 0) {
            running = false;
        } else {
            printf("Something has gone wrong.\n");
        }

        printf("Received: %s\n", inputBuff);
    }

    printf("Socket %d disconnected\n", socketID);
    close(socketID);
    pthread_exit(NULL);
}


bool tileContainsMine(GameState *gameState, int x, int y) {
    return gameState->tiles[x][y].isMine;
}


void placeMines(GameState *gameState) {
    for (int i = 0; i < NUM_TILES_Y; ++i) {
        for (int j = 0; j < NUM_TILES_X; ++j) {
            gameState->tiles[i][j].isMine = false;
            gameState->tiles[i][j].adjacentMines = 0;
            gameState->tiles[i][j].revealed = false;
        }
    }

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


void addScore(Player *currentPlayer, int time) {

    currentPlayer->gamesWon++;

    if (leaderBoard.head == NULL) {
        leaderBoard.head = malloc(sizeof(Score));
        if (leaderBoard.head == NULL) {
            printf("%s\n", "Error");
            return;
        }
        leaderBoard.head->name = currentPlayer->name;
        leaderBoard.head->time = time;
        leaderBoard.head->next = NULL;

        return;
    }

    Score *current = leaderBoard.head;

    if (time > current->time) {
        leaderBoard.head = malloc(sizeof(Score));
        if (leaderBoard.head == NULL) {
            printf("%s\n", "Error");
            return;
        }
        leaderBoard.head->name = currentPlayer->name;
        leaderBoard.head->time = time;
        leaderBoard.head->next = current;
        return;
    }

    while (current->next != NULL && current->next->time > time) {
        current = current->next;
    }

    Score *next = current->next;

    current->next = malloc(sizeof(Score));
    if (leaderBoard.head == NULL) {
        printf("%s\n", "Error");
        return;
    }
    current->next->name = currentPlayer->name;
    current->next->time = time;
    current->next->next = next;
}

//void show_leaderboard() {
//    Score *current = leaderBoard.head;
//
//    printf("=======================================================\n");
//
//    while (current != NULL) {
//        Player *curr_player = getPlayer(current->name);
//        printf("%s\t\t%d seconds\t\t%d games won, %d games played\n", current->name, current->time,
//               curr_player->gamesWon, curr_player->gamesPlayed);
//        current = current->next;
//    }
//
//    printf("=======================================================\n");
//
//}


void setupPlayers() {
    leaderBoard.head = NULL;

    Player *players = calloc(MAX_USERS, sizeof(Player));

    char buff[100] = "";
    int i = 0;
    FILE *file;
    file = fopen("Authentication.txt", "r");

    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Ignore the first line of the file.
    fgets(buff, 255, file);

    while ((fgets(buff, 255, file)) != NULL) {
        char *tok;
        tok = strtok(buff, "\n\t\r ");
        strcpy(players[i].name, tok);

        tok = strtok(NULL, "\n\t\r ");
        strcpy(players[i].password, tok);

        i++;
    }

    fclose(file);

    for (i = 0; i < MAX_USERS; ++i) {
        leaderBoard.players[i] = &players[i];
    }
}

int main(int argc, char *argv[]) {
    /* Thread and thread attributes */
    pthread_t client_thread;

    int sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
    struct sockaddr_in my_addr;    /* my address information */
    struct sockaddr_in their_addr; /* connector's address information */
    socklen_t sin_size;

    /* Get port number for server to listen on */
    if (argc != 2) {
        fprintf(stderr, "usage: client port_number\n");
        exit(1);
    }

    /* generate the socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    /* generate the end point */
    my_addr.sin_family = AF_INET;         /* host byte order */
    my_addr.sin_port = htons(atoi(argv[1]));     /* short, network byte order */
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

    printf("server starts listening ...\n");

    /* repeat: accept, send, close the connection */
    /* for every accepted connection, use a sepetate process or thread to serve it */
    while (1) {  /* main accept() loop */
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size)) == -1) {
            perror("accept");
            continue;
        }
        printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));

        // Create a thread to accept client
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&client_thread, &attr, Run_Thread, new_fd);
        pthread_join(client_thread, NULL);
    }

    close(new_fd);

    return 0;
}


