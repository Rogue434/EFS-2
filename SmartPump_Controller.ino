//define library
#include <EEPROM.h>
#include <SimpleModbusSlave.h>

//define pinMode
int Relay = 5;
int PWM = 6;

//define variabel
#define JumlahRegister 10
unsigned int reg[JumlahRegister];
unsigned long waittime, Timer;
bool StartReset;


/*
  --- REGISTER MODBUS ADDRESS ---
  Modbus Register
  register 0 = Request StatusMode
  register 1 = Request Kecepatan PWM
  register 2 = Request Kondisi Relay
  register 3 = Request Timer Relay
  register 4 = Request Timer Relay Meantime
  register 5 = Swap Relay

  --- EEPROOM ADDRESS LIST --- 
  EEPROM 0 = status mode
  EEPROM 10 = kecepatan PWM
  EEPROM 20 = Waktu CD relay
*/


void writeStringToEEPROM(int addrOffset, const String& strToWrite) {
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);  // Menyimpan panjang string
  for (int i = 0; i < len; i++) {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
}

String readStringFromEEPROM(int addrOffset) {
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++) {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0';
  return String(data);
}

void setup() {
  Serial.begin(9600);
  modbus_configure(9600, 4, 0, JumlahRegister, 0);
  pinMode(Relay, OUTPUT);
  pinMode(PWM, OUTPUT);
  Serial.println("*---- Starting Smart Pump ----*");
  if (readStringFromEEPROM(0).toInt() < 0 || readStringFromEEPROM(0).toInt() > 1) {
    writeStringToEEPROM(0, "0");
    Serial.println("Update Status Mode to : 0");
  }
  if (readStringFromEEPROM(10).toInt() < 0 || readStringFromEEPROM(10).toInt() > 100) {
    writeStringToEEPROM(10, "100");
    Serial.println("Update PWM Speed to : 50%");
  }
  if (readStringFromEEPROM(20).toInt() < 1 || readStringFromEEPROM(20).toInt() > 21600) {
    writeStringToEEPROM(20, "3600");
    Serial.println("Update Switch Timer to : 300 Second");
  }
  reg[0] = readStringFromEEPROM(0).toInt();
  reg[1] = readStringFromEEPROM(10).toInt();
  reg[3] = readStringFromEEPROM(20).toInt();
  int N_PWM = map(reg[1], 0, 100, 0, 255);
  analogWrite(PWM, N_PWM);
}

void (*resetFunc)(void) = 0;

void loop() {
  modbus_update(reg);

  if (reg[0] != readStringFromEEPROM(0).toInt()) {
    if (reg[0] < 0 || reg[0] > 1) {
      writeStringToEEPROM(0, "0");
    } else {
      writeStringToEEPROM(0, String(reg[0]));
    }
    Serial.println(String("Mode Set To\t: ") + reg[0]);
    reg[0] = readStringFromEEPROM(0).toInt();
  }

  if (reg[1] != readStringFromEEPROM(10).toInt()) {
    if (reg[1] < 0 || reg[1] > 100) {
      writeStringToEEPROM(10, "1000");
    } else {
      writeStringToEEPROM(10, String(reg[1]));
    }
    Serial.println(String("PWM Set To\t: ") + reg[1]);
    reg[1] = readStringFromEEPROM(10).toInt();
    int N_PWM = map(reg[1], 0, 100, 0, 255);
    analogWrite(PWM, N_PWM);
  }

  if (reg[3] != readStringFromEEPROM(20).toInt()) {
    if (reg[3] < 1 || reg[3] > 21600) {
      writeStringToEEPROM(20, "3600");
    } else {
      writeStringToEEPROM(20, String(reg[3]));
    }
    Serial.println(String("Timer Set To\t: ") + reg[3]);
    reg[3] = readStringFromEEPROM(20).toInt();
  }

  int statusRelay = digitalRead(Relay);

  if (reg[5] != 0) {
    Serial.println(String("Swap Relay!!"));
    if (statusRelay == 0) {
      digitalWrite(Relay, HIGH);
    } else {
      digitalWrite(Relay, LOW);
    }
    if (readStringFromEEPROM(0).toInt() == 0) {
      reg[4] = 0;
      Timer = 0;
    }
    reg[5] = 0;
  }

  if ((millis() - waittime) > 1000) {
    if (readStringFromEEPROM(0).toInt() == 0) {
      if (Timer > readStringFromEEPROM(20).toInt()) {
        Serial.println("Switch Relay!!");
        if (statusRelay == 0) {
          digitalWrite(Relay, HIGH);
        } else {
          digitalWrite(Relay, LOW);
        }
        Timer = 0;
      }
      Timer++;
    }

    reg[2] = statusRelay;
    reg[3] = readStringFromEEPROM(20).toInt();
    reg[4] = Timer;

    Serial.println(String("StatusMode\t: ") + String(reg[0]));
    Serial.println(String("Kecepatan PWM\t: ") + String(reg[1]));
    Serial.println(String("Relay Condition\t: ") + String(reg[2]));
    Serial.println(String("Relay S_Timer\t: ") + String(reg[3]));
    Serial.println(String("Relay Timer\t: ") + String(reg[4]));
    Serial.println("\n\n");
    waittime = millis();
  }
}