/*******************************************************************************

This is the control firmware for Gamma Bench's Environmentals Display and
Datalogger (EDL).

Created by Grant Giesbrecht
9 August 2019

*******************************************************************************/

#include <Wire.h> //For sensor
#include <SPI.h> //For Sensor
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SPI.h> //For SD Card
#include "SdFat.h" //For SD Card
#include "RTClib.h" //For RTC

//Define errors
#define ERROR_RTC 'R' //RTC failure
#define ERROR_SD 'S' //uSD failure
#define ERROR_SENS 'B' //Sensor failure
#define ERROR_UNDEF 'U' //Undefined error
#define ERROR_FULL 'F' //Full card error

//***************************************************************\\
//******************* Define Pins ********************************\\

//Display Pins
#define DISP_SDA 0 //Serial Data
#define DISP_DCK 1 //Data Clock
#define DISP_CCK 2 //Charlieplexer Clock
#define DISP_DEN 3 //Display Enable
#define DISP_RCK 4 //Register Clock

//MicroSD Card Pins
#define uSD_CS 4 //Chip Select
#define uSD_MOSI 11 //MOSI
#define uSD_MISO 12 //MISO
#define uSD_SCK 13 //SCK

//Interface Pins
#define INT_ERROR_LED 5 //Error Indicator
#define INT_HUMPRES_CTRL 6 //Humidity/Pressure Control (L-> Humidity, H->Pressure)
#define INT_CARD_WRITE_LED 7 //Card Write Indicator
#define INT_EJECT_CTRL 8 //Eject Card Control
#define INT_ERROR_CODE_CTRL 9 //Cycle Error Code Control

//Sensor Pins + RTC Pins
#define I2C_SDA A4 //SDA
#define I2C_SCK A5 //SCK

//********************************************************************************\\
//************************ User Settings *****************************************\\

#define TIME_UPDATE_DISPLAY 1e3 //mS
#define TIME_SAVE_SD_CARD 2.5e3 //mS //9e5 //15*60*1000 mS
#define FILENAME "thorn_data.kv1" //Name of data

bool debug_w_serial = true; //Should typically be off

// ******************************************************************************\\
// **************************** Define Globals **********************************\\

//Define Objects
Adafruit_BME280 sensor; // I2C
RTC_DS1307 rtc; //I2C
SdFat sd; //SPI
SdFile kvFile;

//Define other globals
bool displayPressure = false;
bool cardWriteAllowed = true;
signed int displayErrorCode = -1;
unsigned long last_disp_time = millis();
unsigned long last_write_time = millis();
float temp, hum, pres;
DateTime t;

//Define error codes
String errors("");

void setup(){

  errors = "";
  unsigned status;

  //************ Init Serial (if set) ***********\\

  if (debug_w_serial){

    // Wait for serial connection
    Serial.begin(9600);
    while (!Serial) {
      SysCall::yield();
    }
    delay(500);

    //Wait for character input
    Serial.println(F("Type any character to start"));
    while (!Serial.available()) {
      SysCall::yield();
    }

  }

  //************** Set up SD Card Module ***********\\

  // Initialize at the highest speed supported by the board that is
  // not over 50 MHz. Try a lower speed if SPI errors occur.
  if (!sd.begin(uSD_CS, SD_SCK_MHZ(10))) {
    sd.initErrorHalt();
  }

  if (sd.exists(FILENAME)){ //If exists...
    if (!kvFile.open(FILENAME, O_APPEND)) { //Append to existing file
      sd.errorHalt("Failed to open file");
    }
  }else{ //If does not exit...
    if (!kvFile.open(FILENAME, O_CREAT | O_WRITE | O_EXCL)) { //Create new file
      sd.errorHalt("Failed to open file");
    }
    kvFile.print(F("#VERSION 1.0\n\n#HEADER\nEnvironmental data collected at Giesbrecht's Gamma Bench\n#HEADER\n\nm<s> \tm<d> \tm<d> \tm<d>\nTime \tTemp_C \tPres_Pa \tHum_prcnt\n"));
  }

  //************** Set up Sensor Module ************\\

  status = sensor.begin();
  if (!status) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring, address, sensor ID!"));
    Serial.print(F("SensorID was: 0x"));
    ; Serial.println(sensor.sensorID(), 16);
    Serial.print(F("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n"));
    Serial.print(F("   ID of 0x56-0x58 represents a BMP 280,\n"));
    Serial.print(F("        ID of 0x60 represents a BME 280.\n"));
    Serial.print(F("        ID of 0x61 represents a BME 680.\n"));
    while (1);
  }

  Serial.println();

  //************* Set up RTC Module *****************\\

  rtc.begin();

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  //************* Set Pin Modes ******************\\

  //pinMode();
  pinMode(DISP_SDA, OUTPUT);
  pinMode(DISP_DCK, OUTPUT);
  pinMode(DISP_CCK, OUTPUT);
  pinMode(DISP_DEN, OUTPUT);
  pinMode(DISP_RCK, OUTPUT);

  //Interface Pins
  pinMode(INT_ERROR_LED, OUTPUT);
  pinMode(INT_CARD_WRITE_LED, OUTPUT);
  pinMode(INT_EJECT_CTRL, INPUT);
  pinMode(INT_ERROR_CODE_CTRL, INPUT);

}

