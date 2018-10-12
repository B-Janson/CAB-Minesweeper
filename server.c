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

#define MAXDATASIZE 50 /* max number of bytes we can get at once */
#define ARRAY_SIZE 10  /* Size of array to receive */
#define BACKLOG 10     /* how many pending connections queue will hold */
#define RETURNED_ERROR -1
#define TERMINATE_CONNECTION 65535

#define RANDOM_NUMBER_SEED 42

LeaderBoard leaderBoard;


Player* getPlayer(char* name) {
	for (int i = 0; i < 1; ++i) {
		if (strncmp(leaderBoard.players[i]->name, name, MAXDATASIZE) == 0) {
			return leaderBoard.players[i];
		}
	}
	return NULL;
}

char* Receive_String(int socket_id, char* buf) {
	int number_of_bytes = 0;

	if ((number_of_bytes = recv(socket_id, buf, MAXDATASIZE, 0)) == -1) {
		perror("recv");
		exit(1);
	}

	buf[number_of_bytes] = '\0';

	return buf;
}

void Send_String(int socket_id, char *message) {
	if (send(socket_id, message, MAXDATASIZE , 0) == -1) {
		perror("send");
	}
}

char* Receive_String_And_Reply(int socket_id, char* buf) {
	Receive_String(socket_id, buf);

	Send_String(socket_id, "Received");

	return buf;
}

void Run_Thread(int socket_id) {
	bool running = true;
	char inputBuff[MAXDATASIZE];
	char outputBuff[MAXDATASIZE];
	GameState *gameState = malloc(sizeof(GameState));
	
	// This lets the process know that when the thread dies that it should take care of itself.
	pthread_detach(pthread_self());

	// Set the seed
	srand(RANDOM_NUMBER_SEED);

	bool isAuthenticated = false;

	char * serverResponse = "1";

	// Get the username
	Receive_String_And_Reply(socket_id, inputBuff);

	printf("Username entered: %s\n", inputBuff);

	// Check if a user exists with that name
	Player *curr_player = getPlayer(inputBuff);

	// Get the password
	Receive_String(socket_id, inputBuff);

	// If we found a player with that name AND the password matches, authenticate.
	if (curr_player != NULL && strncmp(curr_player->password, inputBuff, MAXDATASIZE) == 0) {
		isAuthenticated = true;
		printf("User isAuthenticated\n");
	}

	if (!isAuthenticated) {
		running = 0;
		serverResponse = "0";
		printf("Player logged in with wrong credentials.\n");
	}

	Send_String(socket_id, serverResponse);

	// place_mines(gameState);
	// show_board(gameState);

	
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

	while (running) {
		Receive_String_And_Reply(socket_id, inputBuff);

		printf("Received: %s\n", inputBuff);

		if (strncmp(inputBuff, "-1", 30) == 0)
		{
			running = 0;
		}
	}

	printf("Socket %d disconnected\n", socket_id);
	close(socket_id);
	// free(curr_player);
	pthread_exit(NULL);
}




bool tile_contains_mine(GameState *gameState, int x, int y) {
	return gameState->tiles[x][y].isMine;
}


void place_mines(GameState *gameState) {
	for (int i = 0; i < NUM_MINES; ++i)
	{
		int x, y;
		do
		{
			x = rand() % NUM_TILES_X;
			y = rand() % NUM_TILES_Y;
			// printf("%d %d\n", x, y);
		} while (tile_contains_mine(gameState, x, y));
		// place mine at x, y
		gameState->tiles[x][y].isMine = true;
	}
}

void show_board(GameState *gameState) {
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
			if (tile_contains_mine(gameState, i, j))
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



void add_score(char* name, int time) {
	Player *curr_player = getPlayer(name);

	curr_player->gamesWon++;
	curr_player->gamesPlayed++;


	if (leaderBoard.head == NULL)
	{
		leaderBoard.head = malloc(sizeof(Score));
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

	Score *current = leaderBoard.head;

	if (time > current->time)
	{
		leaderBoard.head = malloc(sizeof(Score));
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

	Score *next = current->next;

	current->next = malloc(sizeof(Score));
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
	Score *current = leaderBoard.head;

	printf("=======================================================\n");

	while(current != NULL) {
		Player* curr_player = getPlayer(current->name);
		printf("%s\t\t%d seconds\t\t%d games won, %d games played\n", current->name, current->time, curr_player->gamesWon, curr_player->gamesPlayed);
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
		fprintf(stderr,"usage: client port_number\n");
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
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
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
	while(1) {  /* main accept() loop */
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
			perror("accept");
			continue;
		}
		printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));

		// Create a thread to accept client
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_create(&client_thread, &attr, Run_Thread, new_fd);
		// pthread_join(client_thread, NULL);
	}

	close(new_fd);

	return 0;
}


