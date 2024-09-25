// Monarch Instrument - ROS/ROSM/Remote Optical Sensor/Modulated Remote Optical Sensor (Tachometer)
// Datasheet/Manual: https://monarchserver.com/Files/pdf/manuals/1071-4854-126_ROS_ROSM_Manual.pdf
// Description: This program measures the RPM, direction, and detects agitation phase/direction change.
//              Application is to attach to bottom of washer to record a full wash cycle.
//              Attach firmly, apply special tape on rotating item, and point it at said tape.
// Note: Potential issues may stem from millis() overflow (goes back to zero) after approximately 50 days: https://www.arduino.cc/reference/en/language/functions/time/millis/
//       Another issue occurs if one tach hits but rotating item changes direcion, hits the tach, and continues in that direction. It records the wrong direction but is fixed by changing direction again

// HTM2500LF - Temperature and Relative Humidity Module
// Datasheet: https://www.te.com/commerce/DocumentDelivery/DDEController?Action=showdoc&DocId=Data+Sheet%7FHPC169_C%7FA1%7Fpdf%7FEnglish%7FENG_DS_HPC169_C_A1.pdf%7FCAT-HSA0001
// Description: This module will record and measure the temperature and humidity of the inside of a washer during a cycle.
// Note: Humidity already linearized and properly outputted. This is specifically for temperature. Formulas used can be found in datasheet.

// Variable Creation
#define LaserTachInputPin1 18
#define LaserTachInputPin2 19
#define ANALOG_PIN A0 

volatile bool tachHit1;
volatile bool tachHit2;
volatile uint32_t RiseTime1, RiseTime2;
volatile uint32_t timeDiff;
volatile bool updateRPM;

// Defining constants from Steinhart-Hart coefficient to solve for temperature
const float a = 8.54942e-04;
const float b = 2.57305E-04;
const float c = 1.65368E-07;

// Defining constants for resistor reference and voltage source
const float R_ref = 10000.00; 
const float Vcc = 5.0;

// Interrupt Functions for each tach
void Tach_Read1(){ 
  tachHit1 = true;
  
  // Measure RPM
  static uint32_t prev_time;
  uint32_t currentMS = millis();
  timeDiff = currentMS - prev_time;
  RiseTime1 = prev_time = currentMS;
  updateRPM = true;
}
void Tach_Read2(){ 
  tachHit2 = true;
  RiseTime2 = millis();
}

// Setup (runs once in beginning)
void setup() {
  Serial.begin(115200); 

  // Attach interrupts to tachometers
  pinMode(LaserTachInputPin1, INPUT);
  pinMode(LaserTachInputPin2, INPUT);
  attachInterrupt(digitalPinToInterrupt(LaserTachInputPin1),Tach_Read1,RISING);
  attachInterrupt(digitalPinToInterrupt(LaserTachInputPin2),Tach_Read2,RISING);

  // Variable initialization 
  RiseTime1 = RiseTime2 = 0;
  tachHit1 = false;
  tachHit2 = false;
  updateRPM = false;
}

// Loop (runs forever)
void loop() {
  //==========TEMPERATURE MEASUREMENT==========//
  // Reading from analog output and converting to voltage
  int output = analogRead(ANALOG_PIN);
  float voltage = output * (Vcc / 1023.0);
  Serial.println(output);

  // Calculating resistance of thermistor using voltage divider 
  float R_NTC= R_ref * (Vcc / voltage - 1);

  // Converting resistance to temperature in Kelvin --> Based off of formula: 1/T = a + b * ln(R) + C * ln(R)^3
  float tempK = 1/(a+b*log(R_NTC)+c*pow(log(R_NTC), 3));

  // Conversion from Kelvin to Celsius and Farenheit
  float tempC = tempK - 273.15;
  float tempF = tempC * 1.8 + 32;
  //==========TEMPERATURE MEASUREMENT==========//

  //==========RPM & DIRECTION MEASUREMENT==========//
  // State machine and state recorders creation
  enum TachState{WAIT, START, CLOCKWISE, C_CLOCKWISE, AGITATION};
  static TachState tachState;
  static TachState prevState;

  uint32_t currentMS = millis();

  // Rolling average to measure RPM
  static uint32_t timeDiff_sum, timeDiff_avg;
  if(updateRPM){                                                
    updateRPM = false;
    timeDiff_sum = timeDiff_sum + timeDiff - timeDiff_avg;
    timeDiff_avg = timeDiff_sum;
  }

  // State machine
  switch(tachState){

    case WAIT:
    {
      prevState = WAIT;
      tachState = WAIT;
      Serial.println("Waiting");
      if(RiseTime1 > 0 || RiseTime2 > 0) {
        tachState = START;
        }
      break;
    }

    case START:
    {
      if (tachHit1 && tachHit2) { // Detecting if both tachs are hit before deciding direction
        tachHit1 = false;
        tachHit2 = false;
        if(RiseTime1 < RiseTime2) { // Going clockwise
          tachState = CLOCKWISE;
        }
        else if(RiseTime1 > RiseTime2) { // Going counter-clockwise
          tachState = C_CLOCKWISE;
        }
        else {tachState = START;}
      }
      break;
    }

    case CLOCKWISE:
    {
      float rpm = constrain(60000.0/timeDiff_avg, 0, 1500);
      Serial.print("RPM: ");
      Serial.print(rpm);
      Serial.print(" CW | Temp (F): ");
      Serial.print(tempF);
      Serial.print(" | Temp (C): ");
      Serial.println(tempC);

      if (prevState == C_CLOCKWISE) { // Checks for direction change and/or agitation phase
        prevState = CLOCKWISE;
        tachState = AGITATION;
        break;
      }

      prevState = CLOCKWISE;
      tachState = START;
      break;
    }

    case C_CLOCKWISE:
    {
      float rpm = -1 * constrain(60000.0/timeDiff_avg, 0, 1500);
      Serial.print("RPM: ");
      Serial.print(rpm);
      Serial.print(" CCW | Temp (F): ");
      Serial.print(tempF);
      Serial.print(" | Temp (C): ");
      Serial.println(tempC);

      if (prevState == CLOCKWISE) { // Checks for direction change and/or agitation phase
        prevState = C_CLOCKWISE;
        tachState = AGITATION;
        break;
      }

      prevState = C_CLOCKWISE;
      tachState = START;
      break;
    }

    case AGITATION:
    {
      Serial.print("AGITATION DETECTED | ");
      tachState = START;
      break;
    }

    default:
    {
      Serial.println("ERROR: Illegal State");
      break;
    }
    
  }
  //==========RPM & DIRECTION MEASUREMENT==========//
}