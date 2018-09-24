/******************************************************************
  SparkFun Inventor's Kit
  Example sketch 07 - TEMPERATURE SENSOR
  Use the "serial monitor" window to read a temperature sensor.
  The TMP36 is an easy-to-use temperature sensor that outputs
  a voltage that's proportional to the ambient temperature.
  You can use it for all kinds of automation tasks where you'd
  like to know or control the temperature of something.
  More information on the sensor is available in the datasheet:
  http://dlnmh9ip6v2uc.cloudfront.net/datasheets/Sensors/Temp/TMP35_36_37.pdf
  Even more exciting, we'll start using the Arduino's serial port
  to send data back to your main computer! Up until now, we've
  been limited to using simple LEDs for output. We'll see that
  the Arduino can also easily output all kinds of text and data.
  This sketch was written by SparkFun Electronics,
  with lots of help from the Arduino community.
  This code is completely free for any use.
  Visit http://learn.sparkfun.com/products/2 for SIK information.
  Visit http://www.arduino.cc to learn about the Arduino.
  Version 2.0 6/2012 MDG
******************************************************************/
/*
   Circuit:
   Arduino -------------- Serial 7-Segment
     5V   --------------------  VCC
     GND  --------------------  GND
      8   --------------------  SS
     11   --------------------  SDI
     13   --------------------  SCK
*/
#include <SPI.h> // Include the Arduino SPI library
// We'll use analog input 0 to measure the temperature sensor's
// signal pin.

// Define the SS pin
//  This is the only pin we can move around to any available
//  digital pin.
const int ssPin = 8;

char tempString[10];  // Will be used with sprintf to create strings

float voltage, degreesC, degreesF; //Declare 3 floating point variables
int degreesF_int;
const int temperaturePin = A0;


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
  Serial.begin(9600); //Initialize serial port & set baud rate to 9600 bits per second (bps)
}


void loop()
{


  voltage = getVoltage(temperaturePin); //Measure the voltage at the analog pin

  degreesC = (voltage - 0.5) * 100.0; // Convert the voltage to degrees Celsius

  degreesF = degreesC * (9.0 / 5.0) + 32.0; //Convert degrees Celsius to Fahrenheit
  
  degreesF_int = degreesF * 100;
  //Now print to the Serial monitor. Remember the baud must be 9600 on your monitor!
  // These statements will print lines of data like this:
  // "voltage: 0.73 deg C: 22.75 deg F: 72.96"

  sprintf(tempString, "%4d", degreesF_int);

  // This will output the tempString to the S7S
  s7sSendStringSPI(tempString);

  // Print the decimal at the proper spot
  if (degreesF_int > 100)
    setDecimalsSPI(0b00100010);  // Sets digit 3 decimal on
  else
    setDecimalsSPI(0b00100100);

  Serial.print("voltage: ");
  Serial.print(voltage);
  Serial.print("  deg C: ");
  Serial.print(degreesC);
  Serial.print("  deg F: ");
  Serial.println(degreesF);

  delay(2500); // repeat 2.5 seconds (change as you wish!)
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


float getVoltage(int pin)   //Function to read and return
//floating-point value (true voltage)
//on analog pin
{

  return (analogRead(pin) * 0.004882814);
  // This equation converts the 0 to 1023 value that analogRead()
  // returns, into a 0.0 to 5.0 value that is the true voltage
  // being read at that pin.
}

// Other things to try with this code:

//   Turn on an LED if the temperature is above or below a value.

//   Read that threshold value from a potentiometer - now you've
//   created a thermostat!
