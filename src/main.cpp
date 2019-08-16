#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#define OPENED_PIN  D4
#define RST_PIN     D1
#define SS_PIN      D2
#define GREEN_PIN   D3
#define RED_PIN     D0
#define LOCK_PIN    D8

MFRC522 mfrc522(SS_PIN, RST_PIN);

MFRC522::StatusCode status;
    
MFRC522::MIFARE_Key key;

void lock();

/*
 * Initialize.
 */
void setup() {
    Serial.begin(9600);
    while (!Serial.availableForWrite());

    SPI.begin();
    mfrc522.PCD_Init();

    pinMode(LOCK_PIN, OUTPUT);
    lock();

    pinMode(OPENED_PIN, INPUT_PULLUP);
    
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    digitalWrite(RED_PIN, LOW);
    analogWrite(GREEN_PIN, 0);

    key.keyByte[0] = 0xa0;
    key.keyByte[1] = 0xa1;
    key.keyByte[2] = 0xa2;
    key.keyByte[3] = 0xa3;
    key.keyByte[4] = 0xa4;
    key.keyByte[5] = 0xa5;

    /*WiFiManager wifiManager;
    wifiManager.setConnectTimeout(10);
    wifiManager.autoConnect("ESP8266", "testpass");*/
}

byte* readBlock(byte block) {
    static byte buffer[18];

    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
        return nullptr;

    byte byteCount = sizeof(buffer);
    status = mfrc522.MIFARE_Read(block, buffer, &byteCount);

    /*(status == MFRC522::STATUS_OK) {
        // Dump block data
        Serial.print(F("Block ")); Serial.print(block); Serial.print(F(":"));
        dump_byte_array(buffer, 16);
        Serial.println();
    }*/

    return buffer;
} 

String getUser()
{
    byte block = 128;

    char sectorData[240];
    int currentSectorIndex = 0;

    for(; block < 143; block++) {
        byte *blockData = readBlock(block);

        if(blockData == nullptr)
            break;

        for(byte i = 0; i < 16; i++) 
            sectorData[currentSectorIndex++] = (char) blockData[i];
    }

    int lastBlock = block;

    mfrc522.PICC_HaltA();       // Halt PICC
    mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD

    if(!sectorData[0] || lastBlock != 143)
        return "";

    String modifiedData;

    for(int i = 0; i < 240; i++) {
        if((byte) sectorData[i] == 0x6d)
            break;

        if((byte) sectorData[i] == 0x0) {
            modifiedData += " ";
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

    return newData;
}

void lock() {
    digitalWrite(LOCK_PIN, LOW);
}

void unlock() {
    digitalWrite(LOCK_PIN, HIGH);
}

/*
 * Main loop.
 */
void loop() {
    if (!mfrc522.PICC_IsNewCardPresent())
        return;

    if (!mfrc522.PICC_ReadCardSerial())
        return;

    int isOpened = digitalRead(OPENED_PIN);
    String user = getUser();
    user.trim();
    Serial.println("User: " + user);
    Serial.print("Opened: ");
    Serial.println(isOpened);

    if(user != "" && isOpened == 0) {
        digitalWrite(RED_PIN, LOW);
        analogWrite(GREEN_PIN, 1023);

        unlock();
        delay(4000);        
        lock();

        //delay(13000);
    }
    else {
        digitalWrite(RED_PIN, HIGH);
        analogWrite(GREEN_PIN, 0);
    }
    
    delay(4000);

    digitalWrite(RED_PIN, LOW);
    analogWrite(GREEN_PIN, 0);

    // http://arduino.stackexchange.com/a/14316
    if (!mfrc522.PICC_IsNewCardPresent())
        return;

    if (!mfrc522.PICC_ReadCardSerial())
        return;
}
