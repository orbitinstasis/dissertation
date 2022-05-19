/**********************************************************************************************************
* Project: Haptic Case - ECE56
* By: 74120 -- xyz portion partly based off of example code from sensitronics 
* LastRev: 28/10/2014
* Description: This is the unoptimized / not refactored code for the XYZ pad; 
    checking the output of this code via the PCB on a windows terminal (through USB) should make debugging much easier 

    for the atmega640 100A w/ 1 16 bit mux 
**********************************************************************************************************/
/*
the following needs to happen for each cell of the XYZ pad:
  --Drive the cell's column with +5V
  --Enable the cell's row (multiplexer channel)
  --Take an ADC reading on the ADC read pin 
  --Output the reading via serial port
  --Iterate for each column, then repeat process for the next row
TO-DO: 
  add corresponding section for side strips 
  convert this into the correct C syntax 
*/

/**********************************************************************************************************
* MACROS / PIN DEFS
**********************************************************************************************************/
#define BAUD_RATE                 115200
#define ROW_COUNT                 10
#define COLUMN_COUNT              16

#define PIN_XYZ_ADC_INPUT             A0 //CHANGE HERE
#define PIN_MUX_CHANNEL_0         4  //channel pins 0, 1, 2, etc must be wired to consecutive Arduino pins
#define PIN_MUX_CHANNEL_1         5
#define PIN_MUX_CHANNEL_2         6
#define PIN_MUX_CHANNEL_3         7
#define PIN_MUX_INHIBIT_0         8  //inhibit = active low enable. All mux IC enables must be wired to consecutive Arduino pins
#define COLUMN_DRIVE_PINS[16]     {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16} // an array for each column pins address - i'th column lives at index i

#define CHANNEL_PINS_PER_MUX      4


/**********************************************************************************************************
* GLOBALS
**********************************************************************************************************/



/**********************************************************************************************************
* setup()
**********************************************************************************************************/
void setup()
{
  Serial.begin(BAUD_RATE);
  unsigned short shiftColumnValue = 15;             //to know which column is active --- initalise  to the final column 
  pinMode(PIN_XYZ_ADC_INPUT, INPUT);
  pinMode(PIN_MUX_CHANNEL_0, OUTPUT);
  pinMode(PIN_MUX_CHANNEL_1, OUTPUT);
  pinMode(PIN_MUX_CHANNEL_2, OUTPUT);
  pinMode(PIN_MUX_CHANNEL_3, OUTPUT);
  pinMode(PIN_MUX_INHIBIT_0, OUTPUT); 
  for (int i = 0; i < COLUMN_DRIVE_PINS.length; i++)    // initialise all columns to ground 
  {
    pinMode(COLUMN_DRIVE_PINS[i], OUTPUT);
    digitalWrite(COLUMN_DRIVE_PINS, LOW); 
  }
	unsigned short calibratedValue = 80; //value taken from the calibrator - must be set before execution   
	int fsrVoltage;   
	unsigned long cellResistance;  //resistance and conductance can be large values
	unsigned long fsrConductance; 
	byte mappedCellConductance;   
}


/**********************************************************************************************************
* loop()
**********************************************************************************************************/
void loop()
{
  // FOR THE XYZ PAD 
  for(int i = 0; i < ROW_COUNT; i ++)                             // for all rows 
  { 
    setRowMuxSelectors(i);                                        // set the read row high                          
    shiftColumn();                                                // send out a 1 
    for(int j = 0; j < COLUMN_COUNT; j ++)
    {
	  fsrReading = analogRead(PIN_XYZ_ADC_INPUT); 
	  fsrVoltage = map(fsrReading, 0, 1023, 0, 3300); // mapping the 2^10 analogue v-reading to mV ranging from 0 to V_cc
	  cellResistance = 3300 - fsrVoltage;     // rearranging v.divider equation for R_fsr
	  cellResistance *= 22000;                //  should be changed accordingly for R 
  	  cellResistance /= fsrVoltage;		 // rearranging V-divider equation for fsr_cell_resistance = ((Vcc - V) * R) / V 	
	  fsrConductance = 1000000;           // output in microSiemens
      fsrConductance /= cellResistance;
    if (fsrConductance >= calibratedValue) 	fsrConductance = calibratedValue; //setting max value whenever actual reading is over the limit
    { // conductance from microSiemens to byte representation - max conductance value taken from calculations in the appendix 
      mappedCellConductance = map(fsrConductance, 0, calibratedValue, 0, 255); //outputting conductance in byte form
    }    
	  shiftColumn();                                                         // shift is done to keep range in 8 bits 
      printFixedXYZValues(mappedCellConductance);                            // lowByte extracts just the byte of varying data we care about 
      Serial.print(" ");                                                     // for padding 
    }
    Serial.println();
  }
  Serial.println();
  delay(200);                                                     // for debugging 

  // FOR THE SIDE STRIPS calling the same position / pressure functions for each strip, printing debug data in terminal for each 

  /*
  for each side strip, pressure() and position() saving both to a var -- doing similar analogRead to above -- then similar printFixedSideStrip() 
  */
}


/**********************************************************************************************************
* setRowMuxSelectors() - set the selector pins on the mux dependant on which row we're reading 
**********************************************************************************************************/
void setRowMuxSelectors(int row_number)
{ 
  for(int i = 0; i < CHANNEL_PINS_PER_MUX; i++)          // here we set the selector bits dependant on which row we want to read 
  {                                                       // for each selector pin
    if (bitRead(row_number, i))                            // check i'th bit of the row number (argument of the function)
    {
      digitalWrite(PIN_MUX_CHANNEL_0 + i, HIGH);          // set that i'th selector pin dependant on what was read 
    }
    else
    {
      digitalWrite(PIN_MUX_CHANNEL_0 + i, LOW);
    }
  }  
}


/**********************************************************************************************************
* shiftColumn() - Driving start column with all other columns grounded - repeat for each column 
**********************************************************************************************************/
void shiftColumn()
{
  digitalWrite(COLUMN_DRIVE_PINS[shiftColumnValue], LOW);         // SET the last col low and next one high     
  if (shiftColumnValue >= COLUMN_COUNT - 1)                         //if it's the final column 
  {
    shiftColumnValue = 0; 
  } else                                                            // if it's not the last column  
  {
    shiftColumnValue++;
  }
  digitalWrite(COLUMN_DRIVE_PINS[shiftColumnValue], HIGH);
}


/**********************************************************************************************************
* printFixedXYZValues() - print a value padded with leading spaces such that the value always occupies a fixed
* number of characters / space in the output terminal.
**********************************************************************************************************/
void printFixedXYZValues(byte value)
{
  if(value < 10)
  {
    Serial.print("  ");
  }
  else if(value < 100)
  {
    Serial.print(" ");
  }
  Serial.print(value);
}