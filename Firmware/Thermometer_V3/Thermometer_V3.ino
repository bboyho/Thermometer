/*
  Weather Shield Example
  By: Nathan Seidle
  SparkFun Electronics
  Date: June 10th, 2016
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  This example prints the current humidity, air pressure, temperature and light levels.

  The weather shield is capable of a lot. Be sure to checkout the other more advanced examples for creating
  your own weather station.

*/

#include <SPI.h> // Include the Arduino SPI library
// We'll use analog input 0 to measure the temperature sensor's
// signal pin.

// Define the SS pin
//  This is the only pin we can move around to any available
//  digital pin.
const int ssPin = 9;

char tempString[10];  // Will be used with sprintf to create strings

#include <Wire.h> //I2C needed for sensors
#include "SparkFunMPL3115A2.h" //Pressure sensor - Search "SparkFun MPL3115" and install from Library Manager
#include "SparkFunHTU21D.h" //Humidity sensor - Search "SparkFun HTU21D" and install from Library Manager

MPL3115A2 myPressure; //Create an instance of the pressure sensor
HTU21D myHumidity; //Create an instance of the humidity sensor

//Hardware pin definitions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
const byte STAT_BLUE = 7;
const byte STAT_GREEN = 8;

const byte REFERENCE_3V3 = A3;
const byte LIGHT = A1;
const byte BATT = A2;

//Global Variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
long lastSecond; //The millis counter to see when a second rolls by
float temp_h, pressure, tempf, light_lvl, batt_lvl;
int degreesF_int;

void setup()
{
  // -------- SPI initialization
  pinMode(ssPin, OUTPUT);  // Set the SS pin as an output
  digitalWrite(ssPin, HIGH);  // Set the SS pin HIGH
  SPI.begin();  // Begin SPI hardware
  SPI.setClockDivider(SPI_CLOCK_DIV64);  // Slow down SPI clock
  // --------

  // Clear the display, and then turn on all segments and decimals
  clearDisplaySPI();  // Clears display, resets cursor

  // Custom function to send four bytes via SPI
  //  The SPI.transfer function only allows sending of a single
  //  byte at a time.
  s7sSendStringSPI("-HI-");
  setDecimalsSPI(0b111111);  // Turn on all decimals, colon, apos

  // Flash brightness values at the beginning
  setBrightnessSPI(0);  // Lowest brightness
  delay(1500);
  setBrightnessSPI(255);  // High brightness
  delay(1500);

  // Clear the display before jumping into loop
  clearDisplaySPI();

  Serial.begin(9600);
  Serial.println("Weather Shield Example");

  pinMode(STAT_BLUE, OUTPUT); //Status LED Blue
  pinMode(STAT_GREEN, OUTPUT); //Status LED Green

  pinMode(REFERENCE_3V3, INPUT);
  pinMode(LIGHT, INPUT);

  //Configure the pressure sensor
  myPressure.begin(); // Get sensor online
  myPressure.setModeBarometer(); // Measure pressure in Pascals from 20 to 110 kPa
  myPressure.setOversampleRate(7); // Set Oversample to the recommended 128
  myPressure.enableEventFlags(); // Enable all three pressure and temp event flags

  //Configure the humidity sensor
  myHumidity.begin();

  lastSecond = millis();

  Serial.println("Weather Shield online!");
}

