/* XYZ pad calibration. 

Calibrates maximum conductance microSiemens value to use as maximum value when mapping to a byte
See comments on the FSR testing module for guidance on equations 
Values completely based on data given in teh appendix. should be calibrated for touch
based on mix input calibration http://www.arduino-hacks.com/arduino-vu-meter-lm386electret-microphone-condenser/ 
Based on https://learn.adafruit.com/force-sensitive-resistor-fsr/using-an-fsr */

int maximum=0; //declare and initialize maximum as zero
int minimum=2000; //declare and initialize minimum as 1023
int fsrPin = A5;    
int fsrReading;   
int fsrVoltage;   
unsigned long cellResistance;
unsigned long fsrConductance; 
 
void setup(void) {
  Serial.begin(9600);   
  analogReference(EXTERNAL);
}
 
void loop(void) {
  fsrReading = analogRead(fsrPin);  
  fsrVoltage = map(fsrReading, 0, 1023, 0, 3300);

  cellResistance = 3300 - fsrVoltage;    
  cellResistance *= 24000;             
  cellResistance /= fsrVoltage;
    
  fsrConductance = 1000000;           
  fsrConductance /= cellResistance;
  
  if (fsrConductance>maximum) maximum=fsrConductance; //record the fsrConductance value recieved 
  if (fsrConductance<minimum) minimum=fsrConductance; //record the fsrConductance value recieved 
  Serial.print("Maximum:\t");
  Serial.println(maximum);
  Serial.print("Minimum:\t");
  Serial.println(minimum);
   
  Serial.println("--------------------");
  delay(1000);
}