/*
 * Violet Purple.
 * A programme for using photosensitive diodes as sensor for motion detection.
 * TBD - Add the circuit diagramme here.
 */

const int LED = 9; //The pin for LED.
int val = 0;  //variable used to store the input
                //from the sensor.
int old_val = val;  //will be used to store old value of the input.
int state = 0;  //State of LED.
boolean down = false;

/**
 * Initial setup. Called Once during startup.
 */
void setup() {
  pinMode(LED, OUTPUT); //tell arduino, LED is output.
  Serial.begin(9600); //Set the serial connection at 9600b/s.
  //Analoguous inpits are automaticall configured 
  //as inputs in arduino.
}

/**
 * Main Loop.
 */
void loop() {
  val = analogRead(0);  //read the value from the sensor
  if(val > (old_val+10)) {
    if(down){ //Switch only if the signal has started increasing after a dip.
      Serial.println("switched");
      state = 1 - state;
    }
    old_val = val;  //update changed value
    down = false; //going up
    Serial.print("Read value:"); Serial.println(val);
    Serial.print("Old value:"); Serial.println(old_val);
    Serial.print("Down:"); Serial.println(down);
  }
  if(state) {
    digitalWrite(LED, HIGH); //Turn LED on.
  } else {
    digitalWrite(LED, LOW);
  }
  if( val < (old_val-10)){
    old_val = val;  //update changed value
    down = true;  //going down
    Serial.print("Read value:"); Serial.println(val);
    Serial.print("Old value:"); Serial.println(old_val);
    Serial.print("Down:"); Serial.println(down);
  } 
}
