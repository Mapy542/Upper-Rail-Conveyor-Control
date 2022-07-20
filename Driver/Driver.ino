#include <AccelStepperWithDistance.h>

//Pinouts
const byte stepPin = 11;
const byte directionPin = 10;
const byte enablePin = 9;
const byte resetInput = 2; // Dpin 2
const byte lowEye = 3;
const byte rightEye = 4;
const byte leftEye = 5;
const byte alarm = 6;
const byte robotPermit = 7;
const byte condition = 8;

// 1 or AccelStepper::DRIVER means a stepper driver (with Step and Direction pins)
AccelStepper stepper(AccelStepper::DRIVER, stepPin, directionPin);

//control vars
byte state = 0; //0 un initalized, 1 idle, 2 conveying, 3 error
byte alarmstate = 0; //0 off, 1 low alarm, 2 error alarm;
byte transferstate = 0; // 0 both loaded, 1 robot lockout, 2 empty
byte movestate = 0; //0 empty forward, 1 moving forward eye rising edge, 2 moving forward eye falling edge
long alarmfreq = 0; //keep track of what the alarm is doing
unsigned long alarmsettime = 0; //find timedelta for each alarm phase cycle (after .5 secs toggle from high to low depending on alarm)

//Library Documentation-- kinda
//stepper.runSpeed(); //run forever

//stepper.runToPosition(); // blocking line continues when complete
//  if (stepper.distanceToGo() == 0) // get delta
//stepper.moveTo(-stepper.currentPosition()); // set absolute destination
//stepper.run(); //step
//stepper.move(17); //set relative destination
//stepper.stop(); //stops asap


void setup()
{
  Serial.begin(9600);

  pinMode(enablePin, OUTPUT);
  pinMode(resetInput, INPUT_PULLUP);
  pinMode(lowEye, INPUT_PULLUP);
  pinMode(rightEye, INPUT_PULLUP);
  pinMode(leftEye, INPUT_PULLUP);
  pinMode(alarm, OUTPUT);
  pinMode(robotPermit, INPUT_PULLUP);
  pinMode(condition, OUTPUT);

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
  //control
  switch (state) {
    case 0:
      // init
      stepper.stop();
      digitalWrite(enablePin, HIGH);
      digitalWrite(condition, LOW);
      alarmstate = 0;
      transferstate = 0;
      movestate = 2;
      state = 1;
      break;

    case 1:
      //wait for robot to be done  (if robot permit goes low NO MOVING)
      digitalWrite(enablePin, HIGH);
      digitalWrite(condition, HIGH);
      if (!digitalRead(robotPermit) && transferstate == 0) { //detect rising edge of robot permit for takeout
        transferstate = 1;
      }
      if (digitalRead(robotPermit) && transferstate == 1) { //detect falling edge of robot permit
        transferstate = 2;
        state = 2;
      }
      delay(100); //chill out man
      break;

    case 2:
      //run out next conveyor cycle
      digitalWrite(enablePin, LOW);
      digitalWrite(condition, HIGH);
      if (transferstate == 2) { //cycle forward
        stepper.setSpeed(50);
        if (movestate == 0) {
          for (int i = 0; i <= 50; i++) {
            stepper.runSpeed();
          }
          if (digitalRead(leftEye) && digitalRead(rightEye)) { // if we see parts in both then we good
            movestate = 1;
          }
          else { //stuck or missing part
            stepper.stop();
            state = 3;
          }
        }
        if (movestate == 1) { //move forward untill both eyes go empty
          stepper.runSpeed();
          if (!digitalRead(leftEye) && !digitalRead(rightEye)) { //both go empty
            stepper.stop();
            movestate == 2;
          }
        }
        if (movestate == 2) { //finsihed reset states and break case
          movestate == 0;
          transferstate == 0;
          break;
        }
      }
      if (transferstate == 0) { //rehome
        if (digitalRead(leftEye) && digitalRead(rightEye)) { //if both eyes have parts in sight then load like normal
          stepper.setSpeed(50);
          do {
            stepper.runSpeed();
          } while (digitalRead(leftEye) || digitalRead(rightEye));
          stepper.stop();
          movestate = 0;
          state = 1;
          break;
        }
        else {
          stepper.setSpeed(-50);
          for (int i = 0; i <= 50; i++) {
            stepper.runSpeed();
          }
          stepper.stop();
          if (digitalRead(leftEye) && digitalRead(rightEye)) { //there was rails ready to move them forward
          }
          else if ((digitalRead(leftEye) || digitalRead(rightEye)) && !(digitalRead(leftEye) && digitalRead(rightEye))) { //only one detected so bad things
            state = 3;
            stepper.stop();
          }
          else { // there never was ready rail so move the next ones forward
            movestate = 0;
            do {
              stepper.setSpeed(50);
              if (movestate == 0) {
                for (int i = 0; i <= 100; i++) {
                  stepper.runSpeed();
                }
                if (digitalRead(leftEye) && digitalRead(rightEye)) { // if we see parts in both then we good
                  movestate = 1;
                }
                else { //stuck or missing part
                  stepper.stop();
                  state = 3;
                }
              }
              if (movestate == 1) { //move forward untill both eyes go empty
                stepper.runSpeed();
                if (!digitalRead(leftEye) && !digitalRead(rightEye)) { //both go empty
                  stepper.stop();
                  movestate == 2;
                }
                if (movestate == 2) { //finsihed reset states and break case
                  movestate == 0;
                }
              }
            } while (movestate != 2);
            state = 1;
          }
        }
      }
      break;

    case 3:
      //error
      digitalWrite(enablePin, HIGH);
      digitalWrite(condition, LOW);
      stepper.stop();
      alarmstate = 2;
      break;

    default: //how did we get here?
      state = 3;
      break;
  }

  if (!digitalRead(resetInput)) {
    state = 0;
    do {
      delay(5);
    } while (!digitalRead(resetInput));
  }
  if (digitalRead(lowEye) && alarmstate != 2) {
    alarmstate = 1;
  }

  alarmMan();

}
