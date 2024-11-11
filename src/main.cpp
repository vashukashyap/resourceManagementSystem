#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include <SD.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <FirebaseJson.h>




void handlePortal(); // handle for local webpages
void resetEEPROM(struct WiFiInfo WiFiInfo); // clear store WiFi password
WiFiInfo readWiFiInfo();
void writeWiFiInfo(const WiFiInfo&);
File readHTMLFile();


// FireBase URL and API 
#define DATABASE_URL "YOUR-FIREBASE-REALTIME-DATABASE-URL"
#define API_KEY "YOUR-FIREBSE-API"

// PINS
#define INBUILD_LED D4
#define WATER_SENSOR_PIN A0



// structure for storing wifi information
struct WiFiInfo
{
    char ssid[30];
    char password[30];

} userWifiInfo;


FirebaseData fbdo; // firebase object
FirebaseAuth auth;     // firebase auth object
FirebaseConfig config; // firebase config object
FirebaseJson HomeData; // json object to store data



FirebaseJson data;

bool openAP = false; // status of AP (default: false)
unsigned long sendDataPrevMillis = 0; // time of last request
int watervalue = 0;
bool signupOK = false; //status of SignUp

ESP8266WebServer server(80); // init server


void setup()
{   
    

    Serial.begin(115200); // Serial Monitor at 115200
    pinMode(INBUILD_LED, OUTPUT);

    if(!LittleFS.begin())
    {
        Serial.println("unable to mount LittleFS");
    }


    userWifiInfo = readWiFiInfo();

    // EEPROM.begin(sizeof(struct WiFiInfo)); //init EEPROM
    // EEPROM.get(0, userWifiInfo); // getting WiFi info from EEPROM

    Serial.println(userWifiInfo.ssid);
    Serial.println(userWifiInfo.password);
    
    WiFi.mode(WIFI_STA); // setting WiFi mode to station
    WiFi.begin(userWifiInfo.ssid, userWifiInfo.password); // conneting to station

    // connecting to WiFi
    byte tries = 0;
    while(WiFi.status() != WL_CONNECTED)
    {
        // blink led while connecting
        digitalWrite(INBUILD_LED, HIGH);
        delay(500);
        digitalWrite(INBUILD_LED, LOW);
        delay(500);
        
        //quit connecting after 20 tries
        if(tries > 20)
        {   
            openAP=true; // set open access point true 
            break;
        }
        tries++;
    }

    // if connection to wifi is successfull
    if(!openAP) 
    {
        digitalWrite(INBUILD_LED, LOW); // make led glow

        Serial.println();
        Serial.print("CONNECTED [ ");
        Serial.print(WiFi.localIP());
        Serial.print(" ]");

        // setting API and URL in config
        config.api_key = API_KEY;
        config.database_url = DATABASE_URL;
        // auth.token.uid = "h7zVGQdSrVaAle0RIpDAO3Ho84w2";
        // auth.user.email = "test123@gmail.com";
        // auth.user.password = "test123";

        if (Firebase.signUp(&config, &auth, "", ""))
        {
            Serial.println("Firebase [Connected] : SignUp : ok");
            signupOK = true;
        }
        else
        {
            Serial.printf("%s\n", config.signer.signupError.message.c_str());
        }

        config.token_status_callback = tokenStatusCallback;

        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
    }

    // turning on AP if unable to connect to WiFi
    if(openAP)
    {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("Resource Monitor", "12345678");
        Serial.println("Unable to connect to WiFi");
        Serial.println(WiFi.softAPIP());
        digitalWrite(INBUILD_LED, HIGH);
    }
    
    // launching a Server on connected local network
    server.on("/",handlePortal);
    server.serveStatic("/",LittleFS,"/");
    server.begin();
    

    {
        pinMode(WATER_SENSOR_PIN, INPUT);

        data.add("waterValue", watervalue);
    }

}

void loop()
{
    if (Firebase.ready() && signupOK ){
    // Write an Int number on the database path test/int
    if (Firebase.RTDB.setJSON(&fbdo, "test/data", &data)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    }

    watervalue = analogRead(WATER_SENSOR_PIN);
    data.set("waterValue", watervalue);
    
}




void handlePortal()
{
    if(server.method() == HTTP_POST)
    {
        strncpy(userWifiInfo.ssid, server.arg("ssid").c_str(), sizeof(userWifiInfo.ssid));
        strncpy(userWifiInfo.password, server.arg("password").c_str(), sizeof(userWifiInfo.password));
        userWifiInfo.password[server.arg("password").length()] = '\0';

        writeWiFiInfo(userWifiInfo);

        server.send(200, "text/html", "<html><body>YOUR setting is setup successfully</body></html>");
    }

    if(server.method() == HTTP_GET)
    {
        
        File HTMLFile = readHTMLFile();
        server.send(200, "text/html", HTMLFile.readString());
    }
}


WiFiInfo readWiFiInfo()
{
    WiFiInfo userWifiInfo;
    File configFile = LittleFS.open("/config.txt","r");
    if(!configFile)
    {
        Serial.println("[reading] : unable to open file");
    }
    else
    {
        Serial.println("[reading] : file available");
        configFile.read((uint8_t*)&userWifiInfo, sizeof(WiFiInfo));
        configFile.close();
    }
    return userWifiInfo;
}



void writeWiFiInfo(const WiFiInfo& userWifiInfo)
{
    File configFile = LittleFS.open("/config.txt","w");
    if(!configFile)
    {
        Serial.println("[writing] : unable to open file");
    }
    else
    {
        Serial.println("[writing] : file available");
        configFile.write((uint8_t*)&userWifiInfo, sizeof(WiFiInfo));
        configFile.close();
    }
}


File readHTMLFile()
{
    File HTMLFile = LittleFS.open("/index.html","r");
    if(!HTMLFile)
    {
        Serial.println("[reading] : unable to open file");
    }
    else
    {
        Serial.println("[reading] : html file available");
    }
    return HTMLFile;
}