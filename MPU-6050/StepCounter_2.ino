#include "Wire.h" // This library allows you to communicate with I2C devices.

const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
long a22;
double aAll, aFinal;
int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
int16_t temperature; // variables for temperature data

char tmp_str[7]; // temporary variable used in convert function

char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}

float stepRate = 5;
float samplingPeriod = 0.002;
float samplingRate;
int16_t np;

void setup() {
  Wire.setClock(400000);
  Serial.begin(9600);
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  samplingRate = 1/samplingPeriod;
  np = round(samplingRate/stepRate);
}

int16_t baseX = 15300;

int16_t minStepInterval = 350;
unsigned long lastStepTime = 0;

int16_t stepCount = 0;

int16_t stepOver = 1;
int16_t stepped = 0;

void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
  Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
  Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers
  
  // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
  accelerometer_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  accelerometer_x *= 1;
  accelerometer_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  accelerometer_y *= 0.94;
  accelerometer_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  accelerometer_z *= 1;
  temperature = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
  gyro_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
  gyro_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
  gyro_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)

  //a22 = (accelerometer_x*accelerometer_x) + (accelerometer_y*accelerometer_y) + (accelerometer_z*accelerometer_z);
  a22 = pow(accelerometer_x, 2) + pow(accelerometer_y, 2) + pow(accelerometer_z, 2);
  aAll = sqrt(a22);
  aFinal = aAll - 16000;


  if (aFinal < 1000) {
    stepOver = 1;
  }

  unsigned long timeSinceLastStep = millis() - lastStepTime;
  if (aFinal > 2000 && timeSinceLastStep > minStepInterval) {
    stepped = 2000;
    stepCount += 1;
    lastStepTime = millis();
  }

  // Output data to graph
  Serial.print("                                                                   ");
  if (false) {
    Serial.print(convert_int16_to_str(accelerometer_x));
    Serial.print(" ");
    Serial.print(convert_int16_to_str(accelerometer_y));
    Serial.print(" ");
    Serial.print(convert_int16_to_str(accelerometer_z));
    Serial.print(" ");
  }
  Serial.print(convert_int16_to_str(aFinal));
  Serial.print(" ");
  Serial.print(convert_int16_to_str(stepped));
  Serial.print(" ");
  Serial.println();

  Serial.print("Steps: ");
  Serial.print(stepCount);
  Serial.println();

  stepped = 0;

  // Delay
  delay(5);
}
