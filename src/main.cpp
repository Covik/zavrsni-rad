#include <SPI.h>
#include "MFRC522.h"

#define RST_PIN         D1          // Configurable, see typical pin layout above
#define SS_PIN         D2         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

byte buffer[18];
byte block;
byte waarde[64][16];
MFRC522::StatusCode status;
    
MFRC522::MIFARE_Key key;


/*
 * Initialize.
 */
void setup() {
    Serial.begin(9600);         // Initialize serial communications with the PC
    while (!Serial);            // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
    SPI.begin();                // Init SPI bus
    mfrc522.PCD_Init();         // Init MFRC522 card

    key.keyByte[0] = 0xa0;
    key.keyByte[1] = 0xa1;
    key.keyByte[2] = 0xa2;
    key.keyByte[3] = 0xa3;
    key.keyByte[4] = 0xa4;
    key.keyByte[5] = 0xa5;
}

/*
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

byte* readBlock(byte block) {
    static byte buffer[18];

    byte byteCount = sizeof(buffer);
    status = mfrc522.MIFARE_Read(block, buffer, &byteCount);

    /*if (status == MFRC522::STATUS_OK) {
        // Dump block data
        Serial.print(F("Block ")); Serial.print(block); Serial.print(F(":"));
        dump_byte_array(buffer, 16);
    }*/

    return buffer;
} 

String getUser()
{
    byte block = 128;

    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
        return "";

    char sectorData[240];
    int currentSectorIndex = 0;

    for(; block < 143; block++) {
        // Read block
        byte *blockData = readBlock(block);

        if(blockData) {
            for(byte i = 0; i < 16; i++) 
                sectorData[currentSectorIndex++] = (char) blockData[i];
        }
    }

    if(!sectorData[0])
        return "";

    String modifiedData;

    for(int i = 0; i < 240; i++) {
        if((byte) sectorData[i] == 0x6d)
            break;

        if((byte) sectorData[i] == 0x0) {
            modifiedData += " ";

            if(sectorData[i+1] == 'E')
                i++;
        }
        else
            modifiedData += sectorData[i];
    }

    modifiedData.trim();
    modifiedData.toLowerCase();

    String newData;

    for(unsigned int i = 0; i < modifiedData.length(); i++) {
        int currentChar = (int) modifiedData.charAt(i);

        if((currentChar >= 97 && currentChar <= 122) || currentChar == 32)
            newData += (char) currentChar;
    }

    mfrc522.PICC_HaltA();       // Halt PICC
    mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
    return newData;
}

/*
 * Main loop.
 */
void loop() {
    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if (!mfrc522.PICC_IsNewCardPresent())
        return;

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial())
        return;

    // Show some details of the PICC (that is: the tag/card)
    /*Serial.print(F("Card UID:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));*/

    String user = getUser();

    Serial.println("USER:"+user);
    
    // http://arduino.stackexchange.com/a/14316
    if (!mfrc522.PICC_IsNewCardPresent())
        return;

    if (!mfrc522.PICC_ReadCardSerial())
        return;
}
