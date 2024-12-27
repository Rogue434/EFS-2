//UPDATE 23 AGUSTUS 2024

#include <ModbusMaster.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <Wire.h>



#include <DFRobot_ADS1115.h>

#define Pump 7
#define Pump2 2
#define SwitchPump 5

Adafruit_INA219 ina219;
Adafruit_BME680 bme;
ModbusMaster node;

int currentPumpState, currentPumpSpeed, tempPumpSpeed, currentPumpState2, currentPumpSpeed2;
int error;
int STATE = 0;
int SPEED = 0;

float p10, p25;
float pm25, pm10;
bool bme_is_begin = false;
bool ina219_is_begin = false;
String command;
String currentPM1 = "";
String currentPM2 = "";

bool STATUS[14] = {};
String SemeatechType[41] = { "", "", "CO", "O2", "H2", "CH4", "", "CO2", "O3", "H2S", "SO2", "NH3", "CL2", "ETO", "HCL", "PH3", "", "HCN", "", "HF", "",
                             "NO", "NO2", "NOX", "CLO2", "", "", "", "", "", "", "THT", "C2H2", "C2H4", "CH2O", "", "", "", "", "CH3SH", "C2H3C" };

String SENSOR[14] = {"MEMBRASENS", "NO2", "SO2", "O3", "CO", "SENOVOL", "RIKA", "SENTEC", "WINSEN", "NOVA", "BRAVO", "PM_10", "PM_25", "OPC_N3"};

void writeModbus(byte* data, int panjang, bool CRCtype) {
  int i;
  uint16_t crc = 0xFFFF;
  for (byte p = 0; p < panjang; p++) {
    crc ^= data[p];
    for (i = 0; i < 8; ++i) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc = (crc >> 1);
    }
  }
  if (CRCtype == false) {
    Serial1.write(data, panjang);
    Serial1.write(lowByte(crc));
    Serial1.write(highByte(crc));
  } else {
    Serial1.write(data, panjang);
    Serial1.write(highByte(crc));
    Serial1.write(lowByte(crc));
  }
}
int16_t hexArrayToDecimal(byte hexArray[], int length) {
  int16_t decimalValue = 0;

  for (int i = 0; i < length; i++) {
    decimalValue = (decimalValue << 8) | hexArray[i];
  }

  return decimalValue;
}
void setup() {
  Serial.begin(9600);
  Serial.setTimeout(500);
  Serial2.begin(9600);

  pinMode(Pump, OUTPUT);
  pinMode(SwitchPump, OUTPUT);

  digitalWrite(SwitchPump, HIGH);
  delay(1000);
  digitalWrite(SwitchPump, LOW);
  // softStartPump(100,0);
  Serial.println("Ready");
}

void(* resetFunc) (void) = 0;

