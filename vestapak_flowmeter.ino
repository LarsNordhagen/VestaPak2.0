#include <ArduinoBLE.h>

BLEService vestService("19B10000-E8F2-537E-4F6C-D104768A1214"); // service

BLEStringCharacteristic volumeCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 20); // basic water tracking from flowmeter
BLEByteCharacteristic alertCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify); // alert user based on if theyre low on water or not, basic bool
BLEStringCharacteristic intervalReportCharacteristic("19B10003-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 20); // math based action on informing user of optimal water consumption

//pinout
const int sensorPin = 2;
volatile long pulseCount = 0;

const int SAMPLE_TIME = 500; //pulse sample time
const float MIN_SIP = 1.0; //noise gate

// low water alert
const float ALERT_THRESHOLD = 1700.0; //how much water in ml needs to be drank for user to be alerted. can adjust for what we want
bool alertSent = false; //alert bool

// 15 minute consumption alert
const unsigned long INTERVAL_TIME = 900000; // 15 minutes in milliseconds. malleable val for testing.
const float INTERVAL_TARGET_ML = 236.0; // 236 ml is on average, optimal water consumption in 15-20 mins. malleable val for testing.
float intervalVolume_mL = 0.0; //track volume for 15 min interval
unsigned long lastIntervalTime = 0; //15 min interval timer

//flowmeter tracking vars
float totalVolume_mL = 0.0;
long lastPulseCount = 0; 
unsigned long lastSampleTime = 0; 


void setup() {
//  Serial.begin(9600);
//  while (!Serial);

  //init
  pinMode(sensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(sensorPin), increasePulse, RISING); //necessary, detects rising input on square wave, ++
  lastSampleTime = millis();

//init ble in setup
  if (!BLE.begin()) {
//    Serial.println("Starting BLE failed!");
    while (1);
  }

  BLE.setAdvertisingInterval(800); //advertising until connect
  BLE.setLocalName("Vestapak"); //service name
  BLE.setAdvertisedService(vestService); 

  vestService.addCharacteristic(volumeCharacteristic); //for total flowmeter tracking
  vestService.addCharacteristic(alertCharacteristic); //for static low water alert
  vestService.addCharacteristic(intervalReportCharacteristic); //alert for 15 min interval consumptioon

  BLE.addService(vestService);

  volumeCharacteristic.writeValue("0.00 mL"); //set initial
  alertCharacteristic.writeValue(0); // low water alert
  String initialReport = "0.0/" + String((int)INTERVAL_TARGET_ML) + " mL"; //made adjustable for testing purposes. will report to user on water drinking
  intervalReportCharacteristic.writeValue(initialReport); //15 min alert

  BLE.advertise(); //advertise on ble
  
//  Serial.println("Vestapak is running and advertising via BLE");
}

void loop() {
  // allow the BLE stack to process events
  BLE.poll();

  //if sampling time has overflowed,
  if (millis() - lastSampleTime >= SAMPLE_TIME) {
    
    // calculate how many pulses happened just in this period. this will need to be generally tuned for each flowmeter. the ref gives code to help find pulses per ml
    //ref: https://www.youtube.com/watch?v=xXItEusqqEg 
    long currentPulses = pulseCount;
    long pulsesThisTick = currentPulses - lastPulseCount;
    // convert the pulses from this tick into vol
    float volumeThisTick = pulsesThisTick * (100.0 / 505.25); //this calculation was found on a basis of average pulses per 100ml. boils down to 5.05 pulses a ml. 100ml / 505.25 gives you ~.2 ml a pulse.

    // only if the volume in this tick is greater than our min vol (noise gate) does it actually count
    if (volumeThisTick > MIN_SIP) { 
      totalVolume_mL += volumeThisTick;
      intervalVolume_mL += volumeThisTick; //add sip to interval total
      
      //update ble to nfr conect with new variable. send as string. can make char arr for cheaper size transmission
      String volumeString = String(totalVolume_mL, 2) + " mL";
      volumeCharacteristic.writeValue(volumeString);

      // alert checker, send static status update via ble. needs resetting every fill cycle
      if (totalVolume_mL > ALERT_THRESHOLD && !alertSent) { //once alert is sent, disables this. needs reset after
        alertSent = true;
        alertCharacteristic.writeValue(1);
      }

      //If there is no water left, we want to stop our pin from receiving data
      if (totalVolume_mL > (ALERT_THRESHOLD + 300)) {
        detachInterrupt(digitalPinToInterrupt(sensorPin));
      }

    }

    //update pulses
    lastSampleTime = millis();
    lastPulseCount = currentPulses;
    
//    Serial.print("Total Volume: ");
//    Serial.print(totalVolume_mL, 2);
//    Serial.println(" mL");
  }

  //run parallel to sip detection. will only update post-15 min interval and not actively.
  if (millis() - lastIntervalTime >= INTERVAL_TIME) { // 15 min interval could be a while() loop instead
    //semd
    String intervalReportString = String(intervalVolume_mL, 1) + "/" + String((int)INTERVAL_TARGET_ML) + " mL";
    intervalReportCharacteristic.writeValue(intervalReportString);
      
    //reset
    intervalVolume_mL = 0.0;
    lastIntervalTime = millis();
  }
}

//ISR for pulse track
void increasePulse() {
  pulseCount++;
}

/*
// https://docs.arduino.cc/libraries/arduinoble/#BLECharacteristic%20Class
void noWaterAlert(BLEDevice , BLECharacteristic characteristic) {
  String warning = characteristic.value()

  //If there is no water left, we want to stop our pin from receiving data
  if (warning == "No Water")
  {
    detachInterrupt(digitalPinToInterrupt(sensorPin))
  }

}
*/