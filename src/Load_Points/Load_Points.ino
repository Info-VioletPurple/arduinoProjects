/**
Load Program
This program would be used to load points into the card for the player.
It initializes the card first time and resets the keys.
Input:
Points to be loaded : NumPoints
Function:
Step 0: Wait to read the card.
Step 1: Read the card. (Get UID and debug info). Check for failure.
Step 2: Authenticate Sector 1 with private key. Check for failure.
Step 3: Read the current Points and return to debug.
Step 4: Increment the points with NumPoints. Check for failure. Sector 0, Block 1.
Step 5: Stop Communication with the card.
Step 6: Perform Output.
Step 7: Log transaction.
Output:
Speaker:
 Success: Loaded NumPoints Successfully. Total Number of Points are …..
  Failure: Failed to load NumPoints. Please try again….
LED:
  Success: Green LED
  Failure: Red LED
Display:
  Success: Loaded NumPoints Successfully. Total Number of Points are …..
  Failure: Failed to load NumPoints. Please try again….
 */

#include <SPI.h>
#include <MFRC522.h>
#include <CardUtil.h>

#define RST_PIN         9           // Pin Mapping on Arduino
#define SS_PIN          10          // Pin Mapping on Arduino

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.


/**
 * Initialize.
 */
void setup() {
    Serial.begin(9600); // Initialize serial communications with the PC
    while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
    
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card
}

/**
 * Main loop.
 */
void loop() {
    int operation = 0;
    Serial.println("Enter Operation to be performed:");
    Serial.println("1 for Configure ");
    Serial.println("2 for Recharge ");
    Serial.println("3 for Check Balance");
    Serial.println("4 for Reset");
    Serial.println("5 for Check Status");
    while(operation == 0)
      operation = Serial.parseInt();
      
    int numPoints = 0;  //Number of points to be loaded.
    if(operation != 3 && operation != 5) {
      Serial.println(F("Enter the number of points to be loaded"));
      while(numPoints == 0)
        numPoints = Serial.parseInt();
    }
    Serial.println(F("Scan a MIFARE Classic PICC to Proceed."));
      
    // Look for new cards
    while ( ! mfrc522.PICC_IsNewCardPresent());

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
        return;

    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F("Card UID:"));
    dump_byte_array_internal(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.print(F("PICC type: "));
    byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    // Check for compatibility
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("This sample only works with MIFARE Classic cards."));
        return;
    }
    CardUtil cardUtil(mfrc522);         //Create CardUtil instance.
    switch(operation){
      case 1:
        cardUtil.configure(numPoints);
        break;
      case 2:
        cardUtil.addPoints(numPoints);
        break;
      case 3:
        cardUtil.getPoints();
        break;
       case 4:
        cardUtil.reset(numPoints);
        break;
       case 5:
        cardUtil.checkStatus();
        break;
    }
    cardUtil.stop();
}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array_internal(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
