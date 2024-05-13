#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include "secrets.h"
#include "ThingSpeak.h"

//student, teacher, visitor

#define SS_PIN  D8 // The ESP8266 pin D8
#define RST_PIN D2 // The ESP8266 pin D2 

char ssid[] = SECRET_SSID;   // your network SSID (name) 
char pass[] = SECRET_PASS;   // your network password

MFRC522 rfid(SS_PIN, RST_PIN);

WiFiClient  client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo native USB port only
  }
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522

  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);     
    }
    Serial.println("\nConnected.");
  }
  
  WiFi.mode(WIFI_STA); 
  ThingSpeak.begin(client);  // Initialize ThingSpeak

  Serial.println("Scan a RFID tag to record attendance:");
}

void loop() {
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  if (rfid.PICC_IsNewCardPresent()) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been readed
      byte block;
      byte len;
      MFRC522::StatusCode status;

      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      Serial.print("RFID/NFC Tag Type: ");
      Serial.println(rfid.PICC_GetTypeName(piccType));

      String tagUID = readCardUID(rfid);
      Serial.print("Card UID");
      Serial.print(tagUID);
      Serial.println();

      // Fetching Name from the card
      byte buffer1[18];

      block = 1;
      len = 18;
      String name = readDataFromBlock(buffer1, block, len, status, key);
      Serial.print("Name: ");
      Serial.print(name);
      Serial.println();
      
      
      // Fetching Role from the card
      byte buffer2[18];
      block = 4;
      String role = readDataFromBlock(buffer2, block, len, status, key);
      Serial.print("Role: ");
      Serial.print(role);
      Serial.println();

      // Send attendance data to ThingSpeak
      sendAttendanceData(tagUID, name, role);

      rfid.PICC_HaltA(); // halt PICC
      rfid.PCD_StopCrypto1(); // stop encryption on PCD
    }
  }
}


String readCardUID(MFRC522 mfrc522){
  String tagUID = "";
  for (int i = 0; i < mfrc522.uid.size; i++) {
    tagUID.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    tagUID.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  return tagUID;
}

String readDataFromBlock(byte buffer[18], byte block, byte len, MFRC522::StatusCode status, MFRC522::MIFARE_Key key){
  String stringData = "";
  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(rfid.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return "error";
  }

  status = rfid.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return "error";
  }

  //Fetch and convert to String
  for (uint8_t i = 0; i < 16; i++)
  {
    if (buffer[i] != 32)
    {
      // Serial.write(buffer[i]);
      char charValue = char(buffer[i]);
      stringData.concat(String(charValue));
    }
  }

  return stringData;
}

void sendAttendanceData(String tagUID, String name, String role) {
  // int x = ThingSpeak.writeField(myChannelNumber, 3, tagUID, myWriteAPIKey);
  // int y = ThingSpeak.writeField(myChannelNumber, 1, name, myWriteAPIKey);
  // int z = ThingSpeak.writeField(myChannelNumber, 2, role, myWriteAPIKey);

  // set the fields with the values
  ThingSpeak.setField(3, tagUID);
  ThingSpeak.setField(1, name);
  ThingSpeak.setField(2, role);
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }

}



// sectreate keys

// Use this file to store all of the private credentials 
// and connection details

#define SECRET_SSID "CANALBOX-7869-2G"		// replace MySSID with your WiFi network name
#define SECRET_PASS "6KM4uYkppD"	// replace MyPassword with your WiFi password

#define SECRET_CH_ID 2544962			// replace 0000000 with your channel number
#define SECRET_WRITE_APIKEY "NZJSS4O0QUU27BX8"   // replace XYZ with your channel write API Key