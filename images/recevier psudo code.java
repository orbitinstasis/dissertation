resumeSensor = 0; //this will hold the progress for any sensor state you are in  
enum retrieveState { 
        IN_STRIP_1, 
        IN_STRIP_2,
        IN_STRIP_3,
        IN_STRIP_4,
        IN_XYZ,
        READY    
        ;
    }
retrieveState = READY; //initial state

function updateReceivedData(byte[] data) { //pass in an array of bytes as often as possible, unknown size

	i = 0; //initialise the iterator 
	isFirstStateChange = false  //isFirstStateChange is always false *unless* you 
										 //enter the first while loop!
	if (i < buffer length) { //if there's data in the buffer
		while retrieveState == ready  {  //if the state machine is ready (just plugged in)
			if i >= buffer length  		//finished checking data and nothing found then exit method 
				return
			if data[i] == END_marker { //found what we want, change state to first sensor, 
									 //set isFirstStateChange, 
									 //increment i before while condition is false
				isFirstStateChange = true 
			}
			i++  //jump over marker position
		}
        if (i < buffer length) { //
            if (current data is end marker) {
                if (!isFirstStateChange) {
                    increment the state machine to next sensor 
                }
                resumeSensor = 0;
                i++; //jump over marker position
            }
            for (j = i; j < buffer length; j++) {
                tempVal = 0; // used to flipped the values of the values of the physical left side of the strip sensors  
                if (not current data is end marker && in a strip sensor state && (resumeSensor < MAX_STRIP_CELLS)) { // if you're reading force+position of strips
                    if ((state machine is in side strip 1 or 2) && resumeSensor == STRIP_POSITION)
                        tempVal = MAX_RECEIVE_VALUE - current unsigned byte;  //save flipped value 
                    else tempVal = current unsigned byte; //save normal value
                    stripModel[stripSensor][resumeSensor] = tempVal; //save this value to the current side strip model
                    resumeSensor++;
                    if (state machine is in side strip 4 && resumeSensor > STRIP_POSITION) {  //entering XYZ
                        increment the state machine to rear pad  
                        resumeSensor = 0; //reset all sub sensor data used to keep track of sensor data 
                        xCount = 0;
                        yCount = 0;
                    }
                }
                else { //data[i] is marker or first strip reading or in XYZ
                    if (state machine is in rear pad || current data is end marker) {
                        increment the state machine to next sensor 
                        resumeSensor = 0;
                    }
                    if (not current data is end marker && in a strip sensor state ) { // if you've entered a new strip
                        if ((state machine is in side strip 1 or 2) && resumeSensor == STRIP_POSITION)
                            tempVal = MAX_RECEIVE_VALUE - current unsigned byte;  // read comment above sig for explanation
                        else
                            tempVal = current unsigned byte;
                        stripModel[stripSensor][resumeSensor] = tempVal; //save this value to the current side strip model
                        resumeSensor++;
                    }
                    else if (!current data is end marker && state machine is in rear pad) {
                        //do XYZ stuff here
                        if (gotZero) { // if you're printing out arrays of zeroes (optimisation on firmware)
                            for (int z = 0; z < current unsigned byte; z++) { //for all succeeding zeroes 
                                padCell[xCount][yCount] = 0;  //save zero to the associated rear pad cell in the model
                                yCount++; //increment the y axis
                                if (yCount >= COLS) {   //check for overflow, i.e. process all y's in x then x+1's all y's etc
                                    yCount = 0;
                                    xCount++;
                                    if (xCount >= ROWS) {
                                        xCount = ROWS - 1;
                                    }
                                }
                            }
                            gotZero = false; //you are no longer reading a zero 
                        }
                        else if (current unsigned byte == 0) {  //current cell and n succeeding cells are zeroes  
                            gotZero = true;
                        }
                        else {
                            padCell[xCount][yCount] = current unsigned byte; //not a zero, is a force reading, save it to the associated cell in the model 
                            yCount++;  //increment the y axis
                            if (yCount >= COLS) { //check for overflow, i.e. process all y's in x then x+1's all y's etc
                                yCount = 0;
                                xCount++;
                                if (xCount >= ROWS) {
                                    xCount = ROWS - 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}