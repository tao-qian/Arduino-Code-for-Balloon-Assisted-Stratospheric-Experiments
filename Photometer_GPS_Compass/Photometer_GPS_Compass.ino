#include <SdFat.h>
#include <Time.h>
#include <Wire.h>
#include <LSM303.h>
#include <TinyGPS.h>

/**
 * This is the code to be run on the Arduino Mega used for BASE 2013 photometer project.
 
 * The system basically take readings from 
 * 1) 8 LEDs,
 * 2) LSM303 breakout board for tilt-compensated compass(https://www.sparkfun.com/products/10703),
 * 3) GS407 GPS receiver(https://www.sparkfun.com/products/11466).
 
 * The libarary used for LSM303 can be found at (https://github.com/qtstc/LSM303).
 * The TinyGPS 12 library(http://arduiniana.org/libraries/tinygps/) is slightly modified to allow the encode(char):bool method to return true even when no satellite lock is received.
 * This is achieved by changing the if (_gps_data_good) to if(true) in term_complete:bool.
 * 
 * The LEDs are connecte to pin A8 to A15 of the board.
 * The connections for LSM303 is listed below:
 * LSM303 | Arduino Mega
 * Vin    |    5v
 * GND    |    GND
 * SDA    |    SDA
 * SCL    |    SCL
 *
 * The connections for GS407 is listed below:
 * GS407  | Arduino Mega
 * Vin    |    5v
 * GND    |    GND
 * TX     |    RX1
 *
 * An Arduino SD card shield is also required for more storage. Please note that the shield does not really work with Mega. Some jumper wires are required for it work.
 *
 * The file stored in the SD card is in CSV format.
 *
 * Author: Tao Qian, DePauw University 
 */

#define GPSBAUD  9600
#define TERMBAUD  115200

LSM303 compass;
TinyGPS gps;

//Analog pins for LED readings.
const int IR940_ANALOG = 8;
const int IR830_ANALOG = 9;
const int RED_ANALOG = 10;
const int YELLOW_ANALOG = 11;
const int GREEN_ANALOG = 12;
const int BLUE_ANALOG = 13;
const int VIOLET400_ANALOG = 14;
const int UV351_ANALOG = 15;

//Mode for the SD card shield.
const int chipSelect = 8;
SdFat sd;
SdFile myFile;

void setup() {

  // Sets baud rate of your terminal program
  Serial.begin(TERMBAUD);
  // Sets baud rate of your GPS
  Serial1.begin(GPSBAUD);
  // Initialize SdFat or print a detailed error message and halt
  // Use half speed like the native library.
  // change to SPI_FULL_SPEED for more performance.
  if (!sd.begin(chipSelect, SPI_HALF_SPEED)) sd.initErrorHalt();
  initializeCSVFile();
  initializeCompass();
}

void loop()
{
  //Cannot use delay here. GPS will not work with delay.
  takeData();
}

/**
 * Method for taking data and storing them to the sd card.
 */
void takeData()
{
  while(Serial1.available())     // While there is data on the RX pin...
  {
    char c = Serial1.read();    // load the data into a variable...
    Serial.print(c);
    if(gps.encode(c))      // if there is a new valid sentence...
    {
      openFile();
      writeTime();
      writeLED();
      writeCompass();
      writeGPS();
      myFile.close();       
    }
  }
}

/**
  * Initialize the compass.
  * To be called in setup().
  */
void initializeCompass()
{
  Wire.begin();
  compass.init();
  compass.enableDefault();

  // Calibration values.
  compass.m_min.x = -520; 
  compass.m_min.y = -570; 
  compass.m_min.z = -770;
  compass.m_max.x = +540; 
  compass.m_max.y = +500; 
  compass.m_max.z = 180;
}


/**
 * Initalize the file used to store data.
 * It writes the headers for all columns to the file.
 * To be called in setup().
 */
void initializeCSVFile()
{
  openFile();
  //Initialize the column names.
  myFile.print("Time");
  myFile.println(",IR940,IR830,RED,YELLOW,GREEN,BLUE,VIOLET400,UV351,Pitch,Roll,Heading,GPSTime,Latitude,Longitude,Altitude,Course(degree),Speed(kmph)");
  myFile.close();
  //setTime(hr,min,sec,day,month,yr);
}

void writeTime()
{
  myFile.print(millis());
  myFile.print(",");
}

void writeLED()
{
  int data[8];
  data[0]= analogRead(IR940_ANALOG);
  data[1]= analogRead(IR830_ANALOG);
  data[2]= analogRead(RED_ANALOG);
  data[3]= analogRead(YELLOW_ANALOG);
  data[4]= analogRead(GREEN_ANALOG);
  data[5]= analogRead(BLUE_ANALOG);
  data[6]= analogRead(VIOLET400_ANALOG);
  data[7]= analogRead(UV351_ANALOG);
  for(int i = 0;i<8;i++)
  {
    myFile.print(data[i]);
    myFile.print(",");
  }
}

void writeCompass()
{
  compass.read();
  myFile.print(compass.pitch());
  myFile.print(",");
  myFile.print(compass.roll());
  myFile.print(",");  
  myFile.print(compass.heading());
  myFile.print(",");
}

void writeGPS()
{
  //GPS time.
  int year;
  byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year,&month,&day,&hour,&minute,&second,&hundredths);
  myFile.print(month, DEC); 
  myFile.print("-"); 
  myFile.print(day, DEC);
  myFile.print("_"); 
  myFile.print(hour, DEC);
  myFile.print(":"); 
  myFile.print(minute, DEC);
  myFile.print(":");
  myFile.print(second, DEC); 
  myFile.print(".");
  myFile.print(hundredths, DEC);
  myFile.print(",");
  //latitude and longitude
  float latitude,longitude;
  gps.f_get_position(&latitude,&longitude);
  myFile.print(latitude,5);
  myFile.print(",");
  myFile.print(longitude,5);
  myFile.print(",");
  //altitude
  myFile.print(gps.f_altitude());
  myFile.print(",");
  //course
  myFile.print(gps.f_course());
  myFile.print(",");
  //speed
  myFile.println(gps.f_speed_kmph());  
}

/**
 * Open or create the file in the sd card.
 */
void openFile()
{
  // open the file for write at end like the Native SD library
  if (!myFile.open("data.csv", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("opening test.txt for write failed");
  }
}
