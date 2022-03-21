#include <LiquidCrystal.h>

const uint8_t NUMBER_OF_SENSORS = 3;
const uint8_t triggerPins[NUMBER_OF_SENSORS] = {6, 7, 8};
const uint8_t echoPins[NUMBER_OF_SENSORS] = {9, 10, 13};
uint8_t solutionDistances[NUMBER_OF_SENSORS];

float duration, distance;
uint8_t Sensor1Distance;
uint8_t Sensor2Distance;
uint8_t Sensor3Distance;

bool INTERACTION_SOLVED, INTERACTION_RUNNING;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();
  randomSeed(analogRead(A6));
  for (uint8_t i = 0; i < NUMBER_OF_SENSORS; i++) {
    pinMode(triggerPins[i], OUTPUT);
  }
  for (uint8_t i = 0; i < NUMBER_OF_SENSORS; i++) {
    pinMode(echoPins[i], INPUT);
  }
  pinMode(A0, INPUT);
}

void loop() {
  if (!Serial) {
    Serial.begin(9600);
  }
  if (Serial.available()) {
    processSerialMessage();
  }
  if (INTERACTION_SOLVED == false && INTERACTION_RUNNING == true) {
    gameLoop();
  }
}

void gameLoop() {
  Sensor1Distance = getSensorDistance(triggerPins[0], echoPins[0]);
  Sensor2Distance = getSensorDistance(triggerPins[1], echoPins[1]);
  Sensor3Distance = getSensorDistance(triggerPins[2], echoPins[2]);
  printDistanceToLCD();
  if (digitalRead(A0) == HIGH && Sensor1Distance < (solutionDistances[0] + 2) && Sensor1Distance > (solutionDistances[0] - 2) && Sensor2Distance < (solutionDistances[1] + 2) && Sensor2Distance > (solutionDistances[1] - 2) && Sensor3Distance < (solutionDistances[2] + 2) && Sensor3Distance > (solutionDistances[2] - 2)) {
    checkWinning();
  } else if (digitalRead(A0) == HIGH) {
    lcd.setCursor(0, 1);
    lcd.print("Wrong Distances");
    delay(1500);
    lcd.clear();
    printDistanceToLCD();
  }
}

uint8_t getSensorDistance(uint8_t triggerPin, uint8_t echoPin) {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(7);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration * .0343) / 2;

  return (uint8_t)distance;
}

void printDistanceToLCD() {
  lcd.setCursor(0, 0);
  lcd.print(" X=");
  lcd.print(Sensor1Distance);
  lcd.setCursor(5, 0);
  lcd.print(" Y=");
  lcd.print(Sensor2Distance);
  lcd.setCursor(10, 0);
  lcd.print(" Z=");
  lcd.print(Sensor3Distance);
  lcd.print(" ");
}

void processSerialMessage() {
  const uint8_t BUFF_SIZE = 64; // make it big enough to hold your longest command
  static char buffer[BUFF_SIZE + 1]; // +1 allows space for the null terminator
  static uint8_t length = 0; // number of characters currently in the buffer

  char c = Serial.read();
  if ((c == '\r') || (c == '\n')) {
    // end-of-line received
    if (length > 0) {
      tokenizeReceivedMessage(buffer);
    }
    length = 0;
  } else {
    if (length < BUFF_SIZE) {
      buffer[length++] = c; // append the received character to the array
      buffer[length] = 0; // append the null terminator
    }
  }
}

void tokenizeReceivedMessage(char *msg) {
  const uint8_t COMMAND_PAIRS = 10;
  char* tokenizedString[COMMAND_PAIRS + 1];
  uint8_t index = 0;

  char* command = strtok(msg, ";");
  while (command != 0) {
    char* separator = strchr(command, ':');
    if (separator != 0) {
      *separator = 0;
      tokenizedString[index++] = command;
      ++separator;
      tokenizedString[index++] = separator;
    }
    command = strtok(0, ";");
  }
  tokenizedString[index] = 0;

  processReceivedMessage(tokenizedString);
}

void processReceivedMessage(char** command) {
  if (strcmp(command[1], "START") == 0) {
    startSequence(command[3]);
  } else if (strcmp(command[1], "PAUSE") == 0) {
    pauseSequence(command[3]);
  } else if (strcmp(command[1], "STOP") == 0) {
    stopSequence(command[3]);
  } else if (strcmp(command[1], "INTERACTION_SOLVED_ACK") == 0) {
    setInteractionSolved();
  } else if (strcmp(command[1], "PING") == 0) {
    ping(command[3]);
  } else if (strcmp(command[1], "BAUD") == 0) {
    setBaudRate(atoi(command[3]), command[5]);
  } else if (strcmp(command[1], "SETUP") == 0) {
    Serial.println("COM:SETUP;INT_NAME:Ultrasonic Distance Interaction;BAUD:9600");
    Serial.flush();
  }
}

//TODO: Review This Method once Interaction Is Completed
void startSequence(char* TIMESTAMP) {
  INTERACTION_SOLVED = false;
  INTERACTION_RUNNING = true;
  for (uint8_t i = 0; i < NUMBER_OF_SENSORS; i++) {
    solutionDistances[i] = random(4, 26);
  }
  Serial.print("COM:START_ACK;MSG:X-");
  Serial.print(solutionDistances[0]);
  Serial.print(" Y-");
  Serial.print(solutionDistances[1]);
  Serial.print(" Z-");
  Serial.print(solutionDistances[2]);
  Serial.print(";ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void pauseSequence(char* TIMESTAMP) {
  INTERACTION_RUNNING = !INTERACTION_RUNNING;
  if (INTERACTION_RUNNING) {
    Serial.print("COM:PAUSE_ACK;MSG:Device is now running;ID:");
  } else {
    Serial.print("COM:PAUSE_ACK;MSG:Device is now paused;ID:");
  }
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void stopSequence(char* TIMESTAMP) {
  INTERACTION_RUNNING = false;
  Serial.print("COM:STOP_ACK;MSG:Device is now stopped;ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void setInteractionSolved() {
  INTERACTION_SOLVED = true;
  INTERACTION_RUNNING = false;
}

void ping(char* TIMESTAMP) {
  Serial.print("COM:PING;MSG:PING;ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

void setBaudRate(int baudRate, char* TIMESTAMP) {
  Serial.flush();
  Serial.begin(baudRate);
  Serial.print("COM:BAUD_ACK;MSG:The Baud Rate was set to ");
  Serial.print(baudRate);
  Serial.print(";ID:");
  Serial.print(TIMESTAMP);
  Serial.print("\r\n");
  Serial.flush();
}

//TODO: Change this method in order to validate if the interaction has been solved
bool checkWinning() {
  Serial.println("COM:INTERACTION_SOLVED;MSG:User Positioned Objects Correctly;PNT:750");
  Serial.flush();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Correct Distance");
  setInteractionSolved();
}
