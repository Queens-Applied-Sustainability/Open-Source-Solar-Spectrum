/*
 * Control the stepper motors of the Open Source Solar Spectrum
 * https://github.com/Queens-Applied-Sustainability/Open-Source-Solar-Spectrum
 * 
 * Based on code originally written by Matthew De Vuono
 */


//this code merges the zenith calculations with the stepper motion in
//order to get the stepper to move the shadowband to the correct zenith based on the 
//results from the code. The steppers will move in 10 degree increments to 90degrees.
//includes code to initialize the position of the stepper motor for location calibration.

#include <Wire.h>
#include <AFMotor.h>     // http://www.ladyada.net/make/mshield/download.html
#include <Chronodot.h>   // I (phil) found this here: https://github.com/radekw/Arduino/tree/master/Libraries/Chronodot

Chronodot RTC;

const float pi = 3.14592654;

AF_Stepper motor1(200, 1);     //'1' for M1 and M2 or '2' for M3 and M4
AF_Stepper motor2(200, 2);
long previousMillis = 0;      // will store last time LED was updated
long interval = 58;           // interval at which to blink (milliseconds)
int montharray[13]={
  0,1,2,3,4,5,6,7,8,9,10,11,12};
int hourarray[25]={
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,
  19,20,21,22,23,24};    //hour (1-24)

//array to hold the values to find day of year
int monthdoy[13] = {
  0,0,31,59,90,120,151,181,212,243,273,304,334};   
float latitude = 43;    //user changes per location
float longitude = 76.3;    //user changes per location
float meridian = 75;    //user changes per location
float phi;    //initialize latitude in radians
int buttonstate = 0;
const int buttonpin = 2;

void setup() {
  // initialize serial output
  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  motor1.setSpeed(10);  // 10 rpm 
  motor2.setSpeed(10);  // 10 rpm  
  pinMode(13, OUTPUT);
  buttonstate = digitalRead(buttonpin);
  pinMode(buttonpin, INPUT);    
  phi = (latitude*pi)/180;     //latitude to radians
}

void loop() {   
  DateTime now = RTC.now();
  int time = now.minute();

  if(time = 00) {
  //the instances for the program to run, in this case every minute
    // initialize motors
    if (buttonstate == LOW) {     
      // move motor 
      motor1.step(1, FORWARD, SINGLE); 
      motor2.step(1, FORWARD, SINGLE);  
    }

    //run sensing program
    for(int i=0; i<10; i=i++) {
      motor1.step(5, FORWARD, DOUBLE);
      delay(5000);
      digitalWrite(13, HIGH);   // set the spectrometer on
      delay(250);              // wait for a second
      digitalWrite(13, LOW);
      delay(2000);
    }
    motor1.step(50, BACKWARD, DOUBLE);

    //check and import clock values from rtc clock

    int hour = now.hour();          //from clock
    int day = now.day();        //from clock
    int month = now.month();      //from clock

    //calculating the day of year
    int doy = monthdoy[month] + day;

    //declination calculations
    float ratio = (284.0+doy)/365*360;
    float ratiorad = ratio*pi/180.0;
    float decdeg = 23.45*sin(ratiorad);
    float dec = decdeg*pi/180;

    //calculating solar time
    int Bdeg = (doy-1)*(360/365);
    float B = Bdeg*pi/180;                 //dividing E by 60 gives it in a fraction of an hour
    float E = (229.2*(0.000075+0.001868*cos(B)-0.032077*sin(B)-0.014615*cos(2*B)-0.04089*sin(2*B)))/60;
    float solartime = hour + 4*(meridian-longitude)/60+E;

    //solar hour calculation
    float omegadegrees=(solartime-12)*15;
    float omegarad =omegadegrees*pi/180;

    //Zeith calculation in radians
    float zenith =cos(phi)*cos(dec)*cos(omegarad)+sin(phi)*sin(dec);
    zenith = (acos(zenith))*180/pi;


    float result = ((90-zenith)/1.8);
    int angle = result;

    motor2.step(angle, FORWARD, DOUBLE);
    delay(5000);
    for(int i=0; i<10; i++) { 
      motor1.step(5, FORWARD, DOUBLE);
      delay(5000);
      digitalWrite(13, HIGH);   // set the spectrometer on
      delay(250);              // wait for a second
      digitalWrite(13, LOW);
      delay(2000);
    }
    motor1.step(50, BACKWARD, DOUBLE);
    motor2.step(angle, BACKWARD, DOUBLE);
  }
  motor1.release();
  motor2.release();

}