void loop() {
  if (Serial.available() > 0) {
    command = Serial.readStringUntil('#');
    // Serial.println("COMMAND : " + command);
    if(command.equals("data.pm.opc")){
      Serial.println("PM_OPC;" + getOPC_N3() + "END_PM_OPC;");
    }
    
    if (command.substring(0, 12).equals("data.metone.")) {
      int addr = command.substring(12, command.length()).toInt();
      Serial.println("METONE;" + getMETONE(addr) + ";" + getMETONEFLOW(addr) + ";" + String(addr) + ";" + "END_METONE;");
    }

    if (command.equals("data.membrasens.ppm")) {
      Serial.println("MEMBRASENS_PPM;" + getMEMBRASENS_PPM() + "END_MEMBRASENS_PPM");
    }
    if (command.equals("data.test")) {
      Serial.println(String() + "TESTOKAY;" + "END_TEST");
    }
    if (command.equals("data.membrasens.current")) {
      Serial.println("MEMBRASENS_CURRENT;" + getMEMBRASENS_CURRENT() + "END_MEMBRASENS_CURRENT");
    }

    if (command.equals("data.membrasens.temp")) {
      Serial.println("MEMBRASENS_TEMP;" + getMEMBRASENS_TEMP() + "END_MEMBRASENS_TEMP");
    }

    if (command.equals("data.membrasens.zero")) {
      Serial.println("MEMBRASENS_PPM;" + getMEMBRASENS_PPM() + "END_MEMBRASENS_PPM");
      setMEMBRASENS_ZERO();
      Serial.println("MEMBRASENS_ZERO;" + getMEMBRASENS_PPM() + "END_MEMBRASENS_ZERO");
    }
    if (command.equals("data.rika.ws")) {
      Serial.println("WEATHER_STATION;" + getWEATHER_STATION() + "END_WEATHER_STATION");
    }
    if (command.equals("data.winsen.flow")) {
      Serial.println("WINSEN_FLOW;" + getFLOW() + "END_WINSEN_FLOW");
    }
    if (command.equals("data.senovol")) {
      Serial.println("SENOVOL;" + getSENOPPB() + ";" + "END_SENOVOL");
    }
    if (command.substring(0, 21).equals("data.membrasens.span.")) {
      int port = command.substring(21, command.indexOf(".", 22)).toInt();
      int span = command.substring(command.lastIndexOf(".") + 1, command.length()).toInt();
      Serial.println("MEMBRASENS_PPM;" + getMEMBRASENS_PPM() + "END_MEMBRASENS_PPM");
      setMEMBRASENS_SPAN(port, span);
      Serial.println("MEMBRASENS_SPAN;" + getMEMBRASENS_PPM() + "END_MEMBRASENS_SPAN");
    }

    if (command.substring(0, 15).equals("data.semeatech.")) {
      int addr_num = command.substring(15, command.length()).toInt();
      Serial.println("SEMEATECH START;");
      byte addr = 0x01;
      int i = 0;
      while (i < addr_num) {
        Serial.println("SEMEATECH 0x" + String(addr, HEX) + ";" + getSemeaTech(addr) + "SEMEATECH 0x" + String(addr, HEX) + " END;");
        i++;
        addr++;
      }
      Serial.println("SEMEATECH FINISH;");
    }
    if (command.substring(0, 20).equals("data.semeatech.zero.")) {
      int addr_num = command.substring(20, command.length()).toInt();
      Serial.println("SEMEATECH ZERO START;");
      byte addr = 0x01;
      int i = 0;
      while (i < addr_num) {
        Serial.println("ZERO SEMEATECH 0x" + String(addr, HEX) + ";" + getSemeaTech(addr) + "SEMEATECH 0x" + String(addr, HEX) + " END ZERO;");
        setSemeaTechZero(addr);
        Serial.println("SEMEATECH 0x" + String(addr, HEX) + ";" + getSemeaTech(addr) + "SEMEATECH 0x" + String(addr, HEX) + " END;");
        i++;
        addr++;
      }
      Serial.println("SEMEATECH ZERO FINISH;");
    }

    if (command.substring(0, 20).equals("data.semeatech.span.")) {
      int addr = command.substring(20, command.indexOf(".", 21)).toInt();
      int span = command.substring(command.lastIndexOf(".") + 1, command.length()).toInt();
      Serial.println("SPAN SEMEATECH 0x" + String(addr, HEX) + ";" + getSemeaTech(addr) + "SEMEATECH 0x" + String(addr, HEX) + " END SPAN;");
      setSemeaTechSpan(addr, span);
      Serial.println("SEMEATECH 0x" + String(addr, HEX) + ";" + getSemeaTech(addr) + "SEMEATECH 0x" + String(addr, HEX) + " END;");
    }
    if (command.equals("data.pm.nova")) {
      error = readPMNOVA(&p25, &p10);
      if (!error) {
        pm25 = p25;
        pm10 = p10;
      }
      Serial.println("PM_NOVA;" + String(pm25) + ";"  + String(pm10) + ";END_PM_NOVA");
    }
    if (command.equals("data.particlecounter")) {
      Serial.println("PARTICLECOUNTER;" + getParticleCounter() + "END_PARTICLECOUNTER");
    }
    if (command.equals("data.pm.1")) {
      Serial.println("PM1;" + getPM1() + ";END_PM1");
    }

    if (command.equals("data.pm.2")) {
      Serial.println("PM2;" + getPM2() + ";END_PM2");
    }

    if (command.equals("data.ina219")) {
      Serial.println("INA219;" + getINA219() + "END_INA219");
    }

    if (command.equals("data.bme")) {
      Serial.println("BME;" + getBME() + "END_BME");
    }

    if (command.equals("data.sentec")) {
      Serial.println("SENTEC;" + getSENTEC() + "END_SENTEC");
    }

    if (command.equals("data.pressure")) {
      Serial.println("PRESSURE;" + getPRESSURE() + "END_PRESSURE");
    }

    if (command.equals("data.pump")) {
      Serial.println("PUMP;" + String(currentPumpSpeed) + ";"  + String(currentPumpState) + ";END_PUMP");
    }

    if (command.substring(0, 9).equals("pump.set.")) {
      int state = command.substring(9, command.indexOf(".", 10)).toInt();
      int speed = command.substring(command.lastIndexOf(".") + 1, command.length()).toInt();
      softStartPump(speed, state);
      Serial.println("PUMP;" + String(currentPumpSpeed) + ";"  + String(currentPumpState) + ";END_PUMP");
    }

    if (command.substring(0, 11).equals("pump.state.")) {
      STATE = command.substring(command.lastIndexOf(".") + 1, command.length()).toInt();
      softStartPumpEFS(SPEED, SPEED, STATE);
      Serial.println(STATE);
      Serial.println(SPEED);
    }

    if (command.substring(0, 11).equals("pump.speed.")) {
      SPEED = command.substring(command.lastIndexOf(".") + 1, command.length()).toInt();
      softStartPumpEFS(SPEED, SPEED, STATE);
      Serial.println(STATE);
      Serial.println(SPEED);
    }

    if (command.substring(0, 10).equals("pump2.set.")) {
     int state = command.substring(10, command.indexOf(".", 11)).toInt();
      int speed = command.substring(command.lastIndexOf(".") + 1, command.length()).toInt();
      softStartPump2(speed, state);
      Serial.println("PUMP2;" + String(currentPumpSpeed2) + ";"  + String(currentPumpState2) + ";END_PUMP");
    }
    if (command.equals("get.alldata")) {
      dataGetAll();
    }
    command = "";
  }
}

