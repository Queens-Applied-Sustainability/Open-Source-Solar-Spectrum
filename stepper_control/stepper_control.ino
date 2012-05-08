/*
 * Control the stepper motors of the Open Source Solar Spectrum
 * https://github.com/Queens-Applied-Sustainability/Open-Source-Solar-Spectrum
 * 
 * Based on code originally written by Matthew De Vuono
 */
 
///////////// INSTALLATION-SPECIFIC CONSTANTS
#define LATITUDE 43.0  
#define LONGITUDE 76.3
#define MERIDIAN 75.0

// Global Constants
#define SPEC_TRIGGER 13 // output pin to trigger a spectrometer reading
#define BUTTON 2

// External Libraries
#include <Wire.h>
#include <AFMotor.h>   // http://www.ladyada.net/make/mshield/download.html
#include <Chronodot.h> // http://planetstephanie.net/2011/04/09/chronodot-library-for-arduino/


//this code merges the zenith calculations with the stepper motion in
//order to get the stepper to move the shadowband to the correct zenith based on the 
//results from the code. The steppers will move in 10 degree increments to 90degrees.
//includes code to initialize the position of the stepper motor for location calibration.


Chronodot RTC; // Real-time Clock

AF_Stepper optic_motor(200, 1);  // M1 and M2 on the motor driving sheild
AF_Stepper band_motor(200, 2);   // M3 and M4

// fixme: these should not be global
long previousMillis = 0;      // will store last time LED was updated


//array to hold the values to find day of year
int month_day_of_year[13] = {
  0,0,31,59,90,120,151,181,212,243,273,304,334}; 
float phi;    //initialize latitude in radians
int button_state = 0;

void scratch(DateTime &now);

void setup() {
  // initialize serial output
  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  optic_motor.setSpeed(10);  // 10 rpm 
  band_motor.setSpeed(10);  // 10 rpm  
  pinMode(SPEC_TRIGGER, OUTPUT);
  button_state = digitalRead(BUTTON);
  pinMode(BUTTON, INPUT);
}

void loop() {
  DateTime now = RTC.now();
  int time = now.minute();

  if(time = 00) {
  //the instances for the program to run, in this case every minute
    scratch(now);
  }
  optic_motor.release();
  band_motor.release();
}

void scratch(DateTime &now) {
 // initialize motors
  if (button_state == LOW) {     
    // move motor 
    optic_motor.step(1, FORWARD, SINGLE); 
    band_motor.step(1, FORWARD, SINGLE);  
  }

  //run sensing program
  for(int i=0; i<10; i=i++) {
    optic_motor.step(5, FORWARD, DOUBLE);
    delay(5000);
    digitalWrite(SPEC_TRIGGER, HIGH);   // set the spectrometer on
    delay(250);              // wait for a moment
    digitalWrite(SPEC_TRIGGER, LOW);
    delay(2000);
  }
  optic_motor.step(50, BACKWARD, DOUBLE);

  //check and import clock values from rtc clock

  int hour = now.hour();          //from clock
  int day = now.day();        //from clock
  int month = now.month();      //from clock

  //calculating the day of year
  int day_of_year = month_day_of_year[month] + day;

  //declination calculations
  float ratio = (284.0+day_of_year)/365*360;
  float ratio_rad = ratio*PI/180.0;
  float dec_deg = 23.45*sin(ratio_rad);
  float dec = dec_deg*PI/180;

  //calculating solar time
  int Bdeg = (day_of_year-1)*(360/365);
  float B = Bdeg*PI/180;          //dividing E by 60 gives it in a fraction of an hour
  float E = (229.2*(0.000075+0.001868*cos(B)-0.032077*sin(B)-0.014615*cos(2*B)-0.04089*sin(2*B)))/60;
  float solartime = hour + 4*(MERIDIAN-LONGITUDE)/60+E;

  //solar hour calculation
  float omega_degrees=(solartime-12)*15;
  float omega_rad =omega_degrees*PI/180;

  //Zeith calculation in radians
  float zenith =cos(phi)*cos(dec)*cos(omega_rad)+sin(phi)*sin(dec);
  zenith = (acos(zenith))*180/PI;


  float result = ((90-zenith)/1.8);
  int angle = result;

  band_motor.step(angle, FORWARD, DOUBLE);
  delay(5000);
  for(int i=0; i<10; i++) { 
    optic_motor.step(5, FORWARD, DOUBLE);
    delay(5000);
    digitalWrite(SPEC_TRIGGER, HIGH);   // set the spectrometer on
    delay(250);              // wait for a moment
    digitalWrite(SPEC_TRIGGER, LOW);
    delay(2000);
  }
  optic_motor.step(50, BACKWARD, DOUBLE);
  band_motor.step(angle, BACKWARD, DOUBLE);
}