unsigned long temp_time;
void loop(){



  //************ Look for Button Press/Read Controls ******************\\

  //Read Humidity/Pressure Setting
  if (digitalRead(INT_HUMPRES_CTRL) == HIGH){ //Pressure
    displayPressure = true;
  }else{ //Humidity
    displayPressure = false;
  }

  //Check for Eject Button
  if (digitalRead(INT_EJECT_CTRL) == HIGH){ //Eject Card (disable future writes)
    cardWriteAllowed = false;
    kvFile.close();
    if (debug_w_serial){
      Serial.println("Card ejected");
    }

  }

  //Check for Error Code Cycle Button
  if (digitalRead(INT_ERROR_CODE_CTRL) == HIGH){ //Show error code on humidity/pressure display
    displayErrorCode++;
    if (displayErrorCode > errors.length()) displayErrorCode = -1; //Update error to display
  }

  //************* UPDATE DISPLAYS (AFTER ENOUGH TIME) ******************\\

  //Check if the display needs to be updated
  temp_time = millis();
  if (( temp_time - last_disp_time > TIME_UPDATE_DISPLAY) || last_disp_time > temp_time){

    //read data
    temp = sensor.readTemperature();
    pres = sensor.readPressure();
    hum = sensor.readHumidity();
    t = rtc.now(); //Jetzt

    //Check if need to display error codes

    //Update display (This goes here :) ).

    last_disp_time = millis(); //Update timer

    //Send data to serial monitor if specified
    if (debug_w_serial){
      Serial.println("Time: "+ rtc.now().timestamp() + " Temp: " + String(sci(temp, 4)) + " C Hum: " + String(sci(hum, 4)) + " % Pres: " + String(sci(pres, 4)) + " atm");
    }
  }

  //************* RECORD DATA (AFTER ENOUGH TIME) ************************\\

  //Check if you should write data to uSD card
  if (temp_time - last_write_time > TIME_SAVE_SD_CARD || last_write_time > temp_time){

    //read data
    temp = sensor.readTemperature();
    pres = sensor.readPressure();
    hum = sensor.readHumidity();
    t = rtc.now(); //Jetzt

    last_write_time = millis(); //Update timer

    //Save data
    if (cardWriteAllowed){
      if (debug_w_serial) Serial.println("\tWriting to MicroSD Card");
      kvFile.print(t.timestamp() + " \t" + String(sci(temp, 4)) + " \t" + String(sci(pres, 4)) + " \t" + String(sci(hum, 4)) + "\n");
    }

  }



}


/*
 * function 'sci()' taken from Rob Tillaart. Fork his delightful code on github :)
 *
 *  https://github.com/RobTillaart/Arduino/tree/master/libraries/MathHelpers
 *
 */
char __mathHelperBuffer[16];

char * sci(double number, int digits)
{
  int exponent = 0;
  int pos = 0;

  // Handling these costs 13 bytes RAM
  // shorten them with N, I, -I ?
  if (isnan(number))
  {
    strcpy(__mathHelperBuffer, "nan");
    return __mathHelperBuffer;
  }
  if (isinf(number))
  {
    if (number < 0) strcpy(__mathHelperBuffer, "-inf");
    strcpy(__mathHelperBuffer, "inf");
    return __mathHelperBuffer;
  }

  // Handle negative numbers
  bool neg = (number < 0.0);
  if (neg)
  {
    __mathHelperBuffer[pos++] = '-';
    number = -number;
  }

  while (number >= 10.0)
  {
    number /= 10;
    exponent++;
  }
  while (number < 1 && number != 0.0)
  {
    number *= 10;
    exponent--;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (uint8_t i = 0; i < digits; ++i)
  {
    rounding *= 0.1;
  }
  number += rounding;
  if (number >= 10)
  {
    exponent++;
    number /= 10;
  }


  // Extract the integer part of the number and print it
  uint8_t d = (uint8_t)number;
  double remainder = number - d;
  __mathHelperBuffer[pos++] = d + '0';   // 1 digit before decimal point
  if (digits > 0)
  {
    __mathHelperBuffer[pos++] = '.';  // decimal point TODO:rvdt CONFIG?
  }


  // Extract digits from the remainder one at a time to prevent missing leading zero's
  while (digits-- > 0)
  {
    remainder *= 10.0;
    d = (uint8_t)remainder;
    __mathHelperBuffer[pos++] = d + '0';
    remainder -= d;
  }


  // print exponent
  __mathHelperBuffer[pos++] = 'E';
  neg = exponent < 0;
  if (neg)
  {
    __mathHelperBuffer[pos++] = '-';
    exponent = -exponent;
  }
  else __mathHelperBuffer[pos++] = '+';


  // 3 digits for exponent;           // needed for double
  // d = exponent / 100;
  // __mathHelperBuffer[pos++] = d + '0';
  // exponent -= d * 100;

  // 2 digits for exponent
  d = exponent / 10;
  __mathHelperBuffer[pos++] = d + '0';
  d = exponent - d*10;
  __mathHelperBuffer[pos++] = d + '0';

  __mathHelperBuffer[pos] = '\0';

  return __mathHelperBuffer;
}
