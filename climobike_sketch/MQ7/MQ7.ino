// http://davidhoulding.blogspot.com/2014/03/co-carbon-monoxide-gas-sensor-using.html

#define VOLTAGE_REGULATOR_DIGITAL_OUT_PIN 8
#define MQ7_ANALOG_IN_PIN 0

#define MQ7_HEATER_5_V_TIME_MILLIS 60000
#define MQ7_HEATER_1_4_V_TIME_MILLIS 90000

#define GAS_LEVEL_READING_PERIOD_MILLIS 1000

unsigned long startMillis;
unsigned long switchTimeMillis;
boolean heaterInHighPhase;

void setup() {
  Serial.begin(19200);

  pinMode(VOLTAGE_REGULATOR_DIGITAL_OUT_PIN, OUTPUT);

  startMillis = millis();

  turnHeaterHigh();

  Serial.println("Elapsed Time (s), Gas Level");
}

void loop() {
  if (heaterInHighPhase) {
    // 5v phase of cycle. see if need to switch low yet
    if (millis() > switchTimeMillis) {
      turnHeaterLow();
    }
  }
  else {
    // 1.4v phase of cycle. see if need to switch high yet
    if (millis() > switchTimeMillis) {
      turnHeaterHigh();
    }
  }

  readGasLevel();
  delay(GAS_LEVEL_READING_PERIOD_MILLIS);
}

void turnHeaterHigh() {
  // 5v phase
  digitalWrite(VOLTAGE_REGULATOR_DIGITAL_OUT_PIN, LOW);
  heaterInHighPhase = true;
  switchTimeMillis = millis() + MQ7_HEATER_5_V_TIME_MILLIS;
}

void turnHeaterLow() {
  // 1.4v phase
  digitalWrite(VOLTAGE_REGULATOR_DIGITAL_OUT_PIN, HIGH);
  heaterInHighPhase = false;
  switchTimeMillis = millis() + MQ7_HEATER_1_4_V_TIME_MILLIS;
}

void readGasLevel() {
  unsigned int gasLevel = analogRead(MQ7_ANALOG_IN_PIN);
  unsigned int time = (millis() - startMillis) / 1000;

  Serial.print(time);
  Serial.print(",");
  Serial.println(gasLevel);
}
