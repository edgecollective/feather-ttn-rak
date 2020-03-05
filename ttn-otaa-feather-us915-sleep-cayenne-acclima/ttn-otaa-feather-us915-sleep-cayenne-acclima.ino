/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * Copyright (c) 2018 Terry Moore, MCCI
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network. It's pre-configured for the Adafruit
 * Feather M0 LoRa.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!

 * To use this sketch, first register your application and device with
 * the things network, to set or generate an AppEUI, DevEUI and AppKey.
 * Multiple devices can use the same AppEUI, but each device has its own
 * DevEUI and AppKey.
 *
 * Do not forget to define the radio type correctly in
 * arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.
 *
 *******************************************************************************/

// NOTE: as of Feb 16 2020, release v2.3.2 of the mcci-catenda/arduino-lmic library works, but some later versions did not work. should test them.
// v2.3.2 release is here: https://github.com/mcci-catena/arduino-lmic/releases/tag/v2.3.2

//using cayenne: https://github.com/ElectronicCats/CayenneLPP



#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include "RTCZero.h" // https://github.com/arduino-libraries/RTCZero
#include <CayenneLPP.h> // https://github.com/ElectronicCats/CayenneLPP
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include <SDI12.h>


// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
//const 
unsigned TX_INTERVAL = 30; //default

unsigned SHORT_SLEEP_INTERVAL = 30;
//unsigned LONG_SLEEP_INTERVAL = 3600;
unsigned LONG_SLEEP_INTERVAL = 240;


#define RTC_SLEEP 1 // 1: use RTC to deep sleep; 0: use delay to wait

#define VBATPIN A0



#define JOINED A2
#define TRANSMITTED 9
#define SENSORS_READ 11
#define READING_SENSORS 12
#define AWAKE 13


// control the sleep interval

#define SLIDER_PIN A1

int sdi_status = 0; // status of sdi_sensor;  0=no sensor found; 1=sensor found

//---------- ACCLIMA

#define SERIAL_BAUD 115200  // The baud rate for the output serial port
#define DATA_PIN 10         // The pin of the SDI-12 data bus
#define POWER_PIN -1       // The sensor power pin (or -1 if not switching power)

// Define the SDI-12 bus
SDI12 mySDI12(DATA_PIN);

String sdiResponse = "";
String myCommand = "";

// variable that alternates output type back and forth between parsed and raw
boolean flip = 1;

// https://acclima.com/prodlit/TDR%20User%20Manual.pdf

#define num_params 6 // 5 from soil moisture, 6th is battery voltage

float params[num_params]; // acclima params

byte addressRegister[8] = {
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000
};

uint8_t numSensors = 0;

// converts allowable address characters '0'-'9', 'a'-'z', 'A'-'Z',
// to a decimal number between 0 and 61 (inclusive) to cover the 62 possible addresses
byte charToDec(char i){
  if((i >= '0') && (i <= '9')) return i - '0';
  if((i >= 'a') && (i <= 'z')) return i - 'a' + 10;
  if((i >= 'A') && (i <= 'Z')) return i - 'A' + 37;
  else return i;
}

// THIS METHOD IS UNUSED IN THIS EXAMPLE, BUT IT MAY BE HELPFUL.
// maps a decimal number between 0 and 61 (inclusive) to
// allowable address characters '0'-'9', 'a'-'z', 'A'-'Z',
char decToChar(byte i){
  if(i <= 9) return i + '0';
  if((i >= 10) && (i <= 36)) return i + 'a' - 10;
  if((i >= 37) && (i <= 62)) return i + 'A' - 37;
  else return i;
}

// gets identification information from a sensor, and prints it to the serial port
// expects a character between '0'-'9', 'a'-'z', or 'A'-'Z'.
void printInfo(char i){
  String command = "";
  command += (char) i;
  command += "I!";
  mySDI12.sendCommand(command);
  // Serial.print(">>>");
  // Serial.println(command);
  delay(30);

  printBufferToScreen();
}
void printBufferToScreen(){
  Serial.println("printBuffer");
  String buffer = "";
  mySDI12.read(); // consume address
  int item = 0;
  while(mySDI12.available()){
    
  float that = mySDI12.parseFloat();
  Serial.print(item);
  Serial.print(":");
  Serial.println(that);
  params[item]=that;
  item++;
  delay(50);
    /*
    char c = mySDI12.read();
    if(c == '+'){
      buffer += ',';
    }
    else if ((c != '\n') && (c != '\r')) {
      buffer += c;
    }
    delay(50);
*/
    
  }
 Serial.print(buffer);
}

