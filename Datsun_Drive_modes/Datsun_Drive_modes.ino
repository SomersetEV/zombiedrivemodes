
#include <SPI.h>


#define CAN_2515
// #define CAN_2518FD

// Set SPI CS Pin according to your hardware

#if defined(SEEED_WIO_TERMINAL) && defined(CAN_2518FD)
// For Wio Terminal w/ MCP2518FD RPi Hat：
// Channel 0 SPI_CS Pin: BCM 8
// Channel 1 SPI_CS Pin: BCM 7
// Interupt Pin: BCM25
const int SPI_CS_PIN  = BCM8;
const int CAN_INT_PIN = BCM25;
#else

// For Arduino MCP2515 Hat:
// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;
const int CAN_INT_PIN = 2;
#endif


#ifdef CAN_2515
#include "mcp2515_can.h"
mcp2515_can CAN(SPI_CS_PIN); // Set CS pin
#endif

unsigned char flagRecv = 0;
unsigned char len = 0;
unsigned long id = 0;
unsigned char buf[8];
char str[20];

//States
int DriveMode = 4;

//Inputs
int ThrotRamp;
int ThrotMax;
int Gear;
int ButtonSport = 3;
int ButtonEco = 2;
int ButtonDrift = 1;
int motorrpm = 0;

//Outputs
int LightSport = 4;
int LightEco = 5;
int LightDrift = 6;
int ChangeThrotRamp;
int ChangeThrotMax;
int GearChange;


//set buttons to pins

void setup() {
  Serial.begin(115200);
  while (!Serial) {};
  //attachInterrupt(digitalPinToInterrupt(CAN_INT_PIN), MCP2515_ISR, FALLING); // start interrupt
  while (CAN_OK != CAN.begin(CAN_500KBPS)) {             // init can bus : baudrate = 500k
    Serial.println("CAN init fail, retry...");
    delay(100);
  }
  Serial.println("CAN init ok!");




  /*
      set mask, set both the mask to 0x3ff
  */
  CAN.init_Mask(0, 0, 0x3ff);                         // there are 2 mask in mcp2515, you need to set both of them
  CAN.init_Mask(1, 0, 0x3ff);


  /*
      set filter, we can mask up to 6 IDs
  */
  CAN.init_Filt(0, 0, 0x394);                          // RPM receive
  CAN.init_Filt(1, 0, 0x395);                          // Trotramp, ThrotMax and RPM

  CAN.init_Filt(2, 0, 0x467);                          // there are 6 filter in mcp2515
  CAN.init_Filt(3, 0, 0x07);                          // there are 6 filter in mcp2515
  CAN.init_Filt(4, 0, 0x08);                          // there are 6 filter in mcp2515
  CAN.init_Filt(5, 0, 0x09);                          // there are 6 filter in mcp2515


  // pin setup
  pinMode(LightSport, OUTPUT);
  pinMode(LightEco, OUTPUT);
  pinMode(LightDrift, OUTPUT);

  pinMode(ButtonSport, INPUT_PULLUP);
  pinMode(ButtonEco, INPUT_PULLUP);
  pinMode(ButtonDrift, INPUT_PULLUP);
}
void MCP2515_ISR() {
  flagRecv = 1;
}

void canbusread() {
  if (flagRecv) {                // check if get data

    flagRecv = 0;                // clear flag
    unsigned int id = CAN.getCanId();
    // Serial.print(id);
    if (id == 0x394) {

      motorrpm = (buf[0] + buf[1]); //read motorrpm
      Gear = (buf[7]);
      ThrotRamp = (buf[2]);
      ThrotMax = (buf[4]);

    }

  }
}

void setstates() {
  if (ThrotRamp == 100 and Gear == 1 and ThrotMax == 100) { //SportMode
    DriveMode = 1;
    Serial.println("Sportmode");
    //Also light LED sport
  }
  if (ThrotRamp == 1 and Gear == 1 and ThrotMax == 10) { //EcoMode
    DriveMode = 2;
    Serial.println("Eco Mode");
    //Also light LED Eco
  }
  if (ThrotRamp == 100 and Gear == 0 and ThrotMax == 100) { //Drift
    DriveMode = 3;
    Serial.println("Drift Mode");
    //Also light LED Drift
  }


}

void ButtonPress() {

  // messages to set
  unsigned char Throtrampmap[8] = {0x40, 0x01 ,0x20, 13, (ChangeThrotRamp >>0), (ChangeThrotRamp >>8), (ChangeThrotRamp >>16), (ChangeThrotRamp >>24)};
  unsigned char Throtmaxmap[8] = {0x40, 0x01 ,0x20, 25, (ThrotMax >>0), (ThrotMax >>8), (ThrotMax >>16), (ThrotMax >>24)};
  //ButtonSportState = digitalRead(ButtonSport);
  if (digitalRead(ButtonSport) == LOW) {
    if (DriveMode == 1) { // do nothing becuase its also in this mode
      Serial.println("Already in sport mode!");
    }
    else if (DriveMode != 1 and motorrpm == 0) {
      ChangeThrotRamp = 100 * 32;
      ThrotMax = 100 * 32;
      GearChange = 1;
      CAN.MCP_CAN::sendMsgBuf(0x601, 0, 8, Throtrampmap);
      CAN.MCP_CAN::sendMsgBuf(0x601, 0, 8, Throtmaxmap);
      Serial.println("Changing to sport mode");
      digitalWrite(LightSport, HIGH);
      digitalWrite(LightEco, LOW);
      digitalWrite(LightDrift, LOW);
    }


  }
  delay(50);
  if (digitalRead(ButtonEco) == LOW) { //Eco
    if (DriveMode == 2) { // do nothing becuase its also in this mode
      Serial.println("Already in Eco mode");
    }
    else if (DriveMode != 2 and motorrpm == 0) {
      ChangeThrotRamp = 1 * 32;
      ThrotMax = 10 * 32;
      GearChange = 1;
      CAN.MCP_CAN::sendMsgBuf(0x394, 0, 8, Throtrampmap);
      CAN.MCP_CAN::sendMsgBuf(0x394, 0, 8, Throtmaxmap);
      Serial.println("Changing to Eco mode");
      digitalWrite(LightEco, HIGH);
      digitalWrite(LightSport, LOW);
      digitalWrite(LightDrift, LOW);
    }

  }
  delay(50);
  /* if (digitalRead(ButtonDrift) == LOW) { //Drift
     if (DriveMode == 3) { // do nothing becuase its also in this mode
       Serial.println("Already in Drift mode");
     }
     else if (DriveMode != 3 and motorrpm == 0) {
       ChangeThrotRamp = 100;
       GearChange = 0;
       CAN.MCP_CAN::sendMsgBuf(0x394, 0, 8, Changemap);
       Serial.println("Changing to Drift mode");
     }
    }
  */
}



void LightLED() {
  switch (DriveMode) {
    case 1:
      digitalWrite(LightSport, HIGH);
      digitalWrite(LightEco, LOW);
      digitalWrite(LightDrift, LOW);
      break;

    case 2:
      digitalWrite(LightEco, HIGH);
      digitalWrite(LightSport, LOW);
      digitalWrite(LightDrift, LOW);
      break;

    case 3:
      digitalWrite(LightDrift, HIGH);
      digitalWrite(LightEco, LOW);
      digitalWrite(LightSport, LOW);
      break;


  }


}


void loop() {
  canbusread();
// setstates();
//  LightLED();
  ButtonPress();
}
