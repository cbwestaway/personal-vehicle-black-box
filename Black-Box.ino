/*
===Contact & Support===
Website: http://eeenthusiast.com/
Youtube: https://www.youtube.com/EEEnthusiast
Facebook: https://www.facebook.com/EEEnthusiast/
Patreon: https://www.patreon.com/EE_Enthusiast
Revision: 1.0 (July 13th, 2016)
===Hardware===
- Arduino Uno R3
- MPU-6050 (Available from: http://eeenthusiast.com/product/6dof-mpu-6050-accelerometer-gyroscope-temperature/)
===Software===
- Latest Software: https://github.com/VRomanov89/EEEnthusiast/tree/master/MPU-6050%20Implementation/MPU6050_Implementation
- Arduino IDE v1.6.9
- Arduino Wire library
===Terms of use===
The software is provided by EEEnthusiast without warranty of any kind. In no event shall the authors or 
copyright holders be liable for any claim, damages or other liability, whether in an action of contract, 
tort or otherwise, arising from, out of or in connection with the software or the use or other dealings in 
the software.
*/

#include <Wire.h>

long accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ;

long gyroX, gyroY, gyroZ;
float rotX, rotY, rotZ;
int delayTime = 2;
float velocityX, velocityY, velocityZ;
float displacementX, displacementY, displacementZ;
int counter = 0;

// arrays for briefly storing data
float acceleration[3][10];
float velocity[3][10];
float displacement[3][10];

void setup() {
  Serial.begin(9600);
  Wire.begin();
  setupMPU();
}


void loop() {
  if ( counter == 9)
  {
    counter = 0;
    //EEPROM.clear();
    //EEPROM.write();
    for(int i = 0; i < 9; i++)
    {
//      Serial.println(acceleration[1][i]);
//      Serial.println(acceleration[2][i]);
//      Serial.println(acceleration[3][i]);
    }
  }
  recordAccelRegisters();
  recordGyroRegisters();
  getVelocity();
  getDisplacement();
  storeData(gForceX, gForceY, gForceZ, velocityX, velocityY, velocityZ, displacementX, displacementY, displacementZ);
  printData();
  delay(2000);
  counter++;
}

void setupMPU(){
  Wire.beginTransmission(0b1101000); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire.write(0x6B); //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire.write(0b00000000); //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire.endTransmission();  
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1B); //Accessing the register 1B - Gyroscope Configuration (Sec. 4.4) 
  Wire.write(0x00000000); //Setting the gyro to full scale +/- 250deg./s 
  Wire.endTransmission(); 
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1C); //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5) 
  Wire.write(0b00000000); //Setting the accel to +/- 2g
  Wire.endTransmission(); 
}

void recordAccelRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x3B); //Starting register for Accel Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Accel Registers (3B - 40)
  while(Wire.available() < 6);
  accelX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  accelY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  accelZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  processAccelData();
}

void processAccelData(){
  gForceX = accelX / 16384.0;
  gForceY = accelY / 16384.0; 
  gForceZ = accelZ / 16384.0;
}

void recordGyroRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x43); //Starting register for Gyro Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Gyro Registers (43 - 48)
  while(Wire.available() < 6);
  gyroX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  gyroY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  gyroZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  processGyroData();
}

void processGyroData() {
  rotX = gyroX / 131.0;
  rotY = gyroY / 131.0; 
  rotZ = gyroZ / 131.0;
}

void storeData( float gForceX, float gForceY, float gForceZ, float velocityX, float velocityY, float velocityZ, float displacementX, float displacementY, float displacementZ)
{
  acceleration[1][counter] = gForceX;
  acceleration[2][counter] = gForceY;
  acceleration[3][counter] = gForceZ;

  velocity[1][counter] = velocityX;
  velocity[2][counter] = velocityY;
  velocity[3][counter] = velocityZ;

  displacement[1][counter] = displacementX;
  displacement[2][counter] = displacementY;
  displacement[3][counter] = displacementZ;
}
void printData() {
//  Serial.print("Gyro (deg)");
//  Serial.print(" X=");
//  Serial.print(rotX);
//  Serial.print(" Y=");
//  Serial.print(rotY);
//  Serial.print(" Z=");
//  Serial.print(rotZ);
  //Serial.print(" Accel (g)");
  Serial.print(" X=");
  Serial.print(gForceX);
  Serial.print(" === ");
  Serial.println(acceleration[1][counter]);
  Serial.print(" Y=");
  Serial.print(gForceY);
  Serial.print(" === ");
  Serial.println(acceleration[2][counter]);
  Serial.print(" Z=");
  Serial.print(gForceZ); //add "ln" later
  Serial.print(" === ");
  Serial.println(acceleration[3][counter]);
}

// convert to m/s^2 (g's of force to acceleration) and calculate speed (*2)
void getVelocity() {
  velocityX = ((gForceX - 0.06) * 9.81 * delayTime); 
//  Serial.print(" (m/s) X = ");
//  Serial.print(velocityX);
  
  velocityY = ((gForceY + 0.07) * 9.81 * delayTime);
//  Serial.print(" Y = ");
//  Serial.print(velocityY);
  
  velocityZ = ((gForceZ - 0.97) * 9.81 * delayTime);
//  Serial.print(" Z = ");
//  Serial.print(velocityZ);
}

// convert to m/s^2 (g's of force to acceleration) and calculate speed (*2)
void getDisplacement() {
  float displacementX = (velocityX * delayTime); 
//  Serial.print(" (m) X = ");
//  Serial.print(displacementX);
  
  float displacementY = (velocityY * delayTime);
//  Serial.print(" Y = ");
//  Serial.print(displacementY);
  
  float displacementZ = (velocityZ * delayTime);
//  Serial.print(" Z = ");
//  Serial.println(displacementZ);
}

