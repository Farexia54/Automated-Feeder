#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM
#include <SPI.h>        // RC522 Module uses SPI protocol
#include <MFRC522.h>    // Library for Mifare RC522 Devices

#define COMMON_ANODE
#define LED_ON HIGH
#define LED_OFF LOW

#define redLed 7    // Set Led Pins
#define blueLed 6
#define greenLed 5

#define DutchTray 4     // Triggers the tray with Dutch's food
#define MidnightTray 2   // Triggers the tray with Midnight's food

bool programMode = false;  // initialize programming mode to false

uint8_t successRead;    // Variable integer to keep if we have Successful Read from Reader

byte storedCard[4];   // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
byte masterCard[4];   // Stores master card's ID read from EEPROM

// Create MFRC522 instance
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

///////////////////////////////////////// Setup ///////////////////////////////////
void setup() 
{
  //Arduino Pin Configuration
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(DutchTray, OUTPUT);
  
  digitalWrite(DutchTray, HIGH);    // Make sure door is locked
  digitalWrite(redLed, LED_OFF);  // Make sure led is off
  digitalWrite(greenLed, LED_OFF);  // Make sure led is off
  digitalWrite(blueLed, LED_OFF); // Make sure led is off

  //Protocol Configuration
  Serial.begin(9600);  // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  if (EEPROM.read(1) != 143) 
  {
    Serial.println(F("No Master Card Defined"));
    Serial.println(F("Scan A PICC to Define as Master Card"));
      do 
      {
        successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0
        digitalWrite(blueLed, LED_ON);    // Visualize Master Card need to be defined
        delay(200);
        digitalWrite(blueLed, LED_OFF);
        delay(200);
      }
    while (!successRead);                  // Program will not go further while you not get a successful read
    for ( uint8_t j = 0; j < 4; j++ )      // Loop 4 times
    {                                      
      EEPROM.write( 2 + j, readCard[j] );  // Write scanned PICC's UID to EEPROM, start from address 3
    }
    EEPROM.write(1, 143);                  // Write to EEPROM we defined Master Card.
    Serial.println(F("Master Card Defined"));
  }
  
  Serial.println(F("-------------------"));
  Serial.println(F("Master Card's UID"));
  for ( uint8_t i = 0; i < 4; i++ )        // Read Master Card's UID from EEPROM
  {          
    masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard
    Serial.print(masterCard[i], HEX);
  }
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Everything is ready"));
  Serial.println(F("Waiting PICCs to be scanned"));
  cycleLeds();    // Everything ready lets give user some feedback by cycling leds
}


///////////////////////////////////////// Main Loop ///////////////////////////////////
void loop () 
{
   do 
   {
      successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0
      if (programMode) 
      {
         cycleLeds();              // Program Mode cycles through Red Green Blue waiting to read a new card
      }
      else 
      {
         normalModeOn();     // Normal mode, blue Power LED is on, all others are off
      }
   }
  while (!successRead);   //the program will not go further while you are not getting a successful read
  if (programMode) 
  {
    if ( isMaster(readCard) )  //When in program mode check First If master card scanned again to exit program mode
    { 
      Serial.println(F("Master Card Scanned"));
      Serial.println(F("Exiting Program Mode"));
      Serial.println(F("-----------------------------"));
      programMode = false;
      return;
    }
     else 
     {
      if ( findID(readCard) ) { // If scanned card is known, it will be deleted
        Serial.println(F("I know this PICC, removing..."));
        deleteID(readCard);
        Serial.println("-----------------------------");
        Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
      }
      else 
      {                    // If scanned card is not known, it will be added
        Serial.println(F("I do not know this PICC, adding..."));
        writeID(readCard);
        Serial.println(F("-----------------------------"));
        Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
      }
     }
  }
  else 
  {
    if ( isMaster(readCard))          // If scanned card's ID matches Master Card's ID - enter program mode
    {    
      programMode = true;
      Serial.println(F("Hello Master - Entered Program Mode"));
      uint8_t count = EEPROM.read(0);   // Read the first Byte of EEPROM that
      Serial.print(F("I have "));     // stores the number of ID's in EEPROM
      Serial.print(count);
      Serial.print(F(" record(s) on EEPROM"));
      Serial.println("");
      Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
      Serial.println(F("Scan Master Card again to Exit Program Mode"));
      Serial.println(F("-----------------------------"));
    }
    else 
    {
      if ( findID(readCard) == (mfrc522.uid.uidByte[0] == 0x7C &&        // Check to see if it matches Dutch's tag
     mfrc522.uid.uidByte[1] == 0x68 &&                                 
     mfrc522.uid.uidByte[2] == 0x15 &&
     mfrc522.uid.uidByte[3] == 0x31)) 
     { 
        Serial.println(F("Welcome Dutch, enjoy your meal!"));
        DutchGranted(20);         // Impulse that lasts 20 ms
      }
      if ( findID(readCard) == (mfrc522.uid.uidByte[0] == 0x00 &&       // Check to see if it matches Midnight's tag
     mfrc522.uid.uidByte[1] == 0x00 &&
     mfrc522.uid.uidByte[2] == 0x00 &&
     mfrc522.uid.uidByte[3] == 0x00)) 
     { 
        Serial.println(F("Welcome Midnight, enjoy your meal!"));
        MidnightGranted(20);         // Impulse that lasts 20 ms
     }
      
     }
  }
}

