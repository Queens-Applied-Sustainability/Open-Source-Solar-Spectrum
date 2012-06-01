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

///////////// INSTALLATION-SPECIFIC CONSTANT!
#define LATITUDE 44.2224 // 44.2224 = St. Lawrence College
#define MERIDIAN 75 // nearest prime merdian defining the time zone modify for a specific location
#define LONGITUDE 76.3 // modify for a specific location
#define DEBUG false // set to true for easier debugging, but make sure it's false for production!

// External Libraries
#include <Wire.h>
#include <AFMotor.h>   // http://www.ladyada.net/make/mshield/download.html
#include <Chronodot.h> // http://planetstephanie.net/2011/04/09/chronodot-library-for-arduino/

// Global Constants
#define SPEC_TRIGGER 2 // output pin to trigger a spectrometer reading

AF_Stepper optic_motor(200, 1);  // M1 and M2 on the motor driving sheild
AF_Stepper band_motor(200, 2);   // M3 and M4
Chronodot RTC; // Real-time Clock
DateTime now;
int lastminute, time = 1;

//array to hold the values to find day of year
const int DAYS_OF_MONTHS[12] = {0,31,59,90,120,151,181,212,243,273,304,334};

// Declare functions implemented later
float angleFromHorizon(DateTime &now);
void takeSpecReadings();

void setup() {
  pinMode(SPEC_TRIGGER, OUTPUT);
  Serial.begin(9600);
  Wire.begin();             // needed for motor drivers
  optic_motor.setSpeed(10); // 10 rpm 
  band_motor.setSpeed(10);  // 10 rpm 
  RTC.begin();
  
  // Uncomment the following line to reset the RTC to the uploading computer's time.
  // RTC.adjust(DateTime(__DATE__, __TIME__));
  
  Serial.println("Ready to go!");
}

void loop() {
  
  Serial.print("Waiting until minute 0: ");
  do {
    do {
      lastminute = time;
      now = RTC.now();
      time = DEBUG ? now.second() : now.minute();  // Show info and take readings for seconds and minutes when debugging (instead of minutes and hours when running)
    } while(lastminute == time); // Wait for the minute to change
    Serial.print(" "); Serial.print(time);
  } while (time != 0);  // Wait for minute 0 (once / hour)
  Serial.println();
  
  Serial.println("Reading spectrometer.");
  takeSpecReadings();
  
  Serial.print("Reading spectrometer with shadow bar (");
  float angle = angleFromHorizon(now);
  Serial.print(angle); Serial.println(" degrees).");
  band_motor.step(abs(angle/1.8), FORWARD, DOUBLE);
  takeSpecReadings();
  band_motor.step(abs(angle/1.8), BACKWARD, DOUBLE);
  
  //optic_motor.release();
  band_motor.release();
}

float angleFromHorizon(DateTime &now) {
  int day_of_year = DAYS_OF_MONTHS[now.month()-1] + now.day();     // does not account for leap-years]
  int B = (day_of_year-1)*(360/365);
  int E = (229.2*(0.000075+0.001868*cos(B)-0.032077*sin(B)-0.014615*cos(2*B)-0.04089*sin(2*B)))/60; //eccentricity factor
  int solartime = (now.hour()+now.minute()/60) + 4*(MERIDIAN-LONGITUDE)/60 + E; //correct assuming the clock does not account for daylight savings 
  int hourangle = (solartime-12)*15; //calculates the hour angle based on solar time 
  float declination = 0.40928 * sin(2*PI * (284+day_of_year)/365); // Equation 1.6.1 Solar Engineering of Thermal Processes
  // Equation 1.6.5
  float incidence = asin(cos(LATITUDE*PI/180)*cos(declination)*cos(hourangle) +
                         sin(LATITUDE*PI/180)*sin(declination));
  if (DEBUG) {
    Serial.print("  y: "); Serial.print(now.year());
      Serial.print(" m: "); Serial.print(now.month());
      Serial.print(" d: "); Serial.print(now.day());
      Serial.print(" h: "); Serial.print(now.hour());
      Serial.print(" m: "); Serial.print(now.minute());
      Serial.print(" s: "); Serial.println(now.second());
    Serial.print("  day of year: "); Serial.println(day_of_year);
    Serial.print("  B: "); Serial.println(B);
    Serial.print("  E: "); Serial.println(E);
    Serial.print("  meridian: "); Serial.println(MERIDIAN);
    Serial.print("  longitude: "); Serial.println(LONGITUDE);
    Serial.print("  solartime: "); Serial.println(solartime);
    Serial.print("  hourangle: "); Serial.println(hourangle);
    Serial.print("  declination: "); Serial.println(declination);
    Serial.print("  incidence: "); Serial.println(incidence);
  }
  return incidence*180/PI;
}

void takeSpecReadings() {
  Serial.print("Angle #");
  for(int i=0; i<10; i++) { 
    Serial.print(" "); Serial.print(i);
    optic_motor.step(5, BACKWARD, DOUBLE);
    delay(5000);
    digitalWrite(SPEC_TRIGGER, HIGH); // set the spectrometer on
    delay(250);                       // wait for a moment
    digitalWrite(SPEC_TRIGGER, LOW);
    delay(2000);
  }
  optic_motor.step(50, FORWARD, DOUBLE);
  Serial.println();
}
