/*
 * =====================================================================================
 *
 *       Filename:  RoofTopProgram.ino
 *
 *    Description:  This program is to be uploaded onto an Arduino system.
 *                  The program will read information from three sensors to obtain 
 *                  Uv Index, IR Temperature, and Luminosity. 
 *                  The readings will be pulled every second and timestamped to the
 *                  current date. 
 *                  A real time clock gives the current time and date.
 *                  All data is written to a .CSV file, and the name of the file is the
 *                  date the program is run.
 *
 *                  This program uses many third-party libraries available from
 *                  Adafruit (https://github.com/adafruit). These libraries are
 *                  mostly under an Apache License, Version 2.0.
 *
 *                  http://www.apache.org/licenses/LICENSE-2.0
 *
 *        Version:  3.0
 *        Created:  03/29/16 13:59
 *       Revision:  none
 *       Compiler:  Arduino
 *
 *         Author:  Ethan Ritch
 *   Organization:  L&N STEM Academy
 *   Contact: echoritch@outlook.com
 * =====================================================================================
 *  - Data outputs to CSV
 *  - Abbreviated output
 *  - Real Time Clock for filename
 *  - MLX 8511 - UV Sensor
 *  - DS1307 Real Time Clock
 *  - MicroSD Shield
 *  - TSL2561 Luminosity Sensor
 *  - MLX90614 IR Temperature
 * =====================================================================================
 *  Libraries Included:
 *    SD - Arduino Native
 *    RTClib - https://github.com/adafruit/RTClib
 *    ArdusatSDK-master - https://github.com/ArduSat/ArdusatSDK
 * =====================================================================================
 */

/*-----------------------------------------------------------------------------
 *  Includes
 *-----------------------------------------------------------------------------*/
#include <Arduino.h>
#include <Wire.h>
#include <ArdusatSDK.h>
#include "RTClib.h"
#include <SPI.h>
#include <SD.h>
ArdusatSerial mySerial(SERIAL_MODE_HARDWARE_AND_SOFTWARE, 8, 9);

/*-----------------------------------------------------------------------------
 *  Constant Definitions
 *-----------------------------------------------------------------------------*/
// interval, in ms, to wait between readings
#define READ_INTERVAL 1000 

// Pin Definition
#define LED_SERIAL 5

// These pins are used for measurements
#define LDR_Pin A2 // analog pin 2 for LDR photo sensor
#define UVOUT A0 // Output from the UV sensor
#define REF_3V3 A1 // 3.3V power on the Arduino board

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  setup
 *  Description:  This function runs when the Arduino first turns on/resets. This is 
 *                our chance to take care of all one-time configuration tasks to get
 *                the program ready to begin logging data.
 * =====================================================================================
 */

RTC_DS1307 rtc;

void setup() 
{
  mySerial.begin(9600);
  
  if (!rtc.begin()) 
  {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (!rtc.isrunning())
  {
    Serial.println("RTC is NOT running!");
    // Following line sets the RTC to the date & time this sketch was compiled
    // This line sets the RTC with an explicit date & time, for example to set
    // Only uncomment the next line if the RTC is not running and needs to be reset.
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Attempt to start the measurement sensors
  beginInfraredTemperatureSensor();
  beginLuminositySensor();
  beginUVLightSensor();
  
  // initialize the digital pin as outputs for the LED
  pinMode(LED_SERIAL, OUTPUT); 
  
  // More pin initializations
  pinMode(UVOUT, INPUT);
  pinMode(REF_3V3, INPUT);

  Serial.print("Initializing SD card...");

  if (!SD.begin(10)) 
  {
    Serial.println("initialization failed!");
    return;
  }

  mySerial.println("");
  mySerial.println("");
  
  mySerial.println("===Initialized==="); 

  // Read the current date from the RTC
  DateTime now = rtc.now();
  String currentYear = String(now.year(), DEC);
  currentYear.remove(0,2);
  String fileName = String(now.month(), DEC) + "-" + String(now.day(), DEC) + "-" + (currentYear);

  // Check if a file exists with the current date
  // If not, create a new file with the starting Column Labels
  if (!SD.exists(String(fileName) + ".csv"))
  {
    mySerial.println(fileName);
  
    File logFile = SD.open(String(fileName) + ".csv", FILE_WRITE);
    if(logFile)
    {
      logFile.println("Time,UV,IR,Lum");
      logFile.close();
      
      Serial.println("Time,UV,IR,Lum");
    }
    else
    {
      Serial.println("Couldn't access file");
    }
  }
} // END OF FUNCTION: SETUP

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  loop
 *  Description:  After setup runs, this loop function runs until the Arduino loses 
 *                power or resets. The program will go through each sensor and read 
 *                the data. The data wil be placed into a string with a timestamp of 
 *                the time it was read. The string is written on a new line in the CSV
 *                file. 
 * =====================================================================================
 */
void loop() 
{
  // Shorten Sensor Component Names and define variables
  temperature_t temp;
  luminosity_t luminosity;
  uvlight_t uv_light;
  byte byteRead;
  float temp_val;
  float infrared_temp;
  float lum_val;
  float tempF;
  
/*-----------------------------------------------------------------------------
 *  Read Data and print to CSV file and Serial
 *-----------------------------------------------------------------------------*/

  // Read current time and generate timestamp
  DateTime now = rtc.now();

  // Grab Current Time of Day to be used for timestamps
  String timeStamp = String(now.hour(), DEC) + ":" + String(now.minute(), DEC) + ":" + String(now.second(), DEC);
  String currentYear = String(now.year(), DEC);
  currentYear.remove(0,2);
    
  // Read MP8511 UV 
  readUVLight(uv_light);

  // Read MLX Infrared temp sensor and convert to Fahrenheit
  readInfraredTemperature(temp);
  tempF = ((temp.t * 9) / 5) + 32;

  // Read TSL2561 Luminosity
  readLuminosity(luminosity);

  // Organize the readings into a single string
  String dataString = (timeStamp) + "," + String(uv_light.uvindex) + "," + String(tempF) + "," + String(luminosity.lux);
  String fileName = String(now.month(), DEC) + "-" + String(now.day(), DEC) + "-" + (currentYear);

  // Open the file to be written to and write dataString
  File logFile = SD.open(String(fileName) + ".csv", FILE_WRITE);
  if(logFile)
  {
    logFile.println(dataString);
    logFile.close();
    
    mySerial.println(dataString);
  }
  else
  {
    mySerial.println("Couldn't access file");
  }
  delay(READ_INTERVAL);
} // END OF FUNCTION: LOOP

/*-----------------------------------------------------------------------------
 *  End of Loop and Code
 *-----------------------------------------------------------------------------*/
