import processing.serial.*;
/*
* GLOBALS
 */
Serial sPort;
int markerCount = 0; 
int startTime; 
int timeDifference = 0; 
/*
* setup()
 */
void setup() 
{
  int port_count = Serial.list().length; //hold all open ports
  sPort = new Serial(this, Serial.list()[port_count - 1], 230400); //assign sPort to open com port 
  startTime = millis();
}

void draw() 
{
  timeDifference = millis() - startTime; 
  if (timeDifference <= 1000) {
    while (sPort.available() > 0) {
      byte byteIn = (byte) sPort.read();
      int uIntIn = byteIn & 0xFF;
      if (uIntIn == 255) {
        markerCount++;
      }
    } 
  } else {
    println(markerCount, timeDifference);
    exit();
  }
}