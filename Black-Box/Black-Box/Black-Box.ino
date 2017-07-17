
// Project Black-Box 
// Black box designed for vehicles to record acceleration, velocity, displacement, and interactions between the driver and other vehicle
// features/controls such as windows, radio, accelerator pedal, and brake pedal.

#include <Wire.h>

// Calculate actual counter frequency 16Mhz/prescales
long counterFreq = 16e6/256;  

// Gyro & Accel storage
long accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ;
long gyroX, gyroY, gyroZ;
float rotX, rotY, rotZ;
float velocityX, velocityY, velocityZ;
float displacementX, displacementY, displacementZ;

// Gyro & Accel calculations 
int delayTime = 1;
int counter = 0;

// Arrays for briefly storing data
float rotation[3][10];
float acceleration[3][10];
float velocity[3][10];
float displacement[3][10];
float interupts [4][10];

// Timer variables 
unsigned long int seconds = 0; //timer calculated in seconds
int minutes = 0;
int hours = 0;
volatile unsigned long cycles = 0;
long period = 65535;
int oldSeconds = -1; // Keeps track of time to detect when new second is reached

//Interupt variables
bool interuptDetected = false;
String whichDistraction = ""; //Stores name of interupt that was triggered
int interuptInitialTime = 0;
int interuptFinalTime = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() //Main Setup Code
{
  Serial.begin(38400);
  Wire.begin();
  setupMPU();
  analogRead(3);
  attachInterrupt(0, distractedDriving, CHANGE); // interrupt 0 is mapped to pin 2 on the Uno, measure rising edges only
  cli();
  // clear bits for timer 0 
  TCCR1A = 0;
  TCCR1B = 0;
  // set timer prescaling to clk/256
  TCCR1B = ( 1 << 2 );
  TIMSK1 |= ( 7 << 0 );
  OCR1A = -1;
  OCR1B = period - 1;
  sei();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() { // Main program code
   
   duration(); // Calculate updated time values
  // delay(10); //Ensure values have been updated before if statement condition is tested
   if ( counter == 10) // resets the counter so it does not store data beyond the bounds of the array 
  {
   counter = 0;
  }

   if (seconds == (oldSeconds + 1)) // make so it outputs movement data once per second!
   { 
    printData(); //Print all data that could be stored in eeprom / hard memory 
    oldSeconds++;
   }

  if (interuptDetected) 
  {
   TCCR1B = 0; 
   whichInterupt();
   // set timer prescaling to clk/256 
   TCCR1B = ( 1 << 2 );
  }
  
  recordGyroRegisters(); // stores rotation data from the gyroscope
  recordAccelRegisters(); // stores acceleration data from the accelerometer
 
  getVelocity();  // converts acceleration data to velocity
  getDisplacement();  // converts acceleration data to displacement
  
  storeGyroData(); // stores X,Y,Z components of rotation in a matrix
  storeAccelData();  // stores X,Y,Z components of acceleration in a matrix
  storeVeloData(); // stores X,Y,Z components of velocity in a matrix
  storeDispData();  // stores X,Y,Z components of displacement in a matrix

  counter++; // increments counter to next position in the array for storage/display of values
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Timer functions

void duration()
{ 
  seconds = ( (TCNT1 + (cycles * period)) / counterFreq );
  minutes = seconds / 60;
  hours = minutes / 60;
}

void timeStamp()
{
  Serial.print("Time: ");
  Serial.print(hours);
  Serial.print("hr ");
  Serial.print(minutes - ( hours * 60 ) );
  Serial.print("min ");
  Serial.print( seconds - (minutes * 60) - (hours * 60));
  Serial.println("sec ");
}


ISR (TIMER1_COMPB_vect)
{
  TCNT1 = 0;
  cycles++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

// reads acceleration data
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

// scales acceleration data read
void processAccelData(){
  gForceX = accelX / 16384.0 * 9.81;
  gForceY = accelY / 16384.0 * 9.81; 
  gForceZ = accelZ / 16384.0 * 9.81;
}

// reads gyroscope data
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

// scales gyroscope data
void processGyroData() {
  rotX = gyroX / 131.0;
  rotY = gyroY / 131.0; 
  rotZ = gyroZ / 131.0;
}

// stores X,Y,Z components of rotation in a matrix
void storeGyroData()
{
  rotation[0][counter] = rotX;
  rotation[1][counter] = rotY;
  rotation[2][counter] = rotZ;
}

// stores X,Y,Z components of acceleration in a matrix
void storeAccelData()
{
  acceleration[0][counter] = gForceX;
  acceleration[1][counter] = gForceY;
  acceleration[2][counter] = gForceZ;
}

// stores X,Y,Z components of velocity in a matrix
void storeVeloData()
{
  velocity[0][counter] = velocityX;
  velocity[1][counter] = velocityY;
  velocity[2][counter] = velocityZ;
}

// stores X,Y,Z components of displacement in a matrix
void storeDispData()
{
  displacement[0][counter] = displacementX;
  displacement[1][counter] = displacementY;
  displacement[2][counter] = displacementZ;
}

// prints data
void printData() {
  Serial.println("////////////////////////////////////////////////////////////////////////////////////////////");
  timeStamp();

  //Print Gyro 
  Serial.print("Gyro: ");
  Serial.print("X = ");
  Serial.print(rotX);
  Serial.print("deg Y = ");
  Serial.print(rotY);
  Serial.print("deg Z = ");
  Serial.print(rotZ);
  Serial.println("deg");

  //Print Acceleration 
  Serial.print("Acceleration: ");
  Serial.print("X = ");
  Serial.print(acceleration[0][counter]);
  Serial.print("m/s^2 Y = ");
  Serial.print(acceleration[1][counter]);
  Serial.print("m/s^2 Z = ");
  Serial.print(acceleration[2][counter]);
  Serial.println("m/s^2");

  //Print Velocity
  Serial.print("Velocity: ");
  Serial.print("X = ");
  Serial.print(velocity[0][counter]);
  Serial.print("m/s Y = ");
  Serial.print(velocity[1][counter]);
  Serial.print("m/s Z = ");
  Serial.print(velocity[2][counter]);
  Serial.println("m/s");

  //Print Displacement
  Serial.print("Displacement: ");
  Serial.print("X = ");
  Serial.print(displacement[0][counter]);
  Serial.print("m Y = ");
  Serial.print(displacement[1][counter]);
  Serial.print("m Z = ");
  Serial.print(displacement[2][counter]);
  Serial.println("m");
}

// convert to m/s^2 (g's of force to acceleration) and calculate speed (*2)
void getVelocity() {
  velocityX = ((gForceX) * delayTime); 

  velocityY = ((gForceY) * delayTime);

  velocityZ = ((gForceZ) * delayTime);
}

// convert to m/s^2 (g's of force to acceleration) and calculate speed (*2)
void getDisplacement() {
  displacementX = (velocityX * delayTime); 
  
  displacementY = (velocityY * delayTime);

  displacementZ = (velocityZ * delayTime);
}

// Flag variable to run additional code in main loop --> (whichInterupt)
void distractedDriving()
{
  interuptDetected = true;
}

//Determine which interupt was flagged  
void whichInterupt()
{
  bool interuptReleased = false;
  int voltage = analogRead(3);
 
  if (voltage > 1000)
    interuptReleased = true; //Set to print ending message
  else if (voltage == 0)
    whichDistraction = "Window controls";
  else if (voltage < 50 && voltage > 0 )
    whichDistraction = "Radio controls";
  else if (voltage > 50 && voltage < 340)
    whichDistraction = "Brake pedal";
  else 
    whichDistraction = "Accelerator pedal";

  //Print distraction notice
  if (interuptReleased == false)
  {
    Serial.println("============================================================================================");
    interuptInitialTime = seconds;
    Serial.print(whichDistraction);
    Serial.println(" engaged");
    timeStamp();
  }
  else
  {
    Serial.println("============================================================================================");
    //Serial.println("////////////////////////////////////////////////////////////////////////////////////////////");
    interuptFinalTime = seconds;
    int changeintime = (interuptFinalTime - interuptInitialTime);
    Serial.print(whichDistraction);
    Serial.print(" released after ");
    
    if (changeintime == 0)
    Serial.print("<1");
    else
    Serial.print(changeintime);
    
    Serial.println(" seconds");
    interuptReleased = false;
    timeStamp();
  }
  
  interuptDetected = false;
}

//void storeDistraction ()
//{
//  String distractionType[] =
//  String timeStamp[] = 
//}

   // EEPROM.clear();
   // EEPROM.write();


