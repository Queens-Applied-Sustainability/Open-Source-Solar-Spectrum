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
#define LATITUDE 45.0

// External Libraries
#include <Wire.h>
#include <AFMotor.h>   // http://www.ladyada.net/make/mshield/download.html
#include <Chronodot.h> // http://planetstephanie.net/2011/04/09/chronodot-library-for-arduino/

// Global Constants
#define SPEC_TRIGGER 13 // output pin to trigger a spectrometer reading

Chronodot RTC; // Real-time Clock

AF_Stepper optic_motor(200, 1);  // M1 and M2 on the motor driving sheild
AF_Stepper band_motor(200, 2);   // M3 and M4

// fixme: these should not be global
long previousMillis = 0;      // will store last time LED was updated


//array to hold the values to find day of year
const int DAYS_OF_MONTHS[12] = {0,31,59,90,120,151,181,212,243,273,304,334};

void sense(DateTime &now, boolean with_shadow);
float angleFromHorizon(DateTime &now);
void bar(int angle, uint8_t dir);
void takeSpecReadings();

DateTime now;
int lastminute, time = 1;

void setup() {
  pinMode(SPEC_TRIGGER, OUTPUT);
  Serial.begin(9600);
  Wire.begin();  // needed for motor drivers
  optic_motor.setSpeed(10);  // 10 rpm 
  band_motor.setSpeed(10);  // 10 rpm 
  RTC.begin();
  // Uncomment the following line to reset the RTC to the uploading computer's time.
  // RTC.adjust(DateTime(__DATE__, __TIME__));
  
  Serial.println("Ready to go!");
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
  float angle = angleFromHorizon(now);
  int steps = abs(angle/1.8);
  Serial.print(angle);
  Serial.println(" degrees).");
  band_motor.step(steps, FORWARD, DOUBLE);
  takeSpecReadings();
  band_motor.step(steps, BACKWARD, DOUBLE);
  
  optic_motor.release();
  band_motor.release();
}

float angleFromHorizon(DateTime &now) {
  int day_of_year = DAYS_OF_MONTHS[now.month()-1] + now.day(); // does not accoutnt for leap-years

  // Equation 1.6.1 Solar Engineering of Thermal Processes
  float declination = 0.40928 * sin(2*PI * (284+day_of_year)/365);
  
  // Equation 1.6.5. Note the missing cos(Beta) -- we're assuming solar noon.
  float zenith = acos(cos(LATITUDE*PI/180)*cos(declination) +
                      sin(LATITUDE*PI/180)*sin(declination));
  
  // Convert to degrees from horizon (so not really the zenith at all)
  return 90 - (zenith*180/PI);
}

void takeSpecReadings() {
  Serial.print("Angle #");
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
