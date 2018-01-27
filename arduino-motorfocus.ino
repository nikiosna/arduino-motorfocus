//https://www.arduinolibraries.info/libraries/accel-stepper
//http://www.hobby-werkstatt-blog.de/arduino/357-schrittmotor-28byj48-am-arduino.php

//https://learn.adafruit.com/tmp36-temperature-sensor/overview
#define tempsensor A1

#include <SoftwareSerial.h>
#include <AccelStepper.h>
#include <EEPROM.h>

SoftwareSerial debugSerial(6, 7);

struct data{
  long currentPosition;
  int compensationvalue;
  int isEEPROMinitialized;
} data;

const int stepsPerRevolution = 32*64;  // change this to fit the number of steps per revolution
const int maxSpeed = 10;
const int maxCmd = 8;

// initialize the stepper library on pins 8 through 11:
AccelStepper stepper(AccelStepper::FULL4WIRE, 8, 9, 10, 11, false);

// multiplier of SPEEDMUX, currently max speed is 480.
int speedFactor = 16;
int speedFactorRaw = 2;
int speedMult = 30;

long steps = 8;
long currentPosition = 0;
long currentPositionTemp = 0;
long targetPosition = 0;
long lastSavedPosition = 0;
long millisLastMove = 0;
const long millisDisableDelay = 15000;
bool isRunning = false;

boolean tempcompensation = false;
long millsLastTempCompensation = 0;
const long millisDisableTempCompensation = 2000;
int compensationvalue;
float temperature;
float temperatureLast;
float a = 1/2.05;   //Sensorvalue to Temperature TMP36
float b = -50;

// read commands
bool eoc = false;
String line;

void setup() {
  Serial.begin(9600);
  debugSerial.begin(9600);


  // initalize motor
  stepper.setMaxSpeed(speedFactor * speedMult);
  stepper.setAcceleration(10);
  millisLastMove = millis();
    
  // read data from EEPROM
  debugSerial.print("Load last position from EEPROM...");
  EEPROM.get(0, data);
  if(data.isEEPROMinitialized != 1) {
    debugSerial.print("EEPROM not valid resetting...");
    data.currentPosition = 0;
    data.compensationvalue = 1;
    data.isEEPROMinitialized = 1;
    EEPROM.put(0, data);
    EEPROM.get(0, data);
  }
  currentPosition = data.currentPosition;
  compensationvalue = data.compensationvalue;
  
  stepper.setCurrentPosition(currentPosition);
  lastSavedPosition = currentPosition;
  debugSerial.println(lastSavedPosition);

  temperature = (analogRead(tempsensor)*a)+b;
  temperatureLast = temperature;
}