/////////////////////////////////////////  Access Granted To Dutch's Food   ///////////////////////////////////
void DutchGranted ( uint16_t setDelay) 
{
  digitalWrite(blueLed, LED_OFF);   // Turn off blue LED
  digitalWrite(redLed, LED_OFF);    // Turn off red LED
  digitalWrite(greenLed, LED_ON);   // Turn on green LED
  digitalWrite(DutchTray, LOW);     // Low signal to the CD drive for x amount of milliseconds
  delay(setDelay);                  
  digitalWrite(DutchTray, HIGH);    // Toggle back to default signal
  delay(1000);                      // Hold green LED on for a second
}

/////////////////////////////////////////  Access Granted To Midnight's Food   ///////////////////////////////////
void MidnightGranted ( uint16_t setDelay) 
{
  digitalWrite(blueLed, LED_OFF);     // Turn off blue LED
  digitalWrite(redLed, LED_OFF);      // Turn off red LED
  digitalWrite(greenLed, LED_ON);     // Turn on green LED
  digitalWrite(MidnightTray, LOW);    // Low signal to the CD drive for x amount of milliseconds
  delay(setDelay);                    
  digitalWrite(MidnightTray, HIGH);   // Toggle back to default signal
  delay(1000);                        // Hold green LED on for a second
}

///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied() 
{
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  digitalWrite(redLed, LED_ON);     // Turn on red LED
  delay(1000);
}