void dataGetAll() {
  Serial.println("MEMBRASENS_PPM;" + getMEMBRASENS_PPM() + "END_MEMBRASENS_PPM"); delay(50);
  Serial.println("SEMEATECH START;");
  byte addr = 0x01;
  int i = 0;
  while (i < 4) {
    Serial.println("SEMEATECH 0x" + String(addr, HEX) + ";" + getSemeaTech(addr) + "SEMEATECH 0x" + String(addr, HEX) + " END;");
    i++;
    addr++;
  }
  Serial.println("SEMEATECH FINISH;"); delay(50);
  Serial.println("SENOVOL;" + getSENOPPB() + ";" + "END_SENOVOL"); delay(50);
  Serial.println("WEATHER_STATION;" + getWEATHER_STATION() + "END_WEATHER_STATION"); delay(50);
  Serial.println("SENTEC;" + getSENTEC() + "END_SENTEC"); delay(50);
  Serial.println("WINSEN_FLOW;" + getFLOW() + "END_WINSEN_FLOW"); delay(50);
  error = readPMNOVA(&p25, &p10);
  if (!error) {
    pm25 = p25;
    pm10 = p10;
    STATUS[9] = true;
  }else{
    STATUS[9] = false;
  }
  Serial.println("PM_NOVA;" + String(pm25) + ";"  + String(pm10) + ";END_PM_NOVA"); delay(50);
  Serial.println("PARTICLECOUNTER;" + getParticleCounter() + "END_PARTICLECOUNTER"); delay(50);
  Serial.println("PM1;" + getPM1() + ";END_PM1"); delay(50);
  Serial.println("PM2;" + getPM2() + ";END_PM2"); delay(50);
  Serial.println("PUMP;" + String(currentPumpSpeed) + ";"  + String(currentPumpState) + ";END_PUMP"); delay(50);
  for (int k = 0; k < 14; k++) {
    if (STATUS[k] == false) {
      Serial.println(String() + SENSOR[k] + ";NOT_CONNECT;END_" + SENSOR[k] + ";");
    } else {
      Serial.println(String() + SENSOR[k] + ";CONNECT;END_" + SENSOR[k] + ";");
    }
    STATUS[k] = false;
  }
}

void softStartPumpEFS(int pumpspeed, int pumpstate, int motor) {
  int i;
  if (motor == 0) {
    currentPumpState = pumpstate;
    currentPumpSpeed = pumpspeed;
    analogWrite(Pump, 0);
    analogWrite(Pump2, 0);
    delay(50);
    digitalWrite(SwitchPump, currentPumpState);
    delay(50);
    for (i = 30; i <= pumpspeed; i++) {
      analogWrite(Pump, map(i, 0, 100, 0, 255));
      delay(50);
    }
    Serial.println("PUMP;" + String(currentPumpSpeed) + ";"  + String(currentPumpState) + ";END_PUMP");
  } else if (motor == 1) {
    currentPumpState2 = pumpstate;
    currentPumpSpeed2 = pumpspeed;
    analogWrite(Pump, 0);
    analogWrite(Pump2, 0);
    delay(50);
    digitalWrite(SwitchPump, currentPumpState);
    delay(50);
    for (i = 30; i <= pumpspeed; i++) {
      analogWrite(Pump2, map(i, 0, 100, 0, 255));
      delay(50);
    }
    Serial.println("PUMP;" + String(currentPumpSpeed2) + ";"  + String(currentPumpState2) + ";END_PUMP");
  }

}

void softStartPump(int pumpspeed, int pumpstate) {
  int i;
  currentPumpState = pumpstate;
  currentPumpSpeed = pumpspeed;
  analogWrite(Pump, 0);
  delay(50);
  digitalWrite(SwitchPump, currentPumpState);
  delay(50);
  for (i = 30; i <= pumpspeed; i++) {
    analogWrite(Pump, map(i, 0, 100, 0, 255));
    delay(50);
  }
}

void softStartPump2(int pumpspeed, int pumpstate) {
  int i;
  currentPumpState2 = pumpstate;
  currentPumpSpeed2 = pumpspeed;
  analogWrite(Pump2, 0);
  delay(50);
  digitalWrite(SwitchPump, currentPumpState);
  delay(50);
  for (i = 30; i <= pumpspeed; i++) {
    analogWrite(Pump2, map(i, 0, 100, 0, 255));
    delay(50);
  }
}

String getOPC_N3(){
  Serial1.begin(9600, SERIAL_8N1);
  node.begin(5, Serial1);
  uint8_t i, result1;
  uint16_t data[8];
  String str_return = "";
  float temperature, humidity, flow_rate, PM_1, PM_2_5, PM_10, heater_status;
  result1 = node.readHoldingRegisters(0, 8);
  if (result1 == node.ku8MBSuccess) {
    for (i = 0; i < 8; i++) {
      data[i] = node.getResponseBuffer(i);
    }
    STATUS[13] = true;
  }else{
    STATUS[13] = false;
  }
  float sequence;
  sequence = data[0];
  temperature = sequence / 100;
  sequence = data[1];
  humidity = sequence/100;
  sequence = data[2];
  flow_rate = sequence;
  sequence = data[3];
  PM_1 = sequence / 100;
  sequence = data[4];
  PM_2_5 = sequence / 100;
  sequence = data[5];
  PM_10 = sequence / 100;
  sequence = data[6];
  heater_status = sequence;
  str_return = String(temperature) + ";" + String(humidity) + ";" + String(flow_rate) + ";" + String(PM_1) + ";" + String(PM_2_5) + ";"+ String(PM_10) + ";";

  delay(10);
  Serial1.end();
  return str_return;
}

