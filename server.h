#ifndef SERVER_H
#define SERVER_H

#include "constants.h"

Player *getPlayer(char *name);

char *receiveString(int socket_id, char *buf);

void sendString(int socket_id, char *message);

char *receiveStringAndReply(int socket_id, char *buf);

bool tileContainsMine(GameState *gameState, int x, int y);

void placeMines(GameState *gameState);

void addScore(char *name, int time);

void show_board(GameState *gameState);

void setup_players();

GameState *setupGame();

#endif