void loop()
{
  //Print readings every second
  if (millis() - lastSecond >= 1000)
  {
    digitalWrite(STAT_BLUE, HIGH); //Blink stat LED

    lastSecond += 1000;

    //Check Humidity Sensor
    float humidity = myHumidity.readHumidity();

    if (humidity == 998) //Humidty sensor failed to respond
    {
      Serial.println("I2C communication to sensors is not working. Check solder connections.");

      //Try re-initializing the I2C comm and the sensors
      myPressure.begin();
      myPressure.setModeBarometer();
      myPressure.setOversampleRate(7);
      myPressure.enableEventFlags();
      myHumidity.begin();
    }
    else
    {
      Serial.print("Humidity = ");
      Serial.print(humidity);
      Serial.print("%,");
      temp_h = myHumidity.readTemperature();
      Serial.print(" temp_h = ");
      Serial.print(temp_h, 2);
      Serial.print("C,");

      //Check Pressure Sensor
      pressure = myPressure.readPressure();
      Serial.print(" Pressure = ");
      Serial.print(pressure);
      Serial.print("Pa,");

      //Check tempf from pressure sensor
      tempf = myPressure.readTempF();
      Serial.print(" temp_f = ");
      Serial.print(tempf, 2);
      Serial.print("F,");

      //Check light sensor
      light_lvl = get_light_level();
      Serial.print(" light_lvl = ");
      Serial.print(light_lvl);
      Serial.print("V,");

      //Check batt level
      batt_lvl = get_battery_level();
      Serial.print(" VinPin = ");
      Serial.print(batt_lvl);
      Serial.print("V");

      Serial.println();

      // This custom function works somewhat like a serial.print.
      //  You can send it an array of chars (string) and it'll print
      //  the first 4 characters in the array.
      
      //print temperature based on humidity
      temp_h = temp_h* (9.0 / 5.0) + 32.0; //Convert degrees Celsius to Fahrenheit
      degreesF_int = temp_h * 100;
      Serial.println(degreesF_int);
      sprintf(tempString, "%4d", degreesF_int);

      // This will output the tempString to the S7S
      s7sSendStringSPI(tempString);

      // Print the decimal at the proper spot
      if (tempf > 100)
        setDecimalsSPI(0b00100100);  // Sets digit 3 decimal on
      else
        setDecimalsSPI(0b00100010);
    }

    digitalWrite(STAT_BLUE, LOW); //Turn off stat LED
  }

  delay(100);
}

// This custom function works somewhat like a serial.print.
//  You can send it an array of chars (string) and it'll print
//  the first 4 characters in the array.
void s7sSendStringSPI(String toSend)
{
  digitalWrite(ssPin, LOW);
  SPI.transfer(0x79); // Send the Move Cursor Command
  SPI.transfer(0x00); // Send the data byte between 0 and 3
  for (int i = 0; i < 3; i++)
  {
    SPI.transfer(toSend[i]);
    Serial.print(toSend[i]);
  }
  SPI.transfer(0x7E); // Individual Segment Control for Digit Position 4
  SPI.transfer(0b01110001); // Display an F

  digitalWrite(ssPin, HIGH);

}

// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplaySPI()
{
  digitalWrite(ssPin, LOW);
  SPI.transfer(0x76);  // Clear display command
  digitalWrite(ssPin, HIGH);
}

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightnessSPI(byte value)
{
  digitalWrite(ssPin, LOW);
  SPI.transfer(0x7A);  // Set brightness command byte
  SPI.transfer(value);  // brightness data byte
  digitalWrite(ssPin, HIGH);
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimalsSPI(byte decimals)
{
  digitalWrite(ssPin, LOW);
  SPI.transfer(0x77);
  SPI.transfer(decimals);
  digitalWrite(ssPin, HIGH);
}


//Returns the voltage of the light sensor based on the 3.3V rail
//This allows us to ignore what VCC might be (an Arduino plugged into USB has VCC of 4.5 to 5.2V)
float get_light_level()
{
  float operatingVoltage = analogRead(REFERENCE_3V3);

  float lightSensor = analogRead(LIGHT);

  operatingVoltage = 3.3 / operatingVoltage; //The reference voltage is 3.3V

  lightSensor = operatingVoltage * lightSensor;

  return (lightSensor);
}

//Returns the voltage of the raw pin based on the 3.3V rail
//This allows us to ignore what VCC might be (an Arduino plugged into USB has VCC of 4.5 to 5.2V)
//Battery level is connected to the RAW pin on Arduino and is fed through two 5% resistors:
//3.9K on the high side (R1), and 1K on the low side (R2)
float get_battery_level()
{
  float operatingVoltage = analogRead(REFERENCE_3V3);

  float rawVoltage = analogRead(BATT);

  operatingVoltage = 3.30 / operatingVoltage; //The reference voltage is 3.3V

  rawVoltage = operatingVoltage * rawVoltage; //Convert the 0 to 1023 int to actual voltage on BATT pin

  rawVoltage *= 4.90; //(3.9k+1k)/1k - multiple BATT voltage by the voltage divider to get actual system voltage

  return (rawVoltage);
}
