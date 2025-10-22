//remove serial comments for debugging. saves power while not sending out 

#include <ArduinoBLE.h>

BLEService vestService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEStringCharacteristic volumeCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead, 20);
BLEStringCharacteristic noWaterCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLEWrite, 20);

const int sensorPin = 2;
volatile long pulseCount = 0;

const int SAMPLE_TIME = 500;
const float MIN_SIP = 1.0; 


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
  BLE.setAdvertisingInterval(800);
  BLE.setLocalName("Vestapak");
  BLE.setAdvertisedService(vestService);
  vestService.addCharacteristic(volumeCharacteristic);
  vestService.addCharacteristic(noWaterCharacteristic);
  BLE.addService(vestService);
  BLE.setEventHandler(BLEWritten, noWaterCharWritten)
  volumeCharacteristic.writeValue("0.00 mL"); //set initial
  BLE.advertise();
  
//  Serial.println("Vestapak is running and advertising via BLE");
}

void loop() {
  // allow the BLE stack to process events
  BLE.poll();

  //if sampling time has overflowed,
  if (millis() - lastSampleTime >= SAMPLE_TIME) {
    
    // calculate how many pulses happened just in this period
    long currentPulses = pulseCount;
    long pulsesThisTick = currentPulses - lastPulseCount;
    // convert the pulses from this tick into vol
    float volumeThisTick = pulsesThisTick * (100.0 / 505.25); //this calculation was found on a basis of average pulses per 100ml. boils down to 5.05 pulses a ml. 100ml / 505.25 gives you ~.2 ml a pulse.

    // only if the volume in this tick is greater than our min vol (noise) does it actually count
    if (volumeThisTick > MIN_SIP) {
      totalVolume_mL += volumeThisTick;
      
      //update ble to nfr conect with new variable. send as string. can make char arr for cheaper transmission
      String volumeString = String(totalVolume_mL, 2) + " mL";
      volumeCharacteristic.writeValue(volumeString);
    }

    //update pulses
    lastSampleTime = millis();
    lastPulseCount = currentPulses;
    
//    Serial.print("Total Volume: ");
//    Serial.print(totalVolume_mL, 2);
//    Serial.println(" mL");
  }
}

//ISR for pulse track
void increasePulse() {
  pulseCount++;
}

// https://docs.arduino.cc/libraries/arduinoble/#BLECharacteristic%20Class
void noWaterAlert(BLEDevice , BLECharacteristic characteristic) {
  String warning = characteristic.value()

  //If there is no water left, we want to stop our pin from receiving data
  if (warning == "No Water")
  {
    detachInterrupt(digitalPinToInterrupt(sensorPin))
  }

}

