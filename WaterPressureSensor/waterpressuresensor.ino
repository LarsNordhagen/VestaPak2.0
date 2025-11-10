const int pressurePin = A0;

void setup() {
  Serial.begin(115200);
  Serial.println("Oversampled High-Resolution Monitor");
  Serial.println("===================================");
}

void loop() {
  // Take 256 samples and sum them for higher resolution
  long oversampledSum = 0;
  for (int i = 0; i < 256; i++) {
    oversampledSum += analogRead(pressurePin);
  }
  
  float highResValue = oversampledSum / 256.0;  
  float voltage = (highResValue / 1023.0) * 5.0;
  
  Serial.print("High-Res: ");
  Serial.print(highResValue, 3);  // 3 decimal places
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 6);
  Serial.println("V");
  
  delay(500);
}