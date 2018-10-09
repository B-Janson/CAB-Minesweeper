#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h>

#define MAXDATASIZE 50 /* max number of bytes we can get at once */
#define ARRAY_SIZE 10
#define RETURNED_ERROR -1

void Receive_Array_Int_Data(int socket_identifier, int size) {
    int number_of_bytes;
    uint16_t statistics;

	int *results = malloc(sizeof(int) * ARRAY_SIZE);

	for (int i = 0; i < size; i++) {
		if ((number_of_bytes = recv(socket_identifier, &statistics, sizeof(uint16_t), 0)) == RETURNED_ERROR) {
			perror("recv");
			exit(EXIT_FAILURE);
		}
		results[i] = ntohs(statistics);
	}
	for (int i = 0; i < ARRAY_SIZE; i++) {
		printf("Array[%d] = %d\n", i, results[i]);
	}

	free(results);
}

void Send_Int_Array(int socket_id, int *myArray) {
	int i = 0;
	uint16_t statistics;  
	for (i = 0; i < ARRAY_SIZE; i++) {
		statistics = htons(myArray[i]);
		send(socket_id, &statistics, sizeof(uint16_t), 0);
	}
}

void Send_String(int socket_id, char message[], int length) {
	// printf("Send_String Socket ID: %d, message: %s, Length: %d\n", socket_id, message, length);
	int numbytes = 0;
	char outputBuf[MAXDATASIZE];

	if (send(socket_id, message, MAXDATASIZE, 0) == -1) {
		perror("send");
	}

	/* Receive message back from server */
	if ((numbytes = recv(socket_id, outputBuf, MAXDATASIZE, 0)) == -1) {
		perror("recv");
		exit(1);
	}

	// outputBuf[numbytes] = '\0';

	// printf("%s\n", outputBuf);
}

int main(int argc, char *argv[]) {
	int socket_id; 
	char inputBuff[MAXDATASIZE];
	struct hostent *he;
	struct sockaddr_in their_addr; /* connector's address information */

	if (argc != 3) {
		fprintf(stderr,"usage: server_ip_address port_number\n");
		exit(1);
	}

	if ((he = gethostbyname(argv[1])) == NULL) {  /* get the host info */
		herror("gethostbyname");
		exit(1);
	}

	if ((socket_id = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	their_addr.sin_family = AF_INET;      /* host byte order */
	their_addr.sin_port = htons(atoi(argv[2]));    /* short, network byte order */
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */

	if (connect(socket_id, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
		printf("FAIL\n");
		perror("connect");
		exit(1);
	}

	int running = 1;

	printf("============================================\n");
	printf("Welcome to the online Minesweeper gaming system\n");
	printf("============================================\n\n");

	printf("You are required to log on with your registered user name and password.\n\n");
	printf("Username: ");
	scanf("%s", inputBuff);
	Send_String(socket_id, inputBuff, strlen(inputBuff));
	// buf = "";

	printf("Password: ");
	scanf("%s", inputBuff);
	Send_String(socket_id, inputBuff, strlen(inputBuff));

	// running = 0;

	while (running) {

		printf("ENTER SOMETHING TO SEND: ");
		scanf("%s", inputBuff);

		if (strncmp(inputBuff, "-1", MAXDATASIZE) == 0) {
			running = 0;
		}

		Send_String(socket_id, inputBuff, strlen(inputBuff));
		
	}

	close(socket_id);

	return 0;
}