void loop() {
  temperature = (analogRead(tempsensor)*a)+b;
  
  // process the command we got
  if (eoc) {
    String cmd, param;
    int len = line.length();
    if (len >= 2) {
      cmd = line.substring(0, 2);
    }
    if (len > 2) {
      param = line.substring(2);
    }

    line = "";
    eoc = false;

    // LED backlight value, always return "00"
    if (cmd.equalsIgnoreCase("GB")) {
      Serial.print("00#");
    }
    // home the motor, hard-coded, ignore parameters since we only have one motor
    else if (cmd.equalsIgnoreCase("PH")) { 
      stepper.setCurrentPosition(8000);
      stepper.moveTo(0);
      isRunning = true;
    }
    // firmware value, always return "10"
    if (cmd.equalsIgnoreCase("GV")) {
      Serial.print("10#");
    }
    // get the current motor position
    else if (cmd.equalsIgnoreCase("GP")) {
      currentPosition = stepper.currentPosition();
      char tempString[6];
      sprintf(tempString, "%04X", currentPosition);
      Serial.print(tempString);
      Serial.print("#");
    }
    // get the new motor position (target)
    else if (cmd.equalsIgnoreCase("GN")) {
      char tempString[6];
      sprintf(tempString, "%04X", targetPosition);
      Serial.print(tempString);
      Serial.print("#");
    }
    // get the current temperature, hard-coded
    else if (cmd.equalsIgnoreCase("GT")) {
      char tempString[6];
      sprintf(tempString, "%04X", (long)temperature);
      Serial.print(tempString);
      Serial.print("#");
    }

    // get the temperature coefficient
    else if (cmd.equalsIgnoreCase("GC")) {
      char tempString[6];
      sprintf(tempString, "%02X", (long)compensationvalue);
      Serial.print(tempString);
      Serial.print("#");
    }

    // set the temperature coefficient
    else if (cmd.equalsIgnoreCase("SC")) {
      long test = hexstr2long(param);
      debugSerial.print("test SC:");
      debugSerial.print(param);
      debugSerial.print(" = ");
      debugSerial.println(test*16);
    }
    
    // get the current motor speed, only values of 02, 04, 08, 10, 20
    else if (cmd.equalsIgnoreCase("GD")) {
      char tempString[6];
      sprintf(tempString, "%02X", speedFactorRaw);
      Serial.print(tempString);
      Serial.print("#");

      debugSerial.print("current motor speed: ");
      debugSerial.println(tempString);
    }
    
    // set speed, only acceptable values are 02, 04, 08, 10, 20
    else if (cmd.equalsIgnoreCase("SD")) {
      speedFactorRaw = hexstr2long(param);

      // SpeedFactor: smaller value means faster
      speedFactor = 32 / speedFactorRaw;
      stepper.setMaxSpeed(speedFactor * speedMult);
    }
    
    // whether half-step is enabled or not, always return "00"
    else if (cmd.equalsIgnoreCase("GH")) {
      Serial.print("00#");
    }
    
    // motor is moving - 01 if moving, 00 otherwise
    else if (cmd.equalsIgnoreCase("GI")) {
      if(abs(targetPosition - currentPosition) > 0){
        Serial.print("01#");
      } else {
        Serial.print("00#");
      }
    }  
    // set current motor position
    else if (cmd.equalsIgnoreCase("SP")) {
      currentPosition = hexstr2long(param) * 16;
      stepper.setCurrentPosition(currentPosition);      
    }
    // set new motor position
    else if (cmd.equalsIgnoreCase("SN")) {
      debugSerial.print("new target position ");
      debugSerial.print(targetPosition);
      debugSerial.print(" -> ");
      targetPosition = hexstr2long(param) * 16;
      debugSerial.println(targetPosition);
    }
    // initiate a move
    else if (cmd.equalsIgnoreCase("FG")) {
      stepper.enableOutputs();
      stepper.moveTo(targetPosition);
    }
    // stop a move
    else if (cmd.equalsIgnoreCase("FQ")) {
      stepper.stop();
    }
    else if (cmd.equals("")){;}
    //unknown command
    else {
      debugSerial.print(cmd);
      debugSerial.print(":");
      debugSerial.println(param);
      debugSerial.println(line);
    }
  }

  //calculate the distance to go for the temperature compensation
  if (tempcompensation == true && (millis() - millsLastTempCompensation) > millisDisableTempCompensation) {
    float tempchange = temperature - temperatureLast;
    if(abs(tempchange) >= 1) {
      debugSerial.print("tempchange: ");
      debugSerial.print(tempchange);
      debugSerial.print("   currentpos: ");
      debugSerial.println(stepper.currentPosition());

      stepper.moveTo(stepper.currentPosition() + tempchange*compensationvalue);
      
      temperatureLast = temperature;
      millsLastTempCompensation = millis();
    }
  }

  // move motor if not done
  if (stepper.distanceToGo() != 0) {    
    isRunning = true;
    millisLastMove = millis();
    stepper.run();
  }
  else {
    if (isRunning) {
      isRunning = false;
      //update current position when stopped
      currentPosition = stepper.currentPosition();
    }
    if(millis() - millisLastMove > millisDisableDelay){
      // Save current location in EEPROM
      if (lastSavedPosition != currentPosition) {
        data.currentPosition = currentPosition;
        EEPROM.put(0, data);
        lastSavedPosition = currentPosition;
        debugSerial.println("Save last position to EEPROM");
        stepper.disableOutputs();
        debugSerial.println("Disabled output pins");
      }
    }
  }
}

void serialEvent () {
  // read the command until the terminating # character
  while (Serial.available() && !eoc) {
    char c = Serial.read();
    if (c != '#' && c != ':') {
      line = line + c;
    }
    else {
      if (c == '#') {
        eoc = true;
      }
    }
  }
}

long hexstr2long(String line) {
  char buf[line.length()+1];
  line.toCharArray(buf, line.length());
  long ret = 0;

  ret = strtol(buf, NULL, 16);
  return (ret);
}
