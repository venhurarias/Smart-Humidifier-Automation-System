#include <Streaming.h>
// #include <dht11.h>
#include <Chrono.h>
#include <avr/wdt.h>
#include <EEPROM.h>

#define TEST_PIN A0

#define KNOB_PIN A5

#define LIMIT_SWITCH_1_PIN 8
#define LIMIT_SWITCH_2_PIN 9

// #define RAIN_SENSOR_PIN A5
// #define DHT_PIN A4

#define HUMIDIFIER_PIN 6
#define PUMP_1_PIN 3
#define PUMP_2_PIN 4
#define DC_FAN_PIN 5
#define LED_PIN 13

#define humidLevel 800
#define FAN_DELAY 20000
#define humidifierOffCounterMin 50
#define humidifierOffCounterMax 360
#define humidifierOnCounter 50

// dht11 DHT11;

int mode = 0;
int testingMode = -1;

bool onTestState = false;

Chrono fanChrono, myChrono;
int counter = 0;

void setup() {
  wdt_disable();
  pinMode(TEST_PIN, INPUT_PULLUP);

  pinMode(KNOB_PIN, INPUT);
  
  pinMode(LIMIT_SWITCH_1_PIN, INPUT_PULLUP);
  pinMode(LIMIT_SWITCH_2_PIN, INPUT_PULLUP);


  pinMode(LED_PIN, OUTPUT);

  // pinMode(RAIN_SENSOR_PIN, INPUT);

  pinMode(HUMIDIFIER_PIN, OUTPUT), digitalWrite(HUMIDIFIER_PIN, HIGH);
  pinMode(PUMP_1_PIN, OUTPUT), digitalWrite(PUMP_1_PIN, HIGH);
  pinMode(PUMP_2_PIN, OUTPUT), digitalWrite(PUMP_2_PIN, HIGH);
  pinMode(DC_FAN_PIN, OUTPUT), digitalWrite(DC_FAN_PIN, HIGH);

  Serial.begin(9600);
  Serial.setTimeout(100);
  digitalWrite(LED_PIN, HIGH);
  Serial << "BEGIN!" << endl;

  int savedValue;
  EEPROM.get(0, savedValue);
  if (savedValue != 0xFFFF) {
    counter = savedValue;
  }
  EEPROM.get(10, savedValue);
  if (savedValue == 0 || savedValue == 1 || savedValue == 2) {
    mode = savedValue;
  }
  wdt_enable(WDTO_2S);
}

void loop() {

  Serial << F("Limit 1: ") << isOnLimit1() << F("\t\t")
         << F("Limit 2: ") << isOnLimit2() << F("\t\t")
         << F("mode: ") << mode << F("\t\t")
         << F("counter: ") << counter << F("\t\t")<<OffCounter() << endl;

  if (myChrono.hasPassed(1000)) {
    myChrono.restart();
    counter++;
    EEPROM.put(0, counter);
  }
  switch (mode) {
    case 0:
      if (fillTank()) {
        mode = 1;
        EEPROM.put(10, mode);
        myChrono.restart();
        counter = 0;
      }

      break;
    case 1:
      fillTank();
      humidifierOn();
      if (counter >= humidifierOnCounter) {
        mode = 2;
        EEPROM.put(10, mode);
        counter = 0;
      }


      break;
    case 2:
      fillTank();
      humidifierOff();
      if (counter >= OffCounter()) {
        mode = 1;
        EEPROM.put(10, mode);
        counter = 0;
      }

      break;
  }
  wdt_reset();
}

bool onTest() {
  return !digitalRead(TEST_PIN);
}
int OffCounter(){
  return map(analogRead(KNOB_PIN), 0, 1023, humidifierOffCounterMin, humidifierOffCounterMax);
}
void testing() {
  if (Serial.available()) {
    String reading = Serial.readString();
    Serial << reading << endl;

    reading.trim();
    if (reading == "q") {
      Serial << "humidifier On" << endl;
      testingMode = 0;
    } else if (reading == "a") {
      Serial << "humidifier Off" << endl;
      testingMode = 1;
    } else if (reading == "w") {
      Serial << "pump 1 On" << endl;
      testingMode = 2;
    } else if (reading == "s") {
      Serial << "pump 1 Off" << endl;
      testingMode = 3;
    } else if (reading == "e") {
      Serial << "pump 2 On" << endl;
      testingMode = 4;
    } else if (reading == "d") {
      Serial << "pump 2 Off" << endl;
      testingMode = 5;
    }
  }
  switch (testingMode) {
    case 0:
      humidifierOn();
      break;

    case 1:
      humidifierOff();
      break;

    case 2:
      Pump1On();
      break;

    case 3:
      Pump1Off();
      break;

    case 4:
      Pump2On();
      break;

    case 5:
      Pump2Off();
      break;
  }
}

bool fillTank() {
  bool isPump1Done = Pump1On();
  bool isPump2Done = Pump2On();
  return isPump1Done && isPump2Done;
}

void humidifierOn() {
  digitalWrite(HUMIDIFIER_PIN, LOW);
  digitalWrite(DC_FAN_PIN, LOW);
  fanChrono.restart();
}

void humidifierOff() {
  digitalWrite(HUMIDIFIER_PIN, HIGH);
  if (fanChrono.hasPassed(FAN_DELAY)) {
    digitalWrite(DC_FAN_PIN, HIGH);
  }
}

bool Pump1On() {
  if (isOnLimit1()) {
    digitalWrite(PUMP_1_PIN, HIGH);
    return true;
  } else {
    digitalWrite(PUMP_1_PIN, LOW);
    return false;
  }
}

void Pump1Off() {
  digitalWrite(PUMP_1_PIN, HIGH);
}

bool Pump2On() {
  if (isOnLimit2()) {
    digitalWrite(PUMP_2_PIN, HIGH);
    return true;
  } else {
    digitalWrite(PUMP_2_PIN, LOW);
    return false;
  }
}

void Pump2Off() {
  digitalWrite(PUMP_2_PIN, HIGH);
}
bool isOnLimit1() {
  return digitalRead(LIMIT_SWITCH_1_PIN);
}

bool isOnLimit2() {
  return digitalRead(LIMIT_SWITCH_2_PIN);
}

// int isRainDetected() {
//   return analogRead(RAIN_SENSOR_PIN);
// }

// float getHumidity() {
//   DHT11.read(DHT_PIN);
//   return (float)DHT11.temperature;
// }
