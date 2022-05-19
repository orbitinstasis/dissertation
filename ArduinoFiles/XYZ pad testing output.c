/* XYZ pad testing to check if calculations in accordance with calculations from appendix

Values completely based on data given in teh appendix. should be calibrated for touch
 
Based on https://learn.adafruit.com/force-sensitive-resistor-fsr/using-an-fsr */
unsigned short calibratedValue = 80; //value taken from the calibrator - must be set before execution
int fsrPin = A5;    
int fsrReading;   
int fsrVoltage;   
unsigned long cellResistance;  //resistance and conductance can be large values
unsigned long fsrConductance; 
byte mappedCellConductance;  // final value to be sent serially shown here to check for expected linearity and behaviour 
 
void setup(void) {
  Serial.begin(9600);   
  analogReference(EXTERNAL); //using 3.3V
}
 
void loop(void) {
  fsrReading = analogRead(fsrPin);  
  Serial.print("Analog reading = ");
  Serial.println(fsrReading); //V_out of v.divider

  fsrVoltage = map(fsrReading, 0, 1023, 0, 3300);
  Serial.print("Voltage reading in mV = ");
  Serial.println(fsrVoltage);  //mapping the reading to a mV
 
  if (fsrVoltage == 0) {
    Serial.println("No pressure");  
  } else {
    cellResistance = 3300 - fsrVoltage;     // rearranging v.divider equation for R_fsr
    cellResistance *= 24000;                // 24K resistor used in home setup. should be changed accordingly
    cellResistance /= fsrVoltage;
    Serial.print("FSR resistance in ohms = ");
    Serial.println(cellResistance);
    
    fsrConductance = 1000000;           // output in microSiemens
    fsrConductance /= cellResistance;
    Serial.print("Conductance in microSiemens: ");
    Serial.println(fsrConductance);
    
    if (fsrConductance >= calibratedValue) 	fsrConductance = calibratedValue; //setting max value whenever actual reading is over the limit
    {
      mappedCellConductance = map(fsrConductance, 0, calibratedValue, 0, 255); //outputting conductance in byte form
      Serial.print("Conductance in byte range: ");
      Serial.println(mappedCellConductance);
    }
  }
  Serial.println("--------------------");
  delay(500);
}