#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdbool.h>

#define MAXDATASIZE 50 // max number of bytes we can get at once
#define ARRAY_SIZE 10 // size of the array to send
#define BACKLOG 10     // how many pending connections queue will hold
#define RETURNED_ERROR -1 // value sent if error occurred
#define TERMINATE_CONNECTION 65535 // value sent/received if the connection should be terminated
#define RANDOM_NUMBER_SEED 42 // seed value for the rng
#define NUM_TILES_X 9
#define NUM_TILES_Y 9
#define NUM_MINES 10
#define START_GAME "game"
#define SHOW_LEADERBOARD "leaderboard"
#define QUIT "quit"


typedef struct Tile {
    int adjacentMines;
    bool revealed;
    bool isMine;
    bool isFlag;
} Tile;

typedef struct GameState {
    Tile tiles[NUM_TILES_X][NUM_TILES_Y];
    int remainingMines;
} GameState;

typedef struct Score {
    char *name;
    int time;
    struct Score *next;
} Score;

typedef struct Player {
    char name[20];
    char password[20];
    int gamesPlayed;
    int gamesWon;
} Player;

typedef struct LeaderBoard {
    Score *head;
    Player *players[10];
} LeaderBoard;

#endif
