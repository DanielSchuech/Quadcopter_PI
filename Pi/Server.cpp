#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <iostream>


int main(){
	int sockfd, newsockfd, readerror;
	struct sockaddr_in serv_addr, cli_addr;
	char buffer[256];
	socklen_t clilen;

	// call socket()
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		perror("ERROR opening socket");
		//exit(1);
		return 0;
	}

	// Initialise socket structure
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(5005);

	bzero((char *)&cli_addr, sizeof(cli_addr));

	// bind host address
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("ERROR on binding");
		//exit(1);
		return 0;
	}

	// listen for clients
	listen(sockfd, 5);
	clilen = sizeof(cli_addr);

	// accept connection from client
	newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
	if (newsockfd < 0)
	{
		perror("ERROR on accept");
		//exit(1);
		return 0;
	}

	// Connection established -> start communicating
	while (true){
		bzero(buffer, 256);
		readerror = recv(newsockfd, buffer, 255, 0);
		if (readerror < 0)
		{
			perror("ERROR reading from socket");
			//exit(1);
			return 0;
		}

		//Decode Values
		float leftStick_X, leftStick_Y, rightStick_X, rightStick_Y, leftTrigger, rightTrigger;
		std::string M[6];
		int j = 0;
		for (int i = 0; i < 100; i++){
			if (buffer[i] == '|' && j == 5) break;
			else if (buffer[i] == '|') j = j + 1;
			else {
				M[j] = M[j] + buffer[i];
			}
		}

		//printf("Message: %s - %s - %s - %s - %s - %s -\n", 
		//	M[0], M[1], M[2], M[3], M[4], M[5]);
		std::cout << "Message: " << M[0] << " - " << M[1] << " - "
			<< M[2] << " - " << M[3] << " - " << M[4] << " - " << M[5] << " - " << std::endl;
	}


}