String getMEMBRASENS_PPM() {
  float board0[4];
  float board1[4];
  String str_return = "";
  Serial1.begin(19200, SERIAL_8E1);
  node.begin(1, Serial1);

  uint8_t i, result1;
  uint16_t data[8];

  union {
    uint32_t j;
    float f;
  } u;

  result1 = node.readHoldingRegisters(1000, 8);

  if (result1 == node.ku8MBSuccess) {
    for (i = 0; i < 8; i++) {
      data[i] = node.getResponseBuffer(i);
    }
    u.j = ((unsigned long)data[1] << 16 | data[0]); board0[0] = u.f;
    u.j = ((unsigned long)data[3] << 16 | data[2]); board0[1] = u.f;
    u.j = ((unsigned long)data[5] << 16 | data[4]); board0[2] = u.f;
    u.j = ((unsigned long)data[7] << 16 | data[6]); board0[3] = u.f;

    for (i = 0; i < 4; i++) {
      str_return = str_return + String(board0[i], 3) + ";";
    }
    STATUS[0] = true;
    delay(10);
  } else {
    STATUS[0] = false;
    for (i = 0; i < 4; i++) {
      str_return = str_return + "0;";
    }
  }

  node.begin(2, Serial1);

  result1 = node.readHoldingRegisters(1000, 8);

  if (result1 == node.ku8MBSuccess) {
    for (i = 0; i < 8; i++) {
      data[i] = node.getResponseBuffer(i);
    }
    u.j = ((unsigned long)data[1] << 16 | data[0]); board1[0] = u.f;
    u.j = ((unsigned long)data[3] << 16 | data[2]); board1[1] = u.f;
    u.j = ((unsigned long)data[5] << 16 | data[4]); board1[2] = u.f;
    u.j = ((unsigned long)data[7] << 16 | data[6]); board1[3] = u.f;

    for (i = 0; i < 4; i++) {
      str_return = str_return + String(board1[i], 3) + ";";
    }
    delay(10);
  } else {
    for (i = 0; i < 4; i++) {
      str_return = str_return + "0;";
    }
  }

  Serial1.end();

  return str_return;
}

String getMEMBRASENS_CURRENT() {
  float board0[4];
  float board1[4];
  String str_return = "";
  Serial1.begin(19200, SERIAL_8E1);
  node.begin(1, Serial1);

  uint8_t i, result1;
  uint16_t data[8];

  union {
    uint32_t j;
    float f;
  } u;

  result1 = node.readHoldingRegisters(1030, 8);

  if (result1 == node.ku8MBSuccess) {
    for (i = 0; i < 8; i++) {
      data[i] = node.getResponseBuffer(i);
    }
    u.j = ((unsigned long)data[1] << 16 | data[0]); board0[0] = u.f;
    u.j = ((unsigned long)data[3] << 16 | data[2]); board0[1] = u.f;
    u.j = ((unsigned long)data[5] << 16 | data[4]); board0[2] = u.f;
    u.j = ((unsigned long)data[7] << 16 | data[6]); board0[3] = u.f;

    for (i = 0; i < 4; i++) {
      str_return = str_return + String(board0[i], 9) + ";";
    }
    delay(10);
  } else {
    for (i = 0; i < 4; i++) {
      str_return = str_return + "0;";
    }
  }

  node.begin(2, Serial1);

  result1 = node.readHoldingRegisters(1030, 8);

  if (result1 == node.ku8MBSuccess) {
    for (i = 0; i < 8; i++) {
      data[i] = node.getResponseBuffer(i);
    }
    u.j = ((unsigned long)data[1] << 16 | data[0]); board1[0] = u.f;
    u.j = ((unsigned long)data[3] << 16 | data[2]); board1[1] = u.f;
    u.j = ((unsigned long)data[5] << 16 | data[4]); board1[2] = u.f;
    u.j = ((unsigned long)data[7] << 16 | data[6]); board1[3] = u.f;

    for (i = 0; i < 4; i++) {
      str_return = str_return + String(board1[i], 9) + ";";
    }
    delay(10);
  } else {
    for (i = 0; i < 4; i++) {
      str_return = str_return + "0;";
    }
  }

  Serial1.end();

  return str_return;
}


