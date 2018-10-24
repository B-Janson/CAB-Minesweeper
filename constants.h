#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdbool.h>

#define MAXDATASIZE 50                  // max number of bytes we can get at once
#define BACKLOG 10                      // how many pending connections queue will hold
#define RETURNED_ERROR -1               // value sent if error occurred
#define TERMINATE_CONNECTION 65535      // value sent/received if the connection should be terminated
#define RANDOM_NUMBER_SEED 42           // seed value for the rng
#define NUM_TILES_X 9                   // number of tiles across
#define NUM_TILES_Y 9                   // number of tiles up
#define NUM_MINES 10                    // number of mines to fill the board with
#define START_GAME "game"               // message to send/receive when wanting to start a game
#define SHOW_LEADERBOARD "leaderboard"  // message to send/receive when wanting to see the leaderboard
#define QUIT "quit"                     // message to send/receive when wanting to kill the connection
#define Y_OFFSET 'A'                    // since user send 'A' for vertical coordinate, need to return this to int
#define X_OFFSET '1'                    // since user send '1' for horizontal coordinate, need to return this to int
#define SUCCESS "1"                     // login was a success
#define FAIL "0"                        // login was failed
#define WIN_MESSAGE "WON"               // message to send/receive when user wins a game
#define MINE_MESSAGE "MINE"             // message to send/receive when user hits a mine
#define NO_MINE_MESSAGE "NONE"          // message to send/receive when no mine was hit after flagging a tile
#define END_OF_MESSAGE "-1"             // message to send/receive when transmission finished

/**
 * Structure of a tile
 */
typedef struct Tile {
    int adjacentMines; // number of mines near this tile
    bool revealed; // whether user has revealed this tile
    bool isMine; // whether this tile is a mine
} Tile;

/**
 * Structure of a Game
 */
typedef struct GameState {
    Tile tiles[NUM_TILES_X][NUM_TILES_Y]; // array of tiles
    int remainingMines; // number of mines still to flag
} GameState;

/**
 * Score is an entry in the leaderboard.
 */
typedef struct Score {
    char *name; // name of the player
    int time; // score of the player
    struct Score *next; // next score in the list
} Score;

/**
 * Structure to store player information
 */
typedef struct Player {
    char name[20]; // name of the player
    char password[20]; // password of the player
    int gamesPlayed; // games played by this player
    int gamesWon; // games won by this player
} Player;

/**
 * Structure to store leader board information. Implemented as linked list of Scores.
 */
typedef struct LeaderBoard {
    Score *head; // head of the list
    Player *players[10]; // array of all possible players
} LeaderBoard;

#endif
