#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "server.h"

#define RANDOM_NUMBER_SEED 42
#define NUM_TILES_X 9
#define NUM_TILES_Y 9
#define NUM_MINES 10



typedef struct
{
	int adjacentMines;
	bool revealed;
	bool isMine;
} Tile;

typedef struct Score
{
	char* name;
	int time;
	struct Score *next;
} score_t;

typedef struct Player
{
	char* name;
	char* password;
	int gamesPlayed;
	int gamesWon;
} player_t;

typedef struct LeaderBoard
{
	score_t *head;
	player_t *players[10];
};

typedef struct GameState
{
	Tile tiles[NUM_TILES_X][NUM_TILES_Y];
};

struct GameState gameState;
struct LeaderBoard leaderBoard;


player_t* getPlayer(char* name) {
	for (int i = 0; i < 10; ++i)
	{
		if (leaderBoard.players[i]->name == name)
		{
			return leaderBoard.players[i];
		}
	}
	return NULL;
}





int main(int argc, char *argv[])
{
	srand(RANDOM_NUMBER_SEED);
	setup_players();
	place_mines();
	show_board();

	leaderBoard.head = NULL;
	// leaderBoard.head = malloc(sizeof(score_t));
	// if (leaderBoard.head == NULL)
	// {
	// 	return 1;
	// }

	// leaderBoard.head->name = "Maolin";
	// leaderBoard.head->time = 20;
	// leaderBoard.head->next = NULL;

	

	add_score("Maolin", 20);
	add_score("Maolin", 25);
	// add_score("Bob", 17);
	// add_score("Tom", 18);
	// add_score("James", 99);
	show_leaderboard();
	// printf("%s\n", "Hello");
	return 0;
}

void place_mines() {
	for (int i = 0; i < NUM_MINES; ++i)
	{
		int x, y;
		do
		{
			x = rand() % NUM_TILES_X;
			y = rand() % NUM_TILES_Y;
			// printf("%d %d\n", x, y);
		} while (tile_contains_mine(x, y));
		// place mine at x, y
		gameState.tiles[x][y].isMine = true;
	}
}

void show_board() {
	char* vertical = "ABCDEFGHI";
	printf("Remaining mines: %d\n", 10);
	printf("\n");
	printf("    1 2 3 4 5 6 7 8 9\n");
	printf("  -------------------\n");
	for (int i = 0; i < NUM_TILES_Y; ++i)
	{
		printf("%c | ", vertical[i]);
		for (int j = 0; j < NUM_TILES_X; ++j)
		{
			if (tile_contains_mine(i, j))
			{
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

bool tile_contains_mine(int x, int y) {
	return gameState.tiles[x][y].isMine;
}

void add_score(char* name, int time) {
	player_t *curr_player = getPlayer(name);

	curr_player->gamesWon++;
	curr_player->gamesPlayed++;


	if (leaderBoard.head == NULL)
	{
		leaderBoard.head = malloc(sizeof(score_t));
		if (leaderBoard.head == NULL)
		{
			printf("%s\n", "Error");
			return;
		}
		leaderBoard.head->name = name;
		leaderBoard.head->time = time;
		leaderBoard.head->next = NULL;

		return;
	}

	score_t *current = leaderBoard.head;

	if (time > current->time)
	{
		leaderBoard.head = malloc(sizeof(score_t));
		if (leaderBoard.head == NULL)
		{
			printf("%s\n", "Error");
			return;
		}
		leaderBoard.head->name = name;
		leaderBoard.head->time = time;
		leaderBoard.head->next = current;
		return;
	}

	while(current->next != NULL && current->next->time > time) {
		current = current->next;
	}

	score_t *next = current->next;

	current->next = malloc(sizeof(score_t));
	if (leaderBoard.head == NULL)
	{
		printf("%s\n", "Error");
		return;
	}
	current->next->name = name;
	current->next->time = time;
	current->next->next = next;
}

void show_leaderboard() {
	score_t *current = leaderBoard.head;

	printf("=======================================================\n");

	while(current != NULL) {
		player_t* curr_player = getPlayer(current->name);
		printf("%s\t\t%d seconds\t\t%d games won, %d games played\n", current->name, current->time, curr_player->gamesWon, curr_player->gamesPlayed);
		current = current->next;
	}

	printf("=======================================================\n");
	
}



void setup_players() {

	leaderBoard.players[0] = malloc(sizeof(player_t));
	leaderBoard.players[0]->name = "Maolin";
	leaderBoard.players[0]->password = "111111";
	leaderBoard.players[0]->gamesWon = 0;
	leaderBoard.players[0]->gamesPlayed = 0;

	return;



	FILE *fp;
	char buff[255];

	fp = fopen("Authentication.txt", "r");
	fscanf(fp, "%s", buff);
   	printf("TEST %s\n", buff);
   	fscanf(fp, "%s", buff);
   	printf("TEST %s\n", buff);

   	for (int i = 0; i < 10; ++i)
   	{
   		char nameBuff[255];
   		char passwordBuff[255];
   		fscanf(fp, "%s", nameBuff);

   		// char blah = nameBuff;




   		// printf("1: %s\n", name);
   		leaderBoard.players[i] = malloc(sizeof(player_t));
   		// fgets(nameBuff, 255, stdin);
   		// strcpy(leaderBoard.players[i]->name, nameBuff);
   		leaderBoard.players[i]->name = &nameBuff;

   		fscanf(fp, "%s", passwordBuff);
   		// printf("2: %s\n", passwordBuff);
		leaderBoard.players[i]->password = passwordBuff;
		leaderBoard.players[i]->gamesWon = 0;
		leaderBoard.players[i]->gamesPlayed = 0;

		printf("%s\n", leaderBoard.players[i]->name);
		printf("%s\n", leaderBoard.players[i]->password);
   	}

   	printf("%s\n", leaderBoard.players[0]->name);

}