///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
uint8_t getID() 
{
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent())   //If a new PICC is placed to the RFID reader, continue
  {
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {   //Since a PICC placed get Serial and continue
    return 0;
  }
  
  Serial.println(F("Scanned PICC's UID:"));
  for ( uint8_t i = 0; i < 4; i++) 
  {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

///////////////////////////////////////// Cycle Leds (Program Mode) ///////////////////////////////////
void cycleLeds() 
{
  digitalWrite(redLed, LED_OFF);    // Make sure red LED is off
  digitalWrite(greenLed, LED_ON);   // Make sure green LED is on
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  delay(200);
  digitalWrite(redLed, LED_OFF);    // Make sure red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  digitalWrite(blueLed, LED_ON);    // Make sure blue LED is on
  delay(200);
  digitalWrite(redLed, LED_ON);     // Make sure red LED is on
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  delay(200);
}

//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn () 
{
  digitalWrite(blueLed, LED_ON);    // Blue LED ON to show system ready to read card
  digitalWrite(redLed, LED_OFF);    // Make sure Red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure Green LED is off
  digitalWrite(DutchTray, LOW);     // Make sure tray is retracted
}

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( uint8_t number ) 
{
  uint8_t start = (number * 4 ) + 2;    // Figure out starting position
  for ( uint8_t i = 0; i < 4; i++ ) 
  {     // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
  }
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) 
{
  if ( !findID( a ) )                  // Before we write to the EEPROM, check to see if we have seen this card before!
  {     
    uint8_t num = EEPROM.read(0);         // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t start = ( num * 4 ) + 6;      // Figure out where the next slot starts
    num++;                                // Increment the counter by one
    EEPROM.write( 0, num );               // Write the new count to the counter
    for ( uint8_t j = 0; j < 4; j++ ) 
    {   // Loop 4 times
      EEPROM.write( start + j, a[j] );    // Write the array values to EEPROM in the right position
    }
    successWrite();
    Serial.println(F("Succesfully added ID record to EEPROM"));
  }
  else 
  {
    failedWrite();
    Serial.println(F("Failed! There is something wrong with the ID or bad EEPROM"));
  }
}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) 
{
  if ( !findID( a ) )          // Before we delete from the EEPROM, check to see if we have this card
  {    
    failedWrite();             // If not
    Serial.println(F("Failed to delete! There is something wrong with ID or bad EEPROM"));
  }
  else {
    uint8_t num = EEPROM.read(0);                             // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t slot;                                             // Figure out the slot number of the card
    uint8_t start;                                            // = ( num * 4 ) + 6; // Figure out where the next slot starts
    uint8_t looping;                                          // The number of times the loop repeats
    uint8_t j;
    uint8_t count = EEPROM.read(0);                           // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );                                   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;                                                    // Decrement the counter by one
    EEPROM.write( 0, num );                                   // Write the new count to the counter
    for ( j = 0; j < looping; j++ )                           // Loop the card shift times
    {                        
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( uint8_t k = 0; k < 4; k++ )                         // Shifting loop
    {                      
      EEPROM.write( start + j + k, 0);
    }
    successDelete();
    Serial.println(F("Succesfully removed ID record from EEPROM"));
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
bool checkTwo ( byte a[], byte b[] ) 
{   
  for ( uint8_t k = 0; k < 4; k++ ) 
  {   // Loop 4 times
    if ( a[k] != b[k] )       // IF a != b then false, because: one fails, all fail
    {  
       return false;
    }
  }
  return true;  
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
uint8_t findIDSLOT( byte find[] ) 
{
  uint8_t count = EEPROM.read(0);          // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ )   // Loop once for each EEPROM entry
  {    
    readID(i);                             // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) )    // Check to see if the storedCard read from EEPROM
    {                                      // is the same as the find[] ID card passed
      return i;                            // The slot number of the card
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
bool findID( byte find[] ) 
{
  uint8_t count = EEPROM.read(0);         // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i < count; i++ )   // Loop once for each EEPROM entry
  {    
    readID(i);                            // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) )   // Check to see if the storedCard read from EEPROM
    {   
      return true;
    }
    else 
    {    // If not, return false
       return false;
    }
  }
  return false;
}

///////////////////////////////////////// Write Success to EEPROM   ///////////////////////////////////
// Flashes the green LED 3 times to indicate a successful write to EEPROM
void successWrite() 
{
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  digitalWrite(redLed, LED_OFF);    // Make sure red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is on
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Make sure green LED is on
  delay(200);
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Make sure green LED is on
  delay(200);
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Make sure green LED is on
  delay(200);
}

///////////////////////////////////////// Write Failed to EEPROM   ///////////////////////////////////
void failedWrite()                  // Flashes the red LED 3 times to indicate a failed write to EEPROM
{
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  digitalWrite(redLed, LED_OFF);    // Make sure red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  delay(200);
  digitalWrite(redLed, LED_ON);   // Make sure red LED is on
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  delay(200);
  digitalWrite(redLed, LED_ON);   // Make sure red LED is on
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  delay(200);
  digitalWrite(redLed, LED_ON);   // Make sure red LED is on
  delay(200);
}

///////////////////////////////////////// Success Remove UID From EEPROM  ///////////////////////////////////
void successDelete()                // Flashes the blue LED 3 times to indicate a success delete to EEPROM
{
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  digitalWrite(redLed, LED_OFF);    // Make sure red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  delay(200);
  digitalWrite(blueLed, LED_ON);    // Make sure blue LED is on
  delay(200);
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  delay(200);
  digitalWrite(blueLed, LED_ON);    // Make sure blue LED is on
  delay(200);
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  delay(200);
  digitalWrite(blueLed, LED_ON);    // Make sure blue LED is on
  delay(200);
}

////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
bool isMaster( byte test[] ) 
{
	return checkTwo(test, masterCard);
}

bool monitorWipeButton(uint32_t interval) 
{
  uint32_t now = (uint32_t)millis();
  while ((uint32_t)millis() - now < interval)   // check on every half a second
  {
  if (((uint32_t)millis() % 500) == 0)
  {
  return true;
  }
 }
}
