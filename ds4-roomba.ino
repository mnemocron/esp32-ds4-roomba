/*
 * roomba inspired by: https://github.com/johnboiles/esp-roomba-mqtt
 * https://github.com/johnboiles/esp-roomba-mqtt/blob/master/src/main.cpp
 * 
 * Roomba command documentation (SCI)
 * https://www.usna.edu/Users/weaprcon/esposito/_files/roomba.matlab/Roomba_SCI.pdf
 * 
 * MG92B Servo 
 * https://www.adafruit.com/product/2307
 * 
 */

#include <ESP32Servo.h>
#include <PS4Controller.h>
#include "roomba.h"
#include <math.h>

// #define SERIAL_COM_MODE_DEBUG // if defined --> prints to console / if commented --> use Roomba serial commands

#define SERVO_PIN 12
#define ROOMBA_BRC_PIN 13
#define LED_FLASH_PIN 4

#define SERVO_AZIMUT_OFFSET_DEG 20

Servo myservo;
Roomba roomba(&Serial, Roomba::Baud19200);

int servoPos = 0;
int flashLEDstate = 0;
int vacuumState = 0;
int roombaWakeState = 0;
int btn_state_square[2] = {0,0};
int btn_state_cross[2] = {0,0};
int btn_state_circle[2] = {0,0};
int btn_state_options[2] = {0,0};

void setup() {
  Serial.begin(19200);
  PS4.begin("B4:8C:9D:32:A3:E2");  // bluetooth MAC that the DS4 controller is paired

	// Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(SERVO_PIN, 750, 2250); // attaches the servo on pin 18 to the servo object
	// using default min/max of 1000us and 2000us
	// different servos may require different min/max settings
	// for an accurate 0 to 180 sweep

  pinMode(LED_FLASH_PIN, OUTPUT);
  digitalWrite(LED_FLASH_PIN, LOW);

  pinMode(ROOMBA_BRC_PIN, INPUT);

  #ifdef SERIAL_COM_MODE_DEBUG
    Serial.println("Ready.");
  #endif
}

void loop() {
  // Below has all accessible outputs from the controller
  myservo.write(servoPos); 

  if (PS4.isConnected()) {
    btn_state_square[1] = btn_state_square[0];
    btn_state_square[0] = PS4.Square();
    btn_state_cross[1] = btn_state_cross[0];
    btn_state_cross[0] = PS4.Cross();
    btn_state_circle[1] = btn_state_circle[0];
    btn_state_circle[0] = PS4.Circle();
    btn_state_options[1] = btn_state_options[0];
    btn_state_options[0] = PS4.Options();
    // rising edge detection
    if (btn_state_square[0] > btn_state_square[1]) {
      toggleLEDFlash();
    }
    if (btn_state_cross[0] > btn_state_cross[1]) {
      toggleVacuum();
    }
    if (btn_state_circle[0] > btn_state_circle[1]) {
      honk();
    }
    if (btn_state_options[0] > btn_state_options[1]) {
      wakeRoomba();
    }
    if (PS4.LStickY()) {
      int azimut = -PS4.LStickY();  // max range: -127 ... +127
      azimut * 2/3; // range -63 ... + 63
      if(azimut > 3 || azimut < 3){
        if(azimut > 70){
          azimut = 70; // limit to +50 deg
        }
        if(azimut < -(SERVO_AZIMUT_OFFSET_DEG)){
          azimut = -(SERVO_AZIMUT_OFFSET_DEG); // limit to 
        }
        servoPos = SERVO_AZIMUT_OFFSET_DEG + azimut;
      }
    }
    
    double stick_steer = (double) PS4.RStickX();
    if(stick_steer < 10.0 && stick_steer > 10.0){
      stick_steer = 0.0;
    }
    double stick_drive = (double) PS4.RStickY();
    if(stick_drive < 10.0 && stick_drive > 10.0){
      stick_drive = 0.0;
    }

    double velocity = sqrt( pow(stick_drive,2.0) + pow(stick_steer,2.0) );
    double angle = atan2(stick_drive, stick_steer);
    double wheel_r, wheel_l;

    // 1. Quadrant
    if(angle >= 0.0 && angle < M_PI_2){
      wheel_l = velocity;
      wheel_r = velocity * sin(angle);
    } else if(angle >= M_PI_2){
      wheel_r = velocity;
      wheel_l = velocity * sin(angle);
    } else if(angle < 0.0 && angle >= (- (M_PI_2))){
      wheel_l = -velocity;
      wheel_r = velocity * sin(angle);
    } else if(angle <= (-(M_PI_2))){
      wheel_r = -velocity;
      wheel_l = velocity * sin(angle);
    }

    wheel_r *= 2;
    wheel_l *= 2;
    if(wheel_r < 20 && wheel_r > -20){
      wheel_r = 0;
    }
    if(wheel_l < 20 && wheel_l > -20){
      wheel_l = 0;
    }
    if(roombaWakeState){    
      roomba.driveDirect(wheel_l, wheel_r);
    }


  #ifdef SERIAL_COM_MODE_DEBUG
    Serial.printf("| %.2f | < %.2f deg  %.2f %.2f\n", velocity, angle, wheel_l, wheel_r);
  #endif
    
    // This delay is to make the output more human readable
    // Remove it when you're not trying to see the output
    delay(100);
  }
}

void toggleLEDFlash(){
  #ifdef SERIAL_COM_MODE_DEBUG
    Serial.printf("LED\n");
  #endif
  if(flashLEDstate){
    digitalWrite(LED_FLASH_PIN, LOW);
    flashLEDstate = 0;
  } else{
    digitalWrite(LED_FLASH_PIN, HIGH);
    flashLEDstate = 1;
  }
}

void toggleVacuum(){
  #ifdef SERIAL_COM_MODE_DEBUG
    Serial.printf("Vacuum\n");
  #endif
  if(vacuumState){
    if(roombaWakeState){
      roomba.drivers(0);
    }
    vacuumState = 0;
  } else{
    if(roombaWakeState){
      roomba.drivers(2); // bit 1 = vacuum motor+
    }
    vacuumState = 1;
  }
}

void honk(){
  #ifdef SERIAL_COM_MODE_DEBUG
    Serial.printf("Honk\n");
  #endif
}

void wakeRoomba(){
  if(roombaWakeState){
    roomba.power();
    roombaWakeState = 0;
  } else{
    pinMode(ROOMBA_BRC_PIN, OUTPUT);
    digitalWrite(ROOMBA_BRC_PIN, LOW);
    delay(100);
    digitalWrite(ROOMBA_BRC_PIN, HIGH);
    delay(2000);

    // device-detect pulse 3x
    for(int i=0; i<3; i++){
      digitalWrite(ROOMBA_BRC_PIN, LOW);
      delay(250);
      digitalWrite(ROOMBA_BRC_PIN, HIGH);
      delay(250);
    }
    pinMode(ROOMBA_BRC_PIN, INPUT);

    roomba.start();
    roomba.fullMode();
    roomba.drive(0, 0); // Stop
    Serial.flush();
    roombaWakeState = 1;
  }
}
