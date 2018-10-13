#ifndef CLIENT_H
#define CLIENT_H

#include "constants.h"

void sendString(int socketID, char *message);

void receiveString(int socketID, char *output);

void sendStringAndReceive(int socketID, char *message, char *outputBuf);

void showBoard();

bool tileContainsMine(int x, int y);

int setupConnection(int argc, char *argv[]);

bool handleLogin(int socketID, char inputBuff[], char outputBuff[]);

void startGame(int socketID, char inputBuff[], char outputBuff[]);

void setupGame();

void viewLeaderBoard(int socketID, char inputBuff[], char outputBuff[]);

#endif