String getMEMBRASENS_TEMP() {
  float board0[4];
  float board1[4];
  String str_return = "";
  Serial1.begin(19200, SERIAL_8E1);
  node.begin(1, Serial1);

  uint8_t i, result1;
  uint16_t data[8];

  union {
    uint32_t j;
    float f;
  } u;

  result1 = node.readHoldingRegisters(1070, 8);

  if (result1 == node.ku8MBSuccess) {
    for (i = 0; i < 8; i++) {
      data[i] = node.getResponseBuffer(i);
    }
    u.j = ((unsigned long)data[1] << 16 | data[0]); board0[0] = u.f;
    u.j = ((unsigned long)data[3] << 16 | data[2]); board0[1] = u.f;
    u.j = ((unsigned long)data[5] << 16 | data[4]); board0[2] = u.f;
    u.j = ((unsigned long)data[7] << 16 | data[6]); board0[3] = u.f;

    for (i = 0; i < 4; i++) {
      str_return = str_return + String(board0[i], 6) + ";";
    }
    delay(10);
  } else {
    for (i = 0; i < 4; i++) {
      str_return = str_return + "0;";
    }
  }

  node.begin(2, Serial1);

  result1 = node.readHoldingRegisters(1070, 8);

  if (result1 == node.ku8MBSuccess) {
    for (i = 0; i < 8; i++) {
      data[i] = node.getResponseBuffer(i);
    }
    u.j = ((unsigned long)data[1] << 16 | data[0]); board1[0] = u.f;
    u.j = ((unsigned long)data[3] << 16 | data[2]); board1[1] = u.f;
    u.j = ((unsigned long)data[5] << 16 | data[4]); board1[2] = u.f;
    u.j = ((unsigned long)data[7] << 16 | data[6]); board1[3] = u.f;

    for (i = 0; i < 4; i++) {
      str_return = str_return + String(board1[i], 6) + ";";
    }
    delay(10);
  } else {
    for (i = 0; i < 4; i++) {
      str_return = str_return + "0;";
    }
  }

  Serial1.end();

  return str_return;
}

void setMEMBRASENS_ZERO() {
  Serial1.begin(19200, SERIAL_8E1);
  byte zeroing_bytes_01[] = {0x01, 0x10, 0x04, 0xB0, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFA, 0xC0};
  byte zeroing_bytes_02[] = {0x01, 0x10, 0x04, 0xC4, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4A, 0x70};
  byte zeroing_bytes_11[] = {0x02, 0x10, 0x04, 0xB0, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB9, 0xC1};
  byte zeroing_bytes_12[] = {0x02, 0x10, 0x04, 0xC4, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x71};
  Serial1.write(zeroing_bytes_01, sizeof(zeroing_bytes_01));
  delay(1000);
  Serial1.write(zeroing_bytes_02, sizeof(zeroing_bytes_02));
  delay(1000);
  Serial1.write(zeroing_bytes_11, sizeof(zeroing_bytes_11));
  delay(1000);
  Serial1.write(zeroing_bytes_12, sizeof(zeroing_bytes_12));
  delay(3000);
  Serial1.end();
}