void resetParams() {
      int param = 0;
      while (param<num_params){
          params[param]=-99.;
          param++;
        } 
}

/*
void getParams(){

  Serial.println("get Params ..");

    mySDI12.read();  // discard address
    int param = 0;
    while(mySDI12.available()){
        float that = mySDI12.parseFloat();
        Serial.println(that);
        if(that != mySDI12.TIMEOUT){    //check for timeout
          //float doubleThat = that * 2;
          params[param]=that;
          
          param++;

        } 
    }
    
    Serial.println();

}
*/

void takeMeasurement(char i){
  String command = "";
  command += i;
  command += "M!"; // SDI-12 measurement command format  [address]['M'][!]
  mySDI12.sendCommand(command);
  delay(30);

  // wait for acknowlegement with format [address][ttt (3 char, seconds)][number of measurments available, 0-9]
  String sdiResponse = "";
  delay(30);
  while (mySDI12.available())  // build response string
  {
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r'))
    {
      sdiResponse += c;
      delay(5);
    }
  }
  mySDI12.clearBuffer();

  // find out how long we have to wait (in seconds).
  uint8_t wait = 0;
  wait = sdiResponse.substring(1,4).toInt();

  // Set up the number of results to expect
  // int numMeasurements =  sdiResponse.substring(4,5).toInt();

  unsigned long timerStart = millis();
  while((millis() - timerStart) < (1000 * wait)){
    if(mySDI12.available())  // sensor can interrupt us to let us know it is done early
    {
      mySDI12.clearBuffer();
      break;
    }
  }
  // Wait for anything else and clear it out
  delay(30);
  mySDI12.clearBuffer();

  // in this example we will only take the 'DO' measurement
  command = "";
  command += i;
  command += "D0!"; // SDI-12 command to get data [address][D][dataOption][!]
  mySDI12.sendCommand(command);
  while(!(mySDI12.available()>1)){}  // wait for acknowlegement
  delay(300); // let the data transfer
  printBufferToScreen();
  mySDI12.clearBuffer();
}

// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
boolean checkActive(char i){

  String myCommand = "";
  myCommand = "";
  myCommand += (char) i;                 // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for(int j = 0; j < 3; j++){            // goes through three rapid contact attempts
    mySDI12.sendCommand(myCommand);
    delay(30);
    if(mySDI12.available()) {  // If we here anything, assume we have an active sensor
      printBufferToScreen();
      mySDI12.clearBuffer();
      return true;
    }
  }
  mySDI12.clearBuffer();
  return false;
}

// this quickly checks if the address has already been taken by an active sensor
boolean isTaken(byte i){
  i = charToDec(i); // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  byte j = i / 8;   // byte #
  byte k = i % 8;   // bit #
  return addressRegister[j] & (1<<k); // return bit status
}

// this sets the bit in the proper location within the addressRegister
// to record that the sensor is active and the address is taken.
boolean setTaken(byte i){
  boolean initStatus = isTaken(i);
  i = charToDec(i); // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  byte j = i / 8;   // byte #
  byte k = i % 8;   // bit #
  addressRegister[j] |= (1 << k);
  return !initStatus; // return false if already taken
}

// THIS METHOD IS UNUSED IN THIS EXAMPLE, BUT IT MAY BE HELPFUL.
// this unsets the bit in the proper location within the addressRegister
// to record that the sensor is active and the address is taken.
boolean setVacant(byte i){
  boolean initStatus = isTaken(i);
  i = charToDec(i); // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  byte j = i / 8;   // byte #
  byte k = i % 8;   // bit #
  addressRegister[j] &= ~(1 << k);
  return initStatus; // return false if already vacant
}



//------------------- SLEEP


