#include <AccelStepperWithDistance.h>

//Pinouts
const byte stepPin = 11;
const byte directionPin = 10;
const byte enablePin = 9;
const byte resetInput = 2; // Dpin 2
const byte lowEye = 3;
const byte rigthEye = 4;
const byte leftEye = 5;
const byte alarm = 6;


// 1 or AccelStepper::DRIVER means a stepper driver (with Step and Direction pins)
AccelStepper stepper(AccelStepper::DRIVER, stepPin, directionPin);

byte state = 0; //0 un initalized, 1 idle, 2 conveying, 3 error
byte alarmstate = 0; //0 off, 1 low alarm, 2 error alarm;
long alarmfreq = 0; //keep track of what the alarm is doing
unsigned long alarmsettime = 0;
void setup()
{
  Serial.begin(9600);

  pinMode(enablePin, OUTPUT);
  pinMode(resetInput, INPUT_PULLUP);
  pinMode(lowEye, INPUT_PULLUP);
  pinMode(rigthEye, INPUT_PULLUP);
  pinMode(leftEye, INPUT_PULLUP);
  pinMode(alarm, OUTPUT);

  //stepper.enableOutputs();
  stepper.setMaxSpeed(1000);
  stepper.setSpeed(50);
  stepper.setAcceleration(100);
}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void alarmMan() {
  switch (alarmstate) {
    case 0:
      noTone(alarm);
      alarmfreq = 0;
      break;

    case 1:
      if (alarmfreq > 1000) { //if we go straight from error alarm to low alarm then reset alarm cycle
        alarmfreq = 0;
      }
      if (alarmfreq == 0) {
        tone(alarm, 800);
        alarmsettime = millis();
        alarmfreq = 800;
      }
      if (alarmfreq == 800 && (millis() - alarmsettime) > 500) { //after 500 milisecs or when available set new tone freq
        tone(alarm, 1000);
        alarmsettime = millis();
        alarmfreq = 1000;
      }
      if (alarmfreq == 1000 && (millis() - alarmsettime) > 500) { //after 500 milisecs or when available set new tone freq
        tone(alarm, 800);
        alarmsettime = millis();
        alarmfreq = 800;
      }
      break;

    case 2:
      if (alarmfreq < 1200) { //if we go straight from error alarm to low alarm then reset alarm cycle
        alarmfreq = 0;
      }
      if (alarmfreq == 0) {
        tone(alarm, 1500);
        alarmsettime = millis();
        alarmfreq = 1500;
      }
      if (alarmfreq == 1500 && (millis() - alarmsettime) > 500) { //after 500 milisecs or when available set new tone freq
        tone(alarm, 2000);
        alarmsettime = millis();
        alarmfreq = 2000;
      }
      if (alarmfreq == 2000 && (millis() - alarmsettime) > 500) { //after 500 milisecs or when available set new tone freq
        tone(alarm, 1500);
        alarmsettime = millis();
        alarmfreq = 1500;
      }
      break;

  }

  if ( millis() > 86400000) {
    resetFunc(); //prevent overwrap time errors, aswell as buildup with a 24hour full restart.
  }
}

void loop()
{
  //stepper.runSpeed();

  //stepper.runToPosition();
  //  if (stepper.distanceToGo() == 0)
  //stepper.moveTo(-stepper.currentPosition());
  //stepper.run();


  //stepper.stop(); //stops asap
  //stepper.run(); //runtoposiiton is option

  //control
  switch (state) {
    case 0:
      // init
      stepper.stop();
      digitalWrite(enablePin, HIGH);
      alarmstate = 0;
      state = 1;
      break;

    case 1:
      //do whatever for idle
      break;

    case 2:
      //run out next conveyor cycle
      break;

    case 3:
      //error
      digitalWrite(enablePin, HIGH);
      alarmstate = 2;
      break;

    default: //how did we get here?
      state = 3;
      break;
  }

  if (digitalRead(resetInput)) {
    state = 0;
  }
  if (digitalRead(lowEye) && alarmstate != 2) {
    alarmstate = 1;
  }
  alarmMan();
}
