#ifndef SERVER_H
#define SERVER_H

#include "constants.h"

#define MAX_USERS 10

Player *getPlayer(char *name);

char *receiveString(int socket_id, char *buf);

void sendString(int socket_id, char *message);

char *receiveStringAndReply(int socket_id, char *buf);

bool tileContainsMine(GameState *gameState, int x, int y);

void placeMines(GameState *gameState);

void addScore(Player *currentPlayer, int time);

void showBoard(GameState *gameState);

void setupPlayers();

GameState *setupGame();

void clearGame(GameState *gameState);

void calculateAdjacent(GameState *gameState);

int* getAdjacentTiles(int y, int x);

#endif

