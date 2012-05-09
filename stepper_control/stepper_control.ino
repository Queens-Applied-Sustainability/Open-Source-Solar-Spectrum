/*
 * Control the stepper motors of the Open Source Solar Spectrum
 * 
 * Calculates the zenith to know exactly where to move the shaddow band
 * The other stepper rotatest the spectrometer 90 degrees in 10 degree increments.
 * https://github.com/Queens-Applied-Sustainability/Open-Source-Solar-Spectrum
 * http://www.appropedia.org/Open_source_solar_spectrum_project
 * 
 * Based on code originally written by Matthew De Vuono
 */

///////////// INSTALLATION-SPECIFIC CONSTANTS
#define LATITUDE 43.0  
#define LONGITUDE 76.3
#define MERIDIAN 75.0

// Global Constants
#define SPEC_TRIGGER 13 // output pin to trigger a spectrometer reading

// External Libraries
#include <Wire.h>
#include <AFMotor.h>   // http://www.ladyada.net/make/mshield/download.html
#include <Chronodot.h> // http://planetstephanie.net/2011/04/09/chronodot-library-for-arduino/


Chronodot RTC; // Real-time Clock

AF_Stepper optic_motor(200, 1);  // M1 and M2 on the motor driving sheild
AF_Stepper band_motor(200, 2);   // M3 and M4

// fixme: these should not be global
long previousMillis = 0;      // will store last time LED was updated


//array to hold the values to find day of year
int month_day_of_year[13] = {0,0,31,59,90,120,151,181,212,243,273,304,334};

float phi;    //initialize latitude in radians

void sense(DateTime &now, boolean with_shadow);
int calculateAngle(DateTime &now);
void bar(int angle, uint8_t dir);
void takeSpecReadings();

DateTime now;
int lastminute, time = 1;

void setup() {
  // initialize serial output
  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));  // set RTC to time of sketch compile/upload (MAKE SURE YOUR COMPUTER'S TIME IS RIGHT)
  optic_motor.setSpeed(10);  // 10 rpm 
  band_motor.setSpeed(10);  // 10 rpm  
  pinMode(SPEC_TRIGGER, OUTPUT);
  Serial.println("Ready to go!");
  Serial.print("unix time: ");
  Serial.println(RTC.now().unixtime());
}

void loop() {
  
  Serial.print ("Waiting until minute 0: ");
  do {
    do {
      lastminute = time;
      now = RTC.now();
      time = now.second();  // TIP: change to now.second for faster debuggingin (run once/minute instead of once/o
    } while(lastminute == time); // Wait for the minute to change
    
    Serial.print(" ");
    Serial.print(time);
  } while (time != 0);  // Wait for minute 0 (once / hour)
  Serial.println();
  
  Serial.println("Reading spectrometer.");
  takeSpecReadings();
  
  Serial.println("Reading spectrometer with shadow bar (");
  int angle = calculateAngle(now);
  Serial.print(angle);
  Serial.println(" degrees).");
  band_motor.step(angle<0? -angle:angle, FORWARD, DOUBLE);
  takeSpecReadings();
  band_motor.step(angle<0? -angle:angle, BACKWARD, DOUBLE);
  
  optic_motor.release();
  band_motor.release();
}

int calculateAngle(DateTime &now) {
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
  return result;
}

void takeSpecReadings() {
  Serial.print("Angle:");
  for(int i=0; i<10; i++) { 
    Serial.print(" ");
    Serial.print(i);
    optic_motor.step(5, FORWARD, DOUBLE);
    delay(5000);
    digitalWrite(SPEC_TRIGGER, HIGH);   // set the spectrometer on
    delay(250);              // wait for a moment
    digitalWrite(SPEC_TRIGGER, LOW);
    delay(2000);
  }
  optic_motor.step(50, BACKWARD, DOUBLE);
  Serial.println();
}

