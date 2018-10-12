#ifndef CLIENT_H
#define CLIENT_H

typedef struct Tile
{
    int adjacentMines;
    bool revealed;
    bool isMine;
} Tile;

typedef struct GameState
{
    Tile tiles[NUM_TILES_X][NUM_TILES_Y];
} GameState;

typedef struct Score
{
    char* name;
    int time;
    struct Score *next;
} Score;

typedef struct Player
{
    char* name;
    char* password;
    int gamesPlayed;
    int gamesWon;
} Player;

typedef struct LeaderBoard
{
    Score *head;
    Player *players[10];
} LeaderBoard;
#endif


void place_mines(GameState *gameState);
bool tile_contains_mine(GameState *gamestate, int x, int y);
void showBoard();
void add_score(char *name, int time);
void show_leaderboard();
Player* getPlayer(char* name);
void setup_players();