RTCZero rtc;
CayenneLPP lpp(51);

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8]= {0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22};
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]= {0xd6,0xe5,0x86,0x69,0xad,0x1b,0x72,0x96};
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from the TTN console can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = { 0x58,0x0c,0x7a,0xc9,0x85,0x95,0x99,0xf2,0xca,0xcb,0x79,0x15,0x0a,0x45,0x6f,0x39 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;



const lmic_pinmap lmic_pins = {
    .nss = 8,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {3, 6, LMIC_UNUSED_PIN},
    .rxtx_rx_active = 0,
    .rssi_cal = 8,              // LBT cal for the Adafruit Feather M0 LoRa, in dB
    .spi_freq = 8000000,
};

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
        
            pinMode(JOINED,OUTPUT);
            digitalWrite(JOINED,HIGH);
            
            Serial.println(F("EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              Serial.println(netid, DEC);
              Serial.print("devaddr: ");
              Serial.println(devaddr, HEX);
              Serial.print("artKey: ");
              for (int i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                Serial.print(artKey[i], HEX);
              }
              Serial.println("");
              Serial.print("nwkKey: ");
              for (int i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              Serial.print("-");
                      Serial.print(nwkKey[i], HEX);
              }
              Serial.println("");
            }
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
      // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
             pinMode(JOINED,INPUT_PULLDOWN);

            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            pinMode(JOINED,INPUT_PULLDOWN);

            break;
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission

            

/*
            // get the status of the slider pin to decide sleep interval
            int sliderState = digitalRead(pushButton);
            
            if (sliderState==1) {
              TX_INTERVAL = SHORT_SLEEP_INTERVAL;
            }
            else {
              TX_INTERVAL = LONG_SLEEP_INTERVAL;
            }
 */
          
            Serial.print("TX_INTERVAL:");
            Serial.println(TX_INTERVAL);


            pinMode(TRANSMITTED,OUTPUT);
            digitalWrite(TRANSMITTED,HIGH);

            pinMode(AWAKE,INPUT_PULLDOWN);
            
            if(RTC_SLEEP) {

            // Sleep for a period of TX_INTERVAL using single shot alarm
            rtc.setAlarmEpoch(rtc.getEpoch() + TX_INTERVAL);
            rtc.enableAlarm(rtc.MATCH_YYMMDDHHMMSS);
            rtc.attachInterrupt(alarmMatch);
            
            // USB port consumes extra current
            USBDevice.detach();

            
            // Enter sleep mode
            rtc.standbyMode();

            
            // Reinitialize USB for debugging
            USBDevice.init();
            USBDevice.attach();

            //now we're awake again
            
            //pulldown_pins();
            pinMode(AWAKE,OUTPUT);
            digitalWrite(AWAKE,HIGH);
            pinMode(TRANSMITTED,INPUT_PULLDOWN);
            pinMode(SENSORS_READ,INPUT_PULLDOWN);


            }
            else{
        
            
            delay(TX_INTERVAL*1000); // if not entering standby mode, do this

            //pulldown_pins();
            pinMode(AWAKE,OUTPUT);
            digitalWrite(AWAKE,HIGH);
            pinMode(TRANSMITTED,INPUT_PULLDOWN);
            pinMode(SENSORS_READ,INPUT_PULLDOWN);
            
            }

            os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(1), do_send);

            //os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            //os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
             pinMode(JOINED,INPUT_PULLDOWN);
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void do_send(osjob_t* j){

  
    pinMode(SENSORS_READ,INPUT_PULLDOWN);
    pinMode(TRANSMITTED,INPUT_PULLDOWN);

  
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {

 pinMode(READING_SENSORS,OUTPUT);
  digitalWrite(READING_SENSORS,HIGH);
  
// read the battery voltage
    float measuredvbat = analogRead(VBATPIN);
measuredvbat *= 2;    // we divided by 2, so multiply back
measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
measuredvbat /= 1024; // convert to voltage
Serial.print("VBat: " ); Serial.println(measuredvbat);

     
// SDI-12 stuff
Serial.println("Opening SDI-12 bus...");
  mySDI12.begin();
  delay(500); // allow things to settle

  // Power the sensors;
  if(POWER_PIN > 0){
    Serial.println("Powering up sensors...");
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);
    delay(200);
  }

  Serial.println("Scanning all addresses, please wait...");
  /*
      Quickly Scan the Address Space
   */

  for(byte i = '0'; i <= '9'; i++) if(checkActive(i)) {numSensors++; setTaken(i);}   // scan address space 0-9

  for(byte i = 'a'; i <= 'z'; i++) if(checkActive(i)) {numSensors++; setTaken(i);}   // scan address space a-z

  for(byte i = 'A'; i <= 'Z'; i++) if(checkActive(i)) {numSensors++; setTaken(i);}   // scan address space A-Z

  /*
      See if there are any active sensors.
   */
  boolean found = false;

  for(byte i = 0; i < 62; i++){
    if(isTaken(i)){
      found = true;
      Serial.print("First address found:  ");
      Serial.println(decToChar(i));
      Serial.print("Total number of sensors found:  ");
      Serial.println(numSensors);
      break;
    }
  }


//pinMode(SENSOR_FOUND_PIN,OUTPUT);

  if(!found) {
    Serial.println("No sensors found, please check connections and restart the Arduino.");
    //digitalWrite(SENSOR_FOUND_PIN,LOW);
  } // stop here
   //digitalWrite(SENSOR_FOUND_PIN,HIGH);


  
  // scan address space 0-9
  for(char i = '0'; i <= '9'; i++) if(isTaken(i)){
    Serial.print(millis()/1000);
    Serial.print(",\t");
    printInfo(i);
    Serial.print(",\t");
    takeMeasurement(i);
    Serial.println();
  }

  // scan address space a-z
  for(char i = 'a'; i <= 'z'; i++) if(isTaken(i)){
    Serial.print(millis()/1000);
    Serial.print(",\t");
    printInfo(i);
    Serial.print(",\t");
    takeMeasurement(i);
    Serial.println();
  }

  // scan address space A-Z
  for(char i = 'A'; i <= 'Z'; i++) if(isTaken(i)){
    Serial.print(millis()/1000);
    Serial.print(",\t");
    printInfo(i);
    Serial.print(",\t");
    takeMeasurement(i);
    Serial.println();
  };
  
  //delay(3000);
  
  for (int i =0;i<num_params;i++) {
    Serial.print("params ");
    Serial.print(i);
    Serial.print(" =");
    Serial.println(params[i]);
  }


 pinMode(READING_SENSORS,INPUT_PULLDOWN);
 
  pinMode(SENSORS_READ,OUTPUT);
  digitalWrite(SENSORS_READ,HIGH);
  
  lpp.reset();

  lpp.addTemperature(1, params[0]); // vwc
  lpp.addTemperature(2, params[1]); // temp
  lpp.addTemperature(3, params[2]); // permittivity
  lpp.addTemperature(4, params[3]); // bulk EC
  lpp.addTemperature(5, params[4]/10.); // pore EC

  lpp.addAnalogInput(6, measuredvbat); // voltage
  lpp.addAnalogInput(7,sdi_status); // sensor status:  0= no sensor found; 1=sensor found
  
        // Prepare upstream data transmission at the next possible time.
        //LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
   LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);
        
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.

/*
if (RTC_SLEEP) {
    pinMode(0,INPUT_PULLUP);
    pinMode(1,INPUT_PULLUP);
    pinMode(A0,INPUT_PULLUP);
    pinMode(A1,INPUT_PULLUP);
    pinMode(A2,INPUT_PULLUP);
    pinMode(A3,INPUT_PULLUP);
    pinMode(A4,INPUT_PULLUP);
    pinMode(A5,INPUT_PULLUP);
    pinMode(0,INPUT_PULLUP);
    pinMode(1,INPUT_PULLUP);
    pinMode(5,INPUT_PULLUP);
    pinMode(9,INPUT_PULLUP);
    pinMode(11,INPUT_PULLUP);
    pinMode(12,INPUT_PULLUP);
    pinMode(13,INPUT_PULLUP);
}
*/
   // pinMode(10,INPUT);

}

