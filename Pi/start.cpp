//Include für Server
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <iostream>

//Include für Motor
#include <wiringPi.h>
#include <softPwm.h>

//Include für main
#include <pthread.h>
#include <sstream>
#include <unistd.h>		//for usleep
#include <stdlib.h>		// abs

using namespace std;

#define MAX_SPEED_CHANGE 1;

struct datacenter{
	float leftStick_X, leftStick_Y, rightStick_X, rightStick_Y, leftTrigger, rightTrigger;
};

float toFloat(string arg){
	stringstream data(arg);
	float x;
	data >> x;
	return x;
}

void *Server(void *arg){
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
		leftStick_X = toFloat(M[0]);
		leftStick_Y = toFloat(M[1]);
		rightStick_X = toFloat(M[2]);
		rightStick_Y = toFloat(M[3]);
		leftTrigger = toFloat(M[4]);
		rightTrigger = toFloat(M[5]);

		((struct datacenter*)arg)->leftStick_X = leftStick_X;
		((struct datacenter*)arg)->leftStick_Y = leftStick_Y;
		((struct datacenter*)arg)->rightStick_X = rightStick_X;
		((struct datacenter*)arg)->rightStick_Y = rightStick_Y;
		((struct datacenter*)arg)->leftTrigger = leftTrigger;
		((struct datacenter*)arg)->rightTrigger = rightTrigger;

		//std::cout << "Message: " << M[0] << " - " << M[1] << " - "
		//	<< M[2] << " - " << M[3] << " - " << M[4] << " - " << M[5] << " - " << std::endl;
	}
}

void *Motor(void *arg){
	//Initialisieren der GPIO für wiringPi
	wiringPiSetup();
	//softPWM initialisieren
	//softPwmCreate(int pin, int initialValue, int pwmRange);
	softPwmCreate(0, 0, 100);

	//PWM ändern -> void softPwmWrite (int pin, int value) ;
	float speed = 0;
	int changeValue = 1;
	while (true){
		//Datacenter parsen + verarbeiten
		//cout << "Left Stick Y: " << ((struct datacenter*)arg)->leftStick_Y << endl;
		float leftStick_Y = ((struct datacenter*)arg)->leftStick_Y;

		if (abs(leftStick_Y) < 1.1) speed = speed + leftStick_Y/MAX_SPEED_CHANGE;
		if (speed < 0) speed = 0;
		if (speed > 100) speed = 100;

		cout << "Speed: " << speed << "%" << endl;
		softPwmWrite(0, speed);



		/* Old Manuel In/Decrease 
		string x;
		cout << "Input: ";
		cin >> x;
		if (x == "a") {
			speed = speed + changeValue;
			cout << "---->Increase Speed to " << speed << "%" << endl;
			softPwmWrite(0, speed);
		}
		else if (x == "y"){
			speed = speed - changeValue;
			cout << "--->Decrease Speed to " << speed << "%" << endl;
			softPwmWrite(0, speed);
		} */

		usleep(100000);
	}
}

int main(){
	//Datacenter initialisieren und Nullen
	struct datacenter data;
	data.leftStick_X = 0;
	data.leftStick_Y = 0;
	data.rightStick_X = 0;
	data.rightStick_Y = 0;
	data.rightTrigger = 0;
	data.leftTrigger = 0;

	pthread_t Server_thread, Motor_thread;
	int error = pthread_create(&Server_thread, NULL, Server, (void *)&data);
	if (error) cout << "ERROR on creating SERVER" << endl;
	error = pthread_create(&Motor_thread, NULL, Motor, (void *)&data);
	if (error) cout << "ERROR on creating MOTOR" << endl;

	//warte auf das beenden der Threads
	pthread_join(Server_thread, NULL);
	pthread_join(Motor_thread, NULL);

	
}