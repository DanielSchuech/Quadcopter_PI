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
#include <wiringPiI2C.h>
#include <softPwm.h>
#include <math.h>		//floor (round)

//Include für main
#include <pthread.h>
#include <sstream>
#include <unistd.h>		//for usleep
#include <stdlib.h>		// abs

#include <sys/time.h>	//time

using namespace std;

#define MAX_SPEED_CHANGE 1;

//define regs for Gyro
#define CTRL_REG1 0x20
#define CTRL_REG2 0x21
#define CTRL_REG3 0x22
#define CTRL_REG4 0x23

struct datacenter{
	float leftStick_X, leftStick_Y, rightStick_X, rightStick_Y, leftTrigger, rightTrigger;
	long seconds;
	int ButtonSelect;
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
		std::string M[7];
		int j = 0;
		for (int i = 0; i < 100; i++){
			if (buffer[i] == '|' && j == 6) break;
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
		int ButtonSelect = toFloat(M[6]);
		//std::cout << ButtonSelect << std::endl;
		
		((struct datacenter*)arg)->leftStick_X = leftStick_X;
		((struct datacenter*)arg)->leftStick_Y = leftStick_Y;
		((struct datacenter*)arg)->rightStick_X = rightStick_X;
		((struct datacenter*)arg)->rightStick_Y = rightStick_Y;
		((struct datacenter*)arg)->leftTrigger = leftTrigger;
		((struct datacenter*)arg)->rightTrigger = rightTrigger;
		((struct datacenter*)arg)->ButtonSelect = ButtonSelect;

		//Zeit speichern
		timeval time;
		gettimeofday(&time, NULL);
		((struct datacenter*)arg)->seconds = time.tv_sec;

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
	softPwmCreate(1, 0, 100);
	softPwmCreate(2, 0, 100);
	softPwmCreate(3, 0, 100);

	//Gyro initialisieren
	int fd = wiringPiI2CSetup(0x69);
	wiringPiI2CWriteReg8(fd, CTRL_REG1, 0x1F); //Turn on all axes, disable power down
	wiringPiI2CWriteReg8(fd, CTRL_REG3, 0x08); //Enable control ready signal
	wiringPiI2CWriteReg8(fd, CTRL_REG4, 0x80); // Set scale (500 deg/sec)
	usleep(100000);                    // Wait to synchronize

	//PWM ändern -> void softPwmWrite (int pin, int value) ;
	float speed1 = 0;
	float speed2 = 0;
	float speed3 = 0;
	float speed4 = 0;
	//int changeValue = 1;

	//Modus Variablen (Ob alle Motoren gleiche Speed oder Gyro Speed kontrolliert)
	int old_ButtonSelect = 0;
	int modus = 0;

	while (true){
		//Datacenter parsen + verarbeiten
		//cout << "Left Stick Y: " << ((struct datacenter*)arg)->leftStick_Y << endl;
		float leftStick_Y = ((struct datacenter*)arg)->leftStick_Y;
		float rightStick_Y = ((struct datacenter*)arg)->rightStick_Y;
		float rightStick_X = ((struct datacenter*)arg)->rightStick_X;
		long seconds = ((struct datacenter*)arg)->seconds;
		int ButtonSelect = ((struct datacenter*)arg)->ButtonSelect;

		//Gyro Werte ermitteln
		int MSB, LSB;
		LSB = wiringPiI2CReadReg8(fd, 0x28);
		MSB = wiringPiI2CReadReg8(fd, 0x29);
		int x = ((MSB << 8) | LSB);
		if (x >= 32768){
			x = (32768 - (x - 32768)) * (-1);
		}

		LSB = wiringPiI2CReadReg8(fd, 0x2A);
		MSB = wiringPiI2CReadReg8(fd, 0x2B);
		int y = ((MSB << 8) | LSB);
		if (y >= 32768){
			y = (32768 - (y - 32768)) * (-1);
		}

		LSB = wiringPiI2CReadReg8(fd, 0x2C);
		MSB = wiringPiI2CReadReg8(fd, 0x2D);
		int z = ((MSB << 8) | LSB);
		if (z >= 32768){
			z = (32768 - (z - 32768)) * (-1);
		}

		//Gyro Werte normalisieren
		float f_x = (float)x / (float)32768;
		float f_y = y / 32768;
		float f_z = z / 32768;

		//Check Time
		timeval time;
		gettimeofday(&time, NULL);
		if (time.tv_sec - seconds > 1){
			leftStick_Y = -0.1;
			rightStick_X = 0;
			rightStick_Y = 0;
			//std::cout << "STOPPPPPPPPPPP" << std::endl;
		}

		//Motoren steuern
		//Modus kontrollieren
		if (old_ButtonSelect < ButtonSelect) modus = !modus;
		old_ButtonSelect = ButtonSelect;
	
		std::cout << "Modus: " << modus << std::endl;
		if (modus == 0){	//Gleiche Speed für alle Motoren
			if (abs(leftStick_Y) < 1.1) {
				speed3 = speed3 + leftStick_Y / MAX_SPEED_CHANGE;
			}
			speed1 = speed3;
			speed2 = speed3;
			speed4 = speed3;
		}
		else{				//Gyro kontrolliert Speed
			std::cout << "Gyro x= " << f_x << " abs: " << fabs(f_x - 0) << std::endl;
			//X-Achse
			if (fabs(f_x - 0) > 0.01){
				std::cout << "change xxxx" << std::endl;
				if ((f_x - 0) < 0.01 && (speed3 - speed1 < 5)) speed3 = speed3 + 1;
				else if (speed1 - speed3 < 5) speed1 = speed1 + 1;
			} 

			//Y-Achse
			if (fabs(f_y - 0) > 0.01){
				if ((f_y - 0) < 0.01 && (speed4 - speed2 < 5)) speed4 = speed4 + 1;
				else if (speed2 - speed4 < 5) speed2 = speed2 + 1;
			}

			//Z-Achse
			if (fabs(f_z - 0) > 0.01){
				if ((f_z - 0) > 0.01){
					speed1 = speed1 - 1;
					speed2 = speed2 - 1;
					speed3 = speed3 - 1;
					speed4 = speed4 - 1;
				}
				else {
					speed1 = speed1 + 1;
					speed2 = speed2 + 1;
					speed3 = speed3 + 1;
					speed4 = speed4 + 1;
				}
			}
		}
		

		

		//Check speed in range
		if (speed1 < 0) speed1 = 0;
		if (speed1 > 100) speed1 = 100;
		if (speed2 < 0) speed2 = 0;
		if (speed2 > 100) speed2 = 100;
		if (speed3 < 0) speed3 = 0;
		if (speed3 > 100) speed3 = 100;
		if (speed4 < 0) speed4 = 0;
		if (speed4 > 100) speed4 = 100;

		cout << "Speed1: " << speed1 << " Speed2: " << speed2 << " Speed3: " << speed3 << " Speed4: " << speed4 << "%" << endl;
		softPwmWrite(0, speed1);
		softPwmWrite(1, speed2);
		softPwmWrite(2, speed3);
		softPwmWrite(3, speed4);

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
	data.ButtonSelect = 0;

	pthread_t Server_thread, Motor_thread;
	int error = pthread_create(&Server_thread, NULL, Server, (void *)&data);
	if (error) cout << "ERROR on creating SERVER" << endl;
	error = pthread_create(&Motor_thread, NULL, Motor, (void *)&data);
	if (error) cout << "ERROR on creating MOTOR" << endl;

	//warte auf das beenden der Threads
	pthread_join(Server_thread, NULL);
	pthread_join(Motor_thread, NULL);

	
}