/**********************************************************************************************************
twat
**********************************************************************************************************/
/**********************************************************************************************************
* PIN DEFS / MACROS
**********************************************************************************************************/
#define Terminal3           2
#define PIN_WIPER           A0
#define FixedResBottom      3
#define Terminal2           4

#define SERIAL_BAUD_RATE    115200
#define PER_CYCLE_DELAY     25
#define TOUCH_THRESH        2 
unsigned short calibratedValue = 80; //value taken from the calibrator - must be set before execution  
int FSLP_Voltage;   
unsigned long FSLP_Res;  //resistance and conductance can be large values
unsigned long FSLP_Conductance; 
byte mappedFLSpConductance;  // final value to be sent serially shown here to check for expected linearity and behaviour 
 
/**********************************************************************************************************
* GLOBALS
**********************************************************************************************************/

/**********************************************************************************************************
* setup()
**********************************************************************************************************/
void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  pinMode(Terminal2, OUTPUT);
  digitalWrite(Terminal2, HIGH); //setting Left side to V_cc
  pinMode(Terminal3, OUTPUT);
  pinMode(FixedResBottom, INPUT);
  pinMode(PIN_WIPER, INPUT);
  analogReference(EXTERNAL); //using 3.3V
}

/**********************************************************************************************************
* loop()
**********************************************************************************************************/
void loop() 
{
  /*** First read force ***/
  pinMode(FixedResBottom, OUTPUT);
  digitalWrite(FixedResBottom, LOW);  //Ground reference divider resistor
  digitalWrite(Terminal3, HIGH);     //Both ends of pot are at +5V
  delay(5);
  int FSLP_Force = analogRead(PIN_WIPER); 
  FSLP_Voltage = map(FSLP_Force, 0, 1023, 0, 3300); //mapping the reading to a mV
  FSLP_Res = 3300 - FSLP_Voltage;     // rearranging v.divider equation for R_fsr
  FSLP_Res *= 4700;                // 4K7 resistor used in home setup. should be changed accordingly
  FSLP_Res /= FSLP_Voltage;
  FSLP_Conductance = 1000000;           // output in microSiemens
  FSLP_Conductance /= FSLP_Res;
  if (FSLP_Conductance >= calibratedValue) 	FSLP_Conductance = calibratedValue; //setting max value whenever actual reading is over the limit
  {
    mappedFLSpConductance = map(FSLP_Conductance, 0, calibratedValue, 0, 255); //outputting conductance in byte form
  }

  /*** Now read position ***/
  clear();// PUT EACH OF THE FOUR STRIPS PIN ADDRESSES IN ARRAYS AND PASS THE SPECIFIC ARRAY TO THIS FUNC
  pinMode(FixedResBottom, INPUT);     //Effectively disconnect reference divider resistor
  digitalWrite(Terminal3, LOW);      //Left end of pot is at +5V, Right end is at GND
  digitalWrite(FixedResBottom, LOW); //draining any charge that may have capactively coupled to sense line during transition of sensor terminals
  //make FixedResBottom high-Z
  //set wiper to ADC input in atmel
  delay(5); //increases sensitivity of sensor for light touches (5 ms delay) 
  byte position_reading = map(analogRead(PIN_WIPER),0,1024,0,255);  // save position as byte (from 10 bit)
  
  /*** If readings are valid, output position and force ***/
  if(FSLP_Force >= TOUCH_THRESH)  //With no force on the sensor the wiper is floating, position readings would be bogus
  {
    Serial.print("Position: ");
    printFixed(position_reading);
    Serial.print("Force: ");
    printFixed(mappedFLSpConductance);
    Serial.println();
  }
  else
  {
    Serial.println("No touch deteced");
  }
  
  delay(PER_CYCLE_DELAY);
}

void clear()
{
  pinMode(Terminal2, OUTPUT);
  pinMode(Terminal3, OUTPUT);
  pinMode(FixedResBottom, OUTPUT);
  pinMode(PIN_WIPER, OUTPUT);   
  digitalWrite(Terminal2, LOW); 
  digitalWrite(Terminal3, LOW); 
  digitalWrite(FixedResBottom, LOW); 
  digitalWrite(PIN_WIPER, LOW); 
}
/**********************************************************************************************************
* printFixed() - Serial Print with space padding for consistent positioning in terminal.
**********************************************************************************************************/
void printFixed(int value)
{
  if(value < 10)
  {
    Serial.print(value);
    Serial.print("     ");
  }
  else if (value < 100)
  {
    Serial.print(value);
    Serial.print("    ");
  }
  else 
    Serial.print(value);
    Serial.print("   ");
}