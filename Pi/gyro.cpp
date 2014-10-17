#include <wiringPiI2C.h>
#include <iostream>
#include <unistd.h>		//for usleep
#include <stdio.h>		//printf
#include<cmath>			//pow

using namespace std;

#define CTRL_REG1 0x20
#define CTRL_REG2 0x21
#define CTRL_REG3 0x22
#define CTRL_REG4 0x23

int main(){
	int fd = wiringPiI2CSetup(0x69);
	wiringPiI2CWriteReg8(fd, CTRL_REG1, 0x1F); //Turn on all axes, disable power down
	wiringPiI2CWriteReg8(fd, CTRL_REG3, 0x08); //Enable control ready signal
	wiringPiI2CWriteReg8(fd, CTRL_REG4, 0x80); // Set scale (500 deg/sec)
	usleep(100000);                    // Wait to synchronize


	float old_x, old_y, old_z;
	old_x = 0;
	old_y = 0;
	old_z = 0;

	while (true){
		int MSB, LSB;

		LSB = wiringPiI2CReadReg16(fd, 0x28);
		MSB = wiringPiI2CReadReg16(fd, 0x29);
		int xh = ((MSB << 8) | LSB);
		float x = (float)xh / (float)16777216;
		if (old_x < 0.05 && x > 0.94) x = old_x;
		old_x = x;

		MSB = wiringPiI2CReadReg16(fd, 0x2B);
		LSB = wiringPiI2CReadReg16(fd, 0x2A);
		int yh = ((MSB << 8) | LSB);
		float y = (float)yh / (float)16777216;
		if (old_y < 0.05 && y > 0.94) y = old_y;
		old_y = y;

		MSB = wiringPiI2CReadReg16(fd, 0x2D);
		LSB = wiringPiI2CReadReg16(fd, 0x2C);
		int zh = ((MSB << 8) | LSB);
		float z = (float)zh / (float)16777216;
		if (old_z < 0.05 && z > 0.94) z = old_z;
		old_z = z;

		/*Berechnung des Winkels
		double xAngle = atan(x / (sqrt(pow(y,2) + pow(z,2))));
		double yAngle = atan(y / (sqrt(pow(x,2) + pow(z,2))));
		double zAngle = atan(sqrt(pow(x,2) + pow(y,2)) / z);

		xAngle *= 180.00;   yAngle *= 180.00;   zAngle *= 180.00;
		xAngle /= 3.141592; yAngle /= 3.141592; zAngle /= 3.141592;
		*/

		cout << "Value of X is: " << x << endl;
		cout << "Value of Y is: " << y << endl;
		cout << "Value of Z is: " << z << endl;


		int t = wiringPiI2CReadReg8(fd, 0x26);
		printf("The temperature is: %d\n\n\n", t);

		
		/*cout << "X: " << wiringPiI2CRead(fd) <<
			"  Y: " << wiringPiI2CRead(fd) <<
			"  Z: " << wiringPiI2CRead(fd) << endl;*/
		usleep(500000);
	}
	return 0;
}