void pulldown_pins() {

//pinMode(SLEEPING,INPUT_PULLDOWN);
pinMode(TRANSMITTED,INPUT_PULLDOWN);
pinMode(JOINED,INPUT_PULLDOWN);
pinMode(SENSORS_READ,INPUT_PULLDOWN);
pinMode(READING_SENSORS,INPUT_PULLDOWN);
pinMode(AWAKE,INPUT_PULLDOWN);

}

void setup() {

pulldown_pins();

pinMode(AWAKE,OUTPUT);
digitalWrite(AWAKE,HIGH);
            
// set the sleep interval TX_INTERVAL based on the slide switch
// left = high = short, right = low = long

  pinMode(SLIDER_PIN, INPUT); // set up the slide switch pin 


int SWITCH_STATUS=digitalRead(SLIDER_PIN);
  if (SWITCH_STATUS) {
    TX_INTERVAL=SHORT_SLEEP_INTERVAL;
  }
  else {
    TX_INTERVAL=LONG_SLEEP_INTERVAL;
  }
  

  
            
    Serial.begin(9600);
    Serial.println(F("Starting"));

  
   
if(RTC_SLEEP) {
      // Initialize RTC
    rtc.begin();
    // Use RTC as a second timer instead of calendar
    rtc.setEpoch(0);
}
  
    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

    LMIC_setLinkCheckMode(0);
    LMIC_setDrTxpow(DR_SF7,14);
    LMIC_selectSubBand(1);

    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}

void alarmMatch()
{

}
