// HTM2500LF - Temperature and Relative Humidity Module
// Datasheet: https://www.te.com/commerce/DocumentDelivery/DDEController?Action=showdoc&DocId=Data+Sheet%7FHPC169_C%7FA1%7Fpdf%7FEnglish%7FENG_DS_HPC169_C_A1.pdf%7FCAT-HSA0001
// Description: This module will record and measure the temperature and humidity of the inside of a washer during a cycle.
// Noote: Humidity already linearized and properly outputted. This is specifically for temperature. Formulas used can be found in datasheet.

#define ANALOG_PIN A0 

// Defining constants from Steinhart-Hart coefficient to solve for temperature
const float a = 8.54942e-04;
const float b = 2.57305E-04;
const float c = 1.65368E-07;

// Defining constants for resistor reference and voltage source
// const float R_ref = 10000.00; 
// const float Vcc = 5.0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  // Reading from analog output and converting to voltage
  int output = analogRead(ANALOG_PIN);
  float voltage = output * (Vcc / 1023.0);

  // Calculating resistance of NTC --> Based off of formula: Vout = (Vcc * NTC) / (Rbatch + NTC) 
  float R_NTC = (R_ref * voltage) / (Vcc - voltage);

  // Calculating resistance of thermistor using voltage divider 
  // float R_NTC= R_ref * (Vcc / voltage - 1);

  // Converting resistance to temperature in Kelvin --> Based off of formula: 1/T = a + b * ln(R) + C * ln(R)^3
  float tempK = 1/(a+b*log(R_NTC)+c*pow(log(R_NTC), 3));

  // Conversion from Kelvin to Celsius and Farenheit
  float tempC = tempK - 273.15;
  float tempF = tempC * 1.8 + 32;

  // Printing temperatures to serial output
  Serial.print("Temperature in Celsius is: ");
  Serial.print(tempC);
  Serial.print(" °C | ");
  Serial.print("Temperature in Farenheit: ");
  Serial.print(tempF);
  Serial.println(" °F");
  
  delay(1000); // Waits 1 second before sampling again
}
