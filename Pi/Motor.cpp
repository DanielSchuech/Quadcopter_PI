#include <wiringPi.h>
#include <softPwm.h>
#include <iostream>
#include <string>

using namespace std;

int main(){
	//Initialisieren der GPIO für wiringPi
	wiringPiSetup();
	//softPWM initialisieren
	//softPwmCreate(int pin, int initialValue, int pwmRange);
	softPwmCreate(0, 0, 100);
	
	//PWM ändern -> void softPwmWrite (int pin, int value) ;
	int speed = 0;
	int changeValue = 1;
	while (true){
		string x;
		cout << "Input: ";
		cin >> x;
		if (x == "a") {
			speed = speed + changeValue;
			cout << "---->Increase Speed to " << speed <<"%" << endl;
			softPwmWrite(0, speed);
		}
		else if (x == "y"){ 
			speed = speed - changeValue;
			cout << "--->Decrease Speed to " << speed <<"%" << endl;
			softPwmWrite(0, speed);
		}
	}

	//LED Test
	/*pinMode(0, OUTPUT);
	digitalWrite(0, HIGH);	// On
	delay(3000);		// mS
	digitalWrite(0, LOW);	// Off
	delay(500);*/
}