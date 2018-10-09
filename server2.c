/*
*  Materials downloaded from the web. See relevant web sites listed on OLT
*  Collected and modified for teaching purpose only by Jinglan Zhang, Aug. 2006
*/


#include <arpa/inet.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <unistd.h>
#include <errno.h>

#define MAXDATASIZE 50 /* max number of bytes we can get at once */
#define ARRAY_SIZE 10  /* Size of array to receive */
#define BACKLOG 10     /* how many pending connections queue will hold */
#define RETURNED_ERROR -1
#define TERMINATE_CONNECTION 65535

typedef struct Player
{
	char* name;
	char* pass;
} player_t;


void Send_Array_Data(int socket_id) {
	/* Create an array of squares of first 30 whole numbers */
	// int simpleArray[ARRAY_SIZE] = {0};
	// for (int i = 0; i < ARRAY_SIZE; i++) {
	// 	simpleArray[i] = i * i;
	// }

	// uint16_t statistics;  
	// for (int i = 0; i < ARRAY_SIZE; i++) {
	// 	statistics = htons(simpleArray[i]);
	// 	send(socket_id, &statistics, sizeof(uint16_t), 0);
	// }
}

int Receive_Int_Array(int socket_identifier) {
	int number_of_bytes;
    uint16_t statistics;
    int *results = malloc(sizeof(int) * ARRAY_SIZE);

    for (int i = 0; i < ARRAY_SIZE; i++) {
    	number_of_bytes = recv(socket_identifier, &statistics, sizeof(uint16_t), 0);

		if (number_of_bytes == RETURNED_ERROR) {
			perror("recv");
			exit(EXIT_FAILURE);
		} else if (number_of_bytes == 0) {
			printf("Nothing received.\n");
			// continue;
		}

		results[i] = ntohs(statistics);
		printf("results[i] = %d\n", results[i]);
		if (results[i] == TERMINATE_CONNECTION) {
			printf("Killing thread.\n");
			free(results);
			return 0;
		}
	}

	free(results);

	return 1;
}

char* Receive_String(int socket_id, char* buf) {
	int number_of_bytes = 0;

	if ((number_of_bytes = recv(socket_id, buf, MAXDATASIZE, 0)) == -1) {
		perror("recv");
		exit(1);
	}

	buf[number_of_bytes] = '\0';

	printf("Received: %s\n", buf);

	if (send(socket_id, "Received (Server)", MAXDATASIZE , 0) == -1) {
		perror("send");
	}

	return buf;
}

void Run_Thread(int socket_id) {
	int limit = 40;
	int running = 1;
	char buf[limit];

	pthread_detach(pthread_self());

	player_t *curr_player = malloc(sizeof(player_t));
	curr_player->name = malloc(sizeof(char) * limit);
	curr_player->pass = malloc(sizeof(char) * limit);

	Receive_String(socket_id, curr_player->name);
	Receive_String(socket_id, curr_player->pass);

	printf("Player Name: %s\n", curr_player->name);
	printf("Player Password: %s\n", curr_player->pass);

	while (running) {
		Receive_String(socket_id, buf);

		if (strncmp(buf, "-1", 30) == 0)
		{
			running = 0;
		}
	}

	printf("Socket %d disconnected\n", socket_id);
	close(socket_id);
	// free(curr_player);
	pthread_exit(NULL);
}




int main(int argc, char *argv[]) {
	/* Thread and thread attributes */
	pthread_t client_thread;
	pthread_attr_t attr;

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

	printf("server starts listnening ...\n");

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
}
