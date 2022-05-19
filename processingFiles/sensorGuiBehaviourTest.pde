
import processing.serial.*;
 
 
/*
* CONSTANTS
*/
int COLS = 16;
int ROWS = 12;
int CELL_SIZE = 20;
int END_MARKER = 0xFF;
int SCALE_READING = 2;
int min_force_thresh = 2;

boolean INVERT_X_AXIS = false;
boolean INVERT_Y_AXIS = false;
boolean SWAP_AXES = true;


/*
* GLOBALS
*/
Cell[][] grid;
Serial sPort;

int xcount = 0;
int ycount = 0;
boolean got_zero = false;


/*
* setup()
*/
void setup() 
{
  background(0, 0, 0);
  
  int port_count = Serial.list().length;
  sPort = new Serial(this, Serial.list()[port_count - 1], 115200);  
  
  size(CELL_SIZE * COLS, CELL_SIZE * ROWS);

  
  grid = new Cell[COLS][ROWS];
  for (int i = 0; i < COLS; i++)
  {
    for (int j = 0; j < ROWS; j++)
    {
      grid[i][j] = new Cell(i * CELL_SIZE, j * CELL_SIZE, CELL_SIZE, CELL_SIZE);
    }
  }
  delay(1000);
}

void draw() 
{
  sideStrip(0);
  sideStrip(8);
  //xyzPad();
}

boolean isBetween(int x, int lower, int upper) { //source: stackoverflow user missingfaktor 
  return lower <= x && x <= upper;
}

void cleanStrips(int sensor, int leave) {
  for(int i = sensor; i < sensor+8; i ++)
  {
    if (i != leave) updatePixel(i, ycount, 0); 
  }
}

void sideStrip(int sensor) {
    while (sPort.available() > 1) {
      byte force = (byte) sPort.read();
      byte position = (byte) sPort.read();
      int unsigned_force = force & 0xFF;
      int unsigned_position = position & 0xFF;
      //unsigned_force *= 1.5;   
      println(unsigned_force, unsigned_position);  
      if (unsigned_force > min_force_thresh)  // no bogus reading - touch actuation 
      {
        if (isBetween(unsigned_position,0,31)) {updatePixel(sensor, ycount, unsigned_force); cleanStrips(sensor, sensor);}  
        else if (isBetween(unsigned_position,32,63)) {updatePixel(sensor+1, ycount, unsigned_force); cleanStrips(sensor, sensor+1);}  
        else if (isBetween(unsigned_position,64,95)) {updatePixel(sensor+2, ycount, unsigned_force); cleanStrips(sensor, sensor+2);}  
        else if (isBetween(unsigned_position,96,127)) {updatePixel(sensor+3, ycount, unsigned_force); cleanStrips(sensor, sensor+3);}  
        else if (isBetween(unsigned_position,128,159)) {updatePixel(sensor+4, ycount, unsigned_force); cleanStrips(sensor, sensor+4);}  
        else if (isBetween(unsigned_position,160,191)) {updatePixel(sensor+5, ycount, unsigned_force); cleanStrips(sensor, sensor+5);}  
        else if (isBetween(unsigned_position,192,223)) {updatePixel(sensor+6, ycount, unsigned_force); cleanStrips(sensor, sensor+6);}  
        else if (isBetween(unsigned_position,224,255)) {updatePixel(sensor+7, ycount, unsigned_force); cleanStrips(sensor, sensor+7);}
      } 
   }
}

void xyzPad() 
{
  while(sPort.available() > 0)
  {
    byte got_byte = (byte) sPort.read();
    int unsigned_force = got_byte & 0xFF;
    if(unsigned_force == END_MARKER)
    {
      xcount = 0;
      ycount = 0;

    }
    else if(got_zero)
    {
      for(int i = 0; i < unsigned_force; i ++)
      {
        updatePixel(xcount, ycount+1, 0);
        xcount ++;
        if(xcount >= COLS)
        {
          xcount = 0;
          ycount ++;
          if(ycount >= ROWS-2)
          {
            ycount = ROWS - 1;
          }
        }
      }
      got_zero = false;
    }
    else if(got_byte == 0)
    {
      got_zero = true;
    }
    else
    {
      updatePixel(xcount, ycount+1, unsigned_force);
      xcount ++;
      if(xcount >= COLS)
      {
        xcount = 0;
        ycount ++;
        if(ycount >= ROWS-2)
        {
          ycount = ROWS - 1;
        }
      }
    }
  }
}

/*
* updatePixel()
*/
void updatePixel(int xpos, int ypos, int force)
{
  if(SWAP_AXES)
  {
    int temp = xpos;
    xpos = ypos;
    ypos = temp;
  }
  if((xpos < ROWS) && (ypos < COLS))
  {
    if(INVERT_Y_AXIS)
    {
      xpos = (COLS - 1) - xpos;
    }
    if(INVERT_X_AXIS)
    {
      ypos = (COLS - 1) - ypos;
    }
    grid[ypos][xpos].display(force);
  }
}

/*
* class Cell
*/
class Cell 
{
  float x, y;
  float w, h;
  int current_force = 0;
  float calibrated = 0;
  
  Cell(float tempX, float tempY, float tempW, float tempH) 
  {
    x = tempX;
    y = tempY;
    w = tempW;
    h = tempH;
  } 

  void display(int newforce) 
  {
    if(newforce < min_force_thresh)
    {
      newforce = 0;
    }
    else
    {
      newforce *= SCALE_READING;
    }
    if(newforce != current_force)
    {
      noStroke();
      fill(0, newforce, 0);
      rect(x, y, w, h); 
      current_force = newforce;
    }
  }
 
}