void setMEMBRASENS_SPAN(int port, int span) {
  if (span > 0 && span < 11) {
    int spanAddress = 1230;
    int board;
    if (port < 4) {
      board = 0;
    } else {
      port = port - 4;
      board = 1;
    }

    byte port_bytes[] = {0xCE, 0xD0, 0xD2, 0XD4};
    byte board_bytes[] = {0x01, 0x02};
    byte span_bytes1[] = {0x00, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x41, 0x41, 0x41};
    byte span_bytes2[] = {0x00, 0x80, 0x00, 0x40, 0x80, 0xA0, 0xC0, 0xE0, 0x00, 0x10, 0x20};

    spanAddress = spanAddress + (2 * port);

    Serial1.begin(19200, SERIAL_8E1);

    byte span_start_bytes[] = {board_bytes[board], 0x10, 0x04, 0xB0, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    byte span_end_bytes[] = {board_bytes[board], 0x10, 0x04, 0xBA, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    byte span_bytes[] = {board_bytes[board], 0x10, 0x04, port_bytes[port], 0x00, 0x02, 0x04, 0x00, 0x00, span_bytes1[span], span_bytes2[span]};

    uint16_t crc = calcCRC(span_start_bytes, sizeof(span_start_bytes));
    Serial1.write(span_start_bytes, sizeof(span_start_bytes));
    Serial1.write(lowByte(crc));
    Serial1.write(highByte(crc));
    delay(1000);

    crc = calcCRC(span_bytes, sizeof(span_bytes));
    Serial1.write(span_bytes, sizeof(span_bytes));
    Serial1.write(lowByte(crc));
    Serial1.write(highByte(crc));
    delay(1000);

    crc = calcCRC(span_end_bytes, sizeof(span_end_bytes));
    Serial1.write(span_end_bytes, sizeof(span_end_bytes));
    Serial1.write(lowByte(crc));
    Serial1.write(highByte(crc));
    delay(1000);
    Serial1.end();
  } else {
    Serial.println("MEMBRASENS_SPAN;ERROR;SPAN RANGE =>  1-10;END_MEMBRASENS_SPAN");
  }
}

String getSemeaTech(byte V1) {
  Serial1.end();
  delay(50);
  Serial1.begin(19200, SERIAL_8N1);
  delay(50);
  if (V1 < 1 || V1 > 255) {
    return "NONE";
  }
  int readLength;

  byte buf[20];
  byte getSensorType[] = { 0x3A, V1, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00 };
  byte getSensorValue[] = { 0x3A, V1, 0x03, 0x00, 0x00, 0x06, 0x00, 0x00 };
  String SensorType;

  writeModbus(getSensorType, sizeof(getSensorType), false);
  delay(100);
  if (Serial1.available() > 0) {
    readLength = Serial1.readBytes(buf, 6);
  } else {
    return "NONE";
  }
  if (buf[0] == 0x3A) {
    SensorType = SemeatechType[buf[3]];
  }
  delay(50);
  writeModbus(getSensorValue, sizeof(getSensorValue), false);
  delay(100);
  if (Serial1.available() > 0) {
    readLength = Serial1.readBytes(buf, 20);
  } else {
    return "NONE";
  }
  if (buf[0] == 0x3A) {
    byte convertuG[] = { buf[6], buf[7], buf[8], buf[9] };
    byte convertppb[] = { buf[10], buf[11], buf[12], buf[13] };
    byte converttemp[] = { buf[14], buf[15] };
    byte converthum[] = { buf[16], buf[17] };
    int16_t uG = hexArrayToDecimal(convertuG, 4);
    int16_t ppb = hexArrayToDecimal(convertppb, 4);
    int16_t temp = hexArrayToDecimal(converttemp, 2);
    int16_t hum = hexArrayToDecimal(converthum, 2);
    float ftemp = temp;
    float fhum = hum;
    float ppm = ppb;
    ppm = ppm /1000;
    return SensorType + ";" + uG + ";" + ppb + ";" + ppm + ";" + String((ftemp / 100), 2) + ";" + String((fhum / 100), 2) + ";";
  }
}

void setSemeaTechZero(byte devicecode) {
  byte buf[20];
  int rlen = 0;
  Serial1.begin(115200, SERIAL_8N1);
  Serial1.setTimeout(500);
  byte command_bytes[] = {0x3A, devicecode, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00};
  uint16_t crc = calcCRC(command_bytes, sizeof(command_bytes));
  Serial1.write(command_bytes, sizeof(command_bytes));
  Serial1.write(lowByte(crc));
  Serial1.write(highByte(crc));
  delay(100);
  if (Serial1.available() > 0) {
    rlen = Serial1.readBytes(buf, 10);
    if (rlen == 10 && buf[1] == devicecode) {
      // for(int i=0 ;i<rlen;i++){
      // Serial.print(String(buf[i],HEX));
      // Serial.print("\t");
      // }
      // Serial.println("");
      Serial.print("Zeroing ");
      Serial.print("0x" + String(devicecode, HEX));
      Serial.println(" Success");
    } else {
      Serial1.end();
      Serial.println ("ERROR;");
    }
  }
  Serial1.end();
  return "NONE;";
}

void setSemeaTechSpan(byte devicecode, int span) {
  byte buf[20];
  int rlen = 0;
  Serial1.begin(115200, SERIAL_8N1);
  Serial1.setTimeout(500);
  bool checkspan = true;
  byte command_bytes[] = {0x3A, devicecode, 0x09, 0x00, 0x00, 0x01, 0x00, span};
  uint16_t crc = calcCRC(command_bytes, sizeof(command_bytes));
  Serial1.write(command_bytes, sizeof(command_bytes));
  Serial1.write(lowByte(crc));
  Serial1.write(highByte(crc));
  delay(100);
  if (Serial1.available() > 0) {
    rlen = Serial1.readBytes(buf, 6);
    if (rlen == 6 && buf[0] == 58 && buf[3] == 1) {
      Serial.println("Span Proccess, please wait!!");
      while (checkspan) {
        delay(100);
        rlen = Serial1.readBytes(buf, 6);
        if (rlen == 6 && buf[0] == 58) {
          Serial.print("Span ");
          Serial.print("0x" + String(devicecode, HEX));
          if (buf[3] == 0) {
            Serial.println(" Success");
          }
          if (buf[3] == 2) {
            Serial.println(" Failed");
          }
          checkspan = false;
        }
      }
    } else {
      Serial1.end();
      Serial.println ("ERROR;");
    }
  }
  Serial1.end();
  return "NONE;";
}

uint16_t calcCRC(byte *data, byte panjang)
{
  int i;
  uint16_t crc = 0xFFFF;
  for (byte p = 0; p < panjang; p++)
  {
    crc ^= data[p];
    for (i = 0; i < 8; ++i)
    {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc = (crc >> 1);
    }
  }
  return crc;
}

String getINA219() {
  float busVoltage, current, power;
  if (!ina219_is_begin) {
    ina219.begin();
    ina219_is_begin = true;
  }
  busVoltage = ina219.getBusVoltage_V();
  current = ina219.getCurrent_mA();
  power = busVoltage * (current / 1000);
  return String(busVoltage, 2) + ";" + String(current, 2) + ";" + String(power, 2) + ";";
}

String getBME() {
  String str_return = "";
  if (!bme_is_begin) {
    bme.begin(0x77);
    if (! bme.performReading()) {
      bme.begin(0x76);
    } else {
      bme_is_begin = true;
    }
    if (! bme.performReading()) {
      Serial.println("BME Error!");
    } else {
      bme_is_begin = true;
    }

    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  }



  if (! bme.performReading()) {
    return "0.00;0.00;0.00;";
  }
  str_return = String(bme.temperature, 2) + ";";
  str_return = str_return + String(bme.humidity, 2) + ";";
  str_return = str_return + String((bme.pressure / 100.0), 2) + ";";
  return str_return;
}

String getSENTEC() {
  uint8_t result1;
  String str_return = "";
  delay(50);
  Serial1.begin(4800, SERIAL_8N1);
  node.begin(1, Serial1);
  result1 =  node.readHoldingRegisters(0, 10);

  if (result1 == node.ku8MBSuccess) {
    str_return = node.getResponseBuffer(0);
    delay(10);
    STATUS[7] = true;
  }else{
    STATUS[7] = false;
  }
  Serial1.end();
  return str_return + ";";
}

String getPRESSURE() {
  String str_return = "";
  str_return = (((analogRead(A2) * (7.63 / 1023.0)) / 5) - 0.04) / 0.009;
  delay(50);
  return String(str_return) + ";";
}

String getPM1() {
  String retval = "";
  float board0[4];

  Serial1.begin(9600);

  node.begin(11, Serial1);

  uint8_t i, result1;
  uint16_t data[14];

  union {
    uint32_t j;
    float f;
  } u;

  result1 = node.readInputRegisters(100, 14);

  if (result1 == node.ku8MBSuccess) {
    for (i = 0; i < 14; i++) {
      data[i] = node.getResponseBuffer(i);
    }
    u.j = ((unsigned long)data[1] << 16 | data[0]); board0[0] = u.f;
    u.j = ((unsigned long)data[3] << 16 | data[2]); board0[1] = u.f;
    u.j = ((unsigned long)data[5] << 16 | data[4]); board0[2] = u.f;
    u.j = ((unsigned long)data[13] << 16 | data[12]); board0[3] = u.f;
    delay(10);
    retval = String(board0[0]) + ";" + board0[3];
    STATUS[11] = true;
  }else{
    STATUS[11] = false;
  }
  Serial1.end();
  return retval;
}

String getPM2() {
  String retval = "";
  float board0[4];

  Serial1.begin(9600);

  node.begin(12, Serial1);

  uint8_t i, result1;
  uint16_t data[14];

  union {
    uint32_t j;
    float f;
  } u;

  result1 = node.readInputRegisters(100, 14);

  if (result1 == node.ku8MBSuccess) {
    for (i = 0; i < 14; i++) {
      data[i] = node.getResponseBuffer(i);
    }
    u.j = ((unsigned long)data[1] << 16 | data[0]); board0[0] = u.f;
    u.j = ((unsigned long)data[3] << 16 | data[2]); board0[1] = u.f;
    u.j = ((unsigned long)data[5] << 16 | data[4]); board0[2] = u.f;
    u.j = ((unsigned long)data[13] << 16 | data[12]); board0[3] = u.f;
    delay(10);
    retval = String(board0[0]) + ";" + board0[3];
    STATUS[12] = true;
  }else{
    STATUS[12] = false;
  }
  Serial1.end();
  return retval;
}

String getWEATHER_STATION() {
  Serial1.end();
  delay(50);
  Serial1.begin(9600, SERIAL_8N1);
  delay(50);
  String str_return;
  node.begin(1, Serial1);
  uint8_t result = node.readHoldingRegisters(0, 5);
  if (result == node.ku8MBSuccess) {
    float ws = node.getResponseBuffer(0);
    int wd = node.getResponseBuffer(1);
    float temp = node.getResponseBuffer(2);
    float hum = node.getResponseBuffer(3);
    float pres = node.getResponseBuffer(4);
    str_return = String((ws / 100), 2) + ";" + wd + ";" + String((temp / 10), 1) + ";" + String((hum / 10), 1) + ";" + String((pres / 10), 1) + ";";
  }
  delay(10);
  return str_return;
}

String getFLOW() {
  String str_return;
  int flow_pin = A0;
  float actual_flow ;
  int analog_flow;
  float min_flow = 0.5;
  float max_flow = 4.5;
  int SLM = 2;
  analog_flow = analogRead(flow_pin);
  if(analog_flow > 300){
    STATUS[8] = true;
  }else{
    STATUS[8] = false;
  }
  actual_flow = SLM * ((analog_flow - min_flow) / (max_flow - min_flow));
  actual_flow = map(actual_flow, 114, 228, 0, 1500);
  str_return = String(actual_flow);
  delay(10);
  return str_return;
}

int readPMNOVA(float *p25, float *p10) {
  byte buffer;
  int value;
  int len = 0;
  int pm10_serial = 0;
  int pm25_serial = 0;
  int checksum_is;
  int checksum_ok = 0;
  int error = 1;
  while ((Serial2.available() > 0) && (Serial2.available() >= (10 - len))) {
    buffer = Serial2.read();
    value = int(buffer);
    switch (len) {
      case (0): if (value != 170) {
          len = -1;
        }; break;
      case (1): if (value == 192 || value == 207) {
          break;
        } else {
          len = -1;
        };
      case (2): pm25_serial = value; checksum_is = value; break;
      case (3): pm25_serial += (value << 8); checksum_is += value; break;
      case (4): pm10_serial = value; checksum_is += value; break;
      case (5): pm10_serial += (value << 8); checksum_is += value; break;
      case (6): checksum_is += value; break;
      case (7): checksum_is += value; break;
      case (8): if (value == (checksum_is % 256)) {
          checksum_ok = 1;
        } else {
          len = -1;
        }; break;
      case (9): if (value != 171) {
          len = -1;
        }; break;
    }
    len++;
    if (len == 10 && checksum_ok == 1) {
      *p10 = (float)pm10_serial / 10.0;
      *p25 = (float)pm25_serial / 10.0;
      len = 0; checksum_ok = 0; pm10_serial = 0.0; pm25_serial = 0.0; checksum_is = 0;
      error = 0;
    }
    yield();
  }
  return error;
}

String getSENOPPB() {
  // int adc = analogRead(A1);
  // float nilai = adc * (5.00 / 1024.00); //nilai tegangan
  // nilai = nilai * 100;
  // int ppb = map(nilai, 5, 303, 0, 10000);

  // String str_return;
  // str_return = String(ppb);
  //  int16_t adc0;
  //  DFRobot_ADS1115 ads(&Wire);
  //  ads.setAddr_ADS1115(0x48);   // 0x48
  //  ads.setGain(eGAIN_TWOTHIRDS);   // 2/3x gain
  //  ads.setMode(eMODE_SINGLE);       // single-shot mode
  //  ads.setRate(eRATE_128);          // 128SPS (default)
  //  ads.setOSMode(eOSMODE_SINGLE);   // Set to start a single-conversion
  //  ads.init();
  //  if (ads.checkADS1115())
  //  {
  //    delay(50);
  //    adc0 = ads.readVoltage(0);
  //    String str_return;
  //    str_return = String(adc0);
  //    return str_return;
  //  }

  int adc = analogRead(A0);
    STATUS[5] = true;
  float nilai = adc * (5.00 / 1024.00);
  nilai = (3.077 * nilai) - 0.154;
  nilai = nilai * 0.53;
  String str_return;
  str_return = String(nilai, 3);
  return str_return;
}

String getParticleCounter() {
  Serial1.begin(115200, SERIAL_8N1);
  node.begin(1, Serial1);
  uint8_t i, result1;
  uint16_t data[8];
  String str_return = "";
  float PM25, PM10;
  result1 = node.readHoldingRegisters(127, 2);
  if (result1 == node.ku8MBSuccess) {
    for (i = 0; i < 3; i++) {
      data[i] = node.getResponseBuffer(i);
    }
    STATUS[10] = true;
  }else{
    STATUS[10] = false;
  }
  PM25 = data[0];
  PM10 = data[1];

  str_return = String(PM25) + ";" + String(PM10) + ";";

  delay(10);
  return str_return;
}

String getMETONE(int addr) {
  float board0[2], board1[2];

  String str_return = "";
  Serial1.begin(9600, SERIAL_8N1);
  node.begin(addr, Serial1);

  uint8_t i, result1, result2;
  uint16_t data[2], data1[2];

  union {
    uint32_t j;
    float f;
  } u;

  result1 = node.readInputRegisters(100, 2);
  delay(100);
  if (result1 == node.ku8MBSuccess) {
    for (i = 0; i < 2; i++) {
      data[i] = node.getResponseBuffer(i);
    }
    u.j = ((unsigned long)data[1] << 16 | data[0]); board0[0] = u.f;

    str_return = String(board0[0]);
  } else {
    delay(200);
    result1 = node.readInputRegisters(100, 2);
    delay(100);
    if (result1 == node.ku8MBSuccess) {
      for (i = 0; i < 2; i++) {
        data[i] = node.getResponseBuffer(i);
      }
      u.j = ((unsigned long)data[1] << 16 | data[0]); board0[0] = u.f;

      str_return = String(board0[0]);
    } else {
      for (i = 0; i < 2; i++) {
        str_return = str_return + "0;";
      }
    }
  }
  delay(100);
  return str_return;
}

String getMETONEFLOW(int addr) {
  float board0[2], board1[2];

  String str_return = "";
  Serial1.begin(9600, SERIAL_8N1);
  node.begin(addr, Serial1);

  uint8_t i, result1, result2;
  uint16_t data[2], data1[2];

  union {
    uint32_t j;
    float f;
  } u;

  result1 = node.readInputRegisters(112, 2);
  delay(100);
  if (result1 == node.ku8MBSuccess) {
    for (i = 0; i < 2; i++) {
      data[i] = node.getResponseBuffer(i);
    }
    u.j = ((unsigned long)data[1] << 16 | data[0]); board0[0] = u.f;
    str_return = String(board0[0]);
  } else {
    delay(200);
    result1 = node.readInputRegisters(112, 2);
    delay(100);
    if (result1 == node.ku8MBSuccess) {
      for (i = 0; i < 2; i++) {
        data[i] = node.getResponseBuffer(i);
      }
      u.j = ((unsigned long)data[1] << 16 | data[0]); board0[0] = u.f;
      str_return = String(board0[0]);
    } else {
      for (i = 0; i < 2; i++) {
        str_return = str_return + "0;";
      }
    }
  }
  delay(100);
  return str_return;
}