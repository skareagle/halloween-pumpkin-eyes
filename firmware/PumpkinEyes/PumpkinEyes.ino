// Pumpkin Eyes - Arduino Nano ESP32 + PCA9685, one SG90 servo per eyeball
// Start sequence (servos one at a time): MIN_ANGLE -> MAX_ANGLE -> midpoint
// (hold 5s after each step). Then keeps picking EYES_PER_MOVE different random
// servos and jumping each to its own random position between MIN_ANGLE and MAX_ANGLE.
// Each new position is at least MIN_CHANGE degrees from that servo's last.
//
// Wiring: PCA9685 SDA -> A4, SCL -> A5, VCC -> 3.3V, GND -> GND.
// Servo power (5V) goes to the PCA9685 screw terminal, not the Nano.
// Needs the "Adafruit PWM Servo Driver Library" (Library Manager).

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

const int SERVO_CHANNELS[] = {0, 1, 2, 3, 4, 5, 6, 7,8, 9, 10, 11}; // one PCA9685 channel per eye
const int SERVO_COUNT = sizeof(SERVO_CHANNELS) / sizeof(SERVO_CHANNELS[0]);
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 150;
const int MIN_CHANGE = 20;      // minimum difference from previous position
const int EYES_PER_MOVE = 3;    // how many different eyes move together
const int START_HOLD_MS = 5000; // hold time for each start sequence position
const int HOLD_MS = 500;        // hold time after each group of random moves
const int STAGGER_MS = 600;     // gap between servos during start up (one full move)

// SG90 pulse range for 0 and 180 degrees. Narrow these if a servo buzzes
// or strains at either end.
const int SERVO_MIN_US = 500;
const int SERVO_MAX_US = 2400;
const int SERVO_FREQ_HZ = 50;

const int START_SEQUENCE[] = {MIN_ANGLE, MAX_ANGLE, (MIN_ANGLE + MAX_ANGLE)/2};
const int START_SEQUENCE_LEN = sizeof(START_SEQUENCE) / sizeof(START_SEQUENCE[0]);

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // default address 0x40
int currentAngles[SERVO_COUNT];

void moveTo(int servo, int target) {
  currentAngles[servo] = target;
  int us = map(target, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
  pwm.writeMicroseconds(SERVO_CHANNELS[servo], us);
}

// Moves servos one at a time, waiting STAGGER_MS between each, so only
// one servo draws movement current at a time.
void moveAllTo(int target) {
  for (int s = 0; s < SERVO_COUNT; s++) {
    moveTo(s, target);
    delay(STAGGER_MS);
  }
}

int pickNextAngle(int previous) {
  int next;
  do {
    next = random(MIN_ANGLE, MAX_ANGLE + 1);
  } while (abs(next - previous) < MIN_CHANGE);
  return next;
}

void setup() {
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ_HZ);
  delay(10);

  // The PCA9685 sends no pulses until a channel is written, so servos stay
  // put until the start sequence reaches them.
  for (int i = 0; i < START_SEQUENCE_LEN; i++) {
    moveAllTo(START_SEQUENCE[i]);
    delay(START_HOLD_MS);
  }
}

// Fills picks[] with count different servo indexes, in random order
// (a partial Fisher-Yates shuffle).
void pickServos(int picks[], int count) {
  int order[SERVO_COUNT];
  for (int s = 0; s < SERVO_COUNT; s++) {
    order[s] = s;
  }
  for (int i = 0; i < count; i++) {
    int j = random(i, SERVO_COUNT);
    int tmp = order[i];
    order[i] = order[j];
    order[j] = tmp;
    picks[i] = order[i];
  }
}

void loop() {
  const int count = min(EYES_PER_MOVE, SERVO_COUNT);
  int picks[SERVO_COUNT];
  pickServos(picks, count);

  for (int i = 0; i < count; i++) {
    moveTo(picks[i], pickNextAngle(currentAngles[picks[i]]));
  }
  delay(HOLD_MS);
}
