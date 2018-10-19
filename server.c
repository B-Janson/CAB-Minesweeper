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
    for (int i = 0; i < 1; ++i) {
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
}

char *receiveStringAndReply(int socket_id, char *buf) {
    receiveString(socket_id, buf);

    sendString(socket_id, "Received");

    return buf;
}

void getSurrounding(GameState *gameState, int surrounding[9][9], int x, int y) {

    if (x < 0 || x >= NUM_TILES_X || y < 0 || y >= NUM_TILES_Y) {
        return;
    }

    if (gameState->tiles[y][x].revealed) {
        return;
    }

    surrounding[y][x] = gameState->tiles[y][x].adjacentMines;
    gameState->tiles[y][x].revealed = true;

    printf("x:%d y:%d adjacent:%d\n", x, y, surrounding[y][x]);

    if (surrounding[y][x] == 0) {
        getSurrounding(gameState, surrounding, x - 1, y - 1);
        getSurrounding(gameState, surrounding, x, y - 1);
        getSurrounding(gameState, surrounding, x + 1, y - 1);
        getSurrounding(gameState, surrounding, x - 1, y);
        getSurrounding(gameState, surrounding, x + 1, y);
        getSurrounding(gameState, surrounding, x - 1, y + 1);
        getSurrounding(gameState, surrounding, x, y + 1);
        getSurrounding(gameState, surrounding, x + 1, y + 1);
    }
}

void handleGame(int socketID, char inputBuff[]) {
    printf("User has chosen to play a game.\n");
    GameState *gameState = setupGame();
    bool playing = true;

    while (playing) {
//        receiveStringAndReply(socketID, inputBuff);
        receiveString(socketID, inputBuff);


        printf("User entered: %s\n", inputBuff);

        printf("inputBUff[0] == %c\n", inputBuff[0]);
        printf("inputBUff[1] == %c\n", inputBuff[1]);
        printf("inputBuff[2] == %c\n", inputBuff[2]);


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
                printf("y:%d x:%d %d\n", y, x, gameState->tiles[y][x].adjacentMines);
                sprintf(inputBuff, "%d", gameState->tiles[y][x].adjacentMines);
            }

            sendString(socketID, inputBuff);


            int surrounding[9][9] = {{0}};

            getSurrounding(gameState, surrounding, x, y);

            for (int i = 0; i < 9; ++i) {
                for (int j = 0; j < 9; ++j) {
                    printf("%d ", surrounding[i][j]);
                }
                printf("\n");
            }





//            for (int i = 0; i < NUM_TILES_X; ++i) {
//                printf("%d ", gameState->tiles[0][i].adjacentMines);
//            }

            printf("\n");

        } else if (inputBuff[2] == 'P') {
            printf("User wants to palce flag at %c%c", inputBuff[0], inputBuff[1]);
        } else {
            printf("This should not happen.\n");
        }
    }

    free(gameState);
}

void sendLeaderBoard() {
    printf("User wants to see leaderboard\n");
}

