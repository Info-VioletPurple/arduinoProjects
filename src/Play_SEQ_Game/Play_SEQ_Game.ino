/**
  Run Program
  Input:
  Points Required to run : NumPoints
  Function:
  Step 0: Wait to read the card.
  Step 1: Read the card. (Get UID and debug info). Check for failure.
  Step 2: Authenticate Sector 0 with private key. Check for failure.
  Step 3: Read the current Points and return to debug.
  Step 4: Check if CurrentPoints > NumPoints. Return failure if false. Log and return.
  Step 5: Decrement the points with NumPoints. Check for failure. Sector 0, Block 1.
  Step 6: Stop Communication with the card.
  Step 7: Perform Output.
  Step 8: Enable the game.
  Step 9: Log transaction.
  Output:
  Speaker:
  Success: Play Game. Total Number of Points are …..
  Failure: Failed to load NumPoints. Please try again….
  LED:
  Success: Green LED
  Failure: Red LED
  Display:
  Success: Loaded NumPoints Successfully. Total Number of Points are …..
  Failure: Failed to load NumPoints. Please try again….
*/

#include <SPI.h>
#include <CardUtil.h>
#include <MFRC522.h>


#define RST_PIN         9           // Pin Mapping on Arduino
#define SS_PIN          10          // Pin Mapping on Arduino
 

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

int numPoints = 0;  //Number of points to be charged.
int numRewards = 0;  //Number of rewards to be awarded.
const byte sequence[16] = {
  0x01, 0x02, 0x03, 0x00, 0x05, 0x06, 0x07, 0x08,
  0x01, 0x02, 0x03, 0x00, 0x05, 0x06, 0x07, 0x08
};

/**
   Initialize.
*/
void setup() {
  Serial.begin(9600); // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card

  Serial.println(F("Enter the number of points required to play the game."));
  while (numPoints == 0)
    numPoints = Serial.parseInt();
  Serial.print("NumPoints Required to play this game:");
  Serial.println(numPoints);
  
  Serial.println(F("Enter the number of rewards winning the game."));
  while (numRewards == 0)
    numRewards = Serial.parseInt();
  Serial.print("NumRewardss awarded on winning this game:");
  Serial.println(numRewards);  
  Serial.println(F("Scan a MIFARE Classic PICC to play the game."));
}

/**
   Main loop.
*/
void loop() {
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;

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
  CardUtil cardUtil(mfrc522);         //Create CardUtil instance
  
  CardUtil::Status status = cardUtil.chargePoints(numPoints);
  if(status.code == CardUtil::STATUS_OK) {
   //proceed with the game
   cardUtil.initSequence(sequence, numRewards); 
   delay(5000);      //delay for 5 seconds (Till the game is over. Don't read if the game is in progression.)
  } else if(status.code == CardUtil::STATUS_INSUFFICIENT_POINTS){
      
  } else {
    
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
