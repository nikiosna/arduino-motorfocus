//https://github.com/laurb9/StepperDriver
//https://www.omc-stepperonline.com/geared-stepper-motor/nema-17-stepper-motor-bipolar-l33mm-w-gear-raio-271-planetary-gearbox-17hs13-0404s-pg27.html

#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "DRV8825.h"

#define RPM 2 //speed
#define ENABLE 7
#define DIR 5
#define STEP 6
#define MOTOR_STEPS 5370 //200*26.85
DRV8825 stepper(MOTOR_STEPS, DIR, STEP, ENABLE);

int direction = 1; //switch this from 1 to -1 to inverse direction

long currentPosition = 0;
long targetPosition = 0;

// read commands
const int maxCmd = 8;
bool eoc = false;
String line;

void setup() {
  Serial.begin(9600);

  // initalize motor
  stepper.begin(RPM, 2);
  stepper.enable();
  
  currentPosition = 10000; 
  targetPosition = currentPosition;
}

void loop() {

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
      targetPosition = -1*currentPosition;
    }
    // firmware value, always return "10"
    if (cmd.equalsIgnoreCase("GV")) {
      Serial.print("10#");
    }
    // get the current motor position
    else if (cmd.equalsIgnoreCase("GP")) {
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
      sprintf(tempString, "%04X", 20.0);
      Serial.print(tempString);
      Serial.print("#");
    }
    // get the temperature coefficient
    else if (cmd.equalsIgnoreCase("GC")) {
      char tempString[6];
      sprintf(tempString, "%02X", 20.0);
      Serial.print(tempString);
      Serial.print("#");
    }
    // set the temperature coefficient
    else if (cmd.equalsIgnoreCase("SC")) {
      long test = hexstr2long(param);
      //TODO
    }
    // get the current motor speed, only values of 02, 04, 08, 10, 20
    else if (cmd.equalsIgnoreCase("GD")) {
      char tempString[6];
      //TODO
    }
    // set speed, only acceptable values are 02, 04, 08, 10, 20
    else if (cmd.equalsIgnoreCase("SD")) {
      //TODO
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
    }
    // set new motor position
    else if (cmd.equalsIgnoreCase("SN")) {
      targetPosition = hexstr2long(param) * 16;
    }
    // initiate a move
    else if (cmd.equalsIgnoreCase("FG")) {
      stepper.enable();
      stepper.move((currentPosition - targetPosition)*direction);
      currentPosition = targetPosition;
    }
    // stop a move
    else if (cmd.equalsIgnoreCase("FQ")) {
      stepper.disable();
    }
    else if (cmd.equals("")){;}
    //unknown command
    else {
    }
  }

  //...
  
}

void serialEvent() {
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