int *getAdjacentTiles(GameState *gameState, int i, int j) {
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
    show_board(gameState);

    // loop to show the number of mines at a location
    char *vertical = "ABCDEFGHI";
    printf("\n");
    printf("    1 2 3 4 5 6 7 8 9\n");
    printf("  -------------------\n");
    for (int i = 0; i < NUM_TILES_Y; ++i) {
        printf("%c | ", vertical[i]);
        for (int j = 0; j < NUM_TILES_X; ++j) {
            // this gets a list of coordinates of surrounding squares
            int *adjacentTiles = getAdjacentTiles(gameState, i, j);
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
//            printf("i:%d j:%d\n", i, j);
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

    // leaderBoard.head = malloc(sizeof(score_t));
    // if (leaderBoard.head == NULL)
    // {
    // 	return 1;
    // }

    // leaderBoard.head->name = "Maolin";
    // leaderBoard.head->time = 20;
    // leaderBoard.head->next = NULL;



    // add_score("Maolin", 20);
    // add_score("Maolin", 25);
    // add_score("Bob", 17);
    // add_score("Tom", 18);
    // add_score("James", 99);
    // show_leaderboard();
    // printf("%s\n", "Hello");



    // for (int i = 0; i < 10; ++i) {
    // 	if (strncmp(names[i], curr_player->name, 50) == 0 && strncmp(passwords[i], curr_player->pass, 50) == 0) {
    // 		isAuthenticated = true;
    // 		printf("User isAuthenticated\n");
    // 		break;
    // 	}
    // }

    addScore("Maolin", 25);

    while (running) {
        receiveStringAndReply(socketID, inputBuff);

        if (strncmp(inputBuff, START_GAME, MAXDATASIZE) == 0) {
            handleGame(socketID, inputBuff);
        } else if (strncmp(inputBuff, SHOW_LEADERBOARD, MAXDATASIZE) == 0) {
            sendLeaderBoard();
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

void show_board(GameState *gameState) {
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
            // printf("%d ", gameState.tiles[i][j].isMine);
        }
        printf("\n");
    }
    printf("\n");
}


void addScore(char *name, int time) {
    Player *curr_player = getPlayer(name);

    curr_player->gamesWon++;
    curr_player->gamesPlayed++;


    if (leaderBoard.head == NULL) {
        leaderBoard.head = malloc(sizeof(Score));
        if (leaderBoard.head == NULL) {
            printf("%s\n", "Error");
            return;
        }
        leaderBoard.head->name = name;
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
        leaderBoard.head->name = name;
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
    current->next->name = name;
    current->next->time = time;
    current->next->next = next;
}

void show_leaderboard() {
    Score *current = leaderBoard.head;

    printf("=======================================================\n");

    while (current != NULL) {
        Player *curr_player = getPlayer(current->name);
        printf("%s\t\t%d seconds\t\t%d games won, %d games played\n", current->name, current->time,
               curr_player->gamesWon, curr_player->gamesPlayed);
        current = current->next;
    }

    printf("=======================================================\n");

}


void setup_players() {
    leaderBoard.head = NULL;

    leaderBoard.players[0] = malloc(sizeof(Player));
    leaderBoard.players[0]->name = "Maolin";
    leaderBoard.players[0]->password = "111111";
    leaderBoard.players[0]->gamesWon = 0;
    leaderBoard.players[0]->gamesPlayed = 0;

    return;



    // FILE *fp;
    // char buff[255];

    // fp = fopen("Authentication.txt", "r");
    // fscanf(fp, "%s", buff);
    //   	printf("TEST %s\n", buff);
    //   	fscanf(fp, "%s", buff);
    //   	printf("TEST %s\n", buff);

    //   	for (int i = 0; i < 10; ++i)
    //   	{
    //   		char nameBuff[255];
    //   		char passwordBuff[255];
    //   		fscanf(fp, "%s", nameBuff);

    //   		// char blah = nameBuff;




    //   		// printf("1: %s\n", name);
    //   		leaderBoard.players[i] = malloc(sizeof(Player));
    //   		// fgets(nameBuff, 255, stdin);
    //   		// strcpy(leaderBoard.players[i]->name, nameBuff);
    //   		leaderBoard.players[i]->name = &nameBuff;

    //   		fscanf(fp, "%s", passwordBuff);
    //   		// printf("2: %s\n", passwordBuff);
    // 	leaderBoard.players[i]->password = passwordBuff;
    // 	leaderBoard.players[i]->gamesWon = 0;
    // 	leaderBoard.players[i]->gamesPlayed = 0;

    // 	printf("%s\n", leaderBoard.players[i]->name);
    // 	printf("%s\n", leaderBoard.players[i]->password);
    //   	}

    //   	printf("%s\n", leaderBoard.players[0]->name);

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

    setup_players();

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


