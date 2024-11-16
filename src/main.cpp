#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include <SD.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <FirebaseJson.h>


// structure for storing wifi information
struct WiFiInfo
{
    char ssid[30];
    char password[30];

} userWifiInfo;




void handlePortal(); // handle for local webpages
WiFiInfo readWiFiInfo();
bool writeWiFiInfo(const WiFiInfo&);
File readHTMLFile();


// FireBase URL and API 
#define DATABASE_URL ""
#define API_KEY ""

// PINS
#define INBUILD_LED D4
#define WATER_SENSOR_PIN A0


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
IPAddress local_IP(192,168,1,2); // custom local IP Address
IPAddress gateway(192, 168, 1, 1); // custom gateway Address
IPAddress subnet(255, 255, 0, 0); // custom subnet Address
IPAddress primaryDNS(8, 8, 8, 8); // custom primaryDNS Address
IPAddress secondaryDNS(8, 8, 4, 4); // custom secondaryDNS Address


void setup()
{   
    

    Serial.begin(115200); // Serial Monitor at 115200
    pinMode(INBUILD_LED, OUTPUT);

    if(!LittleFS.begin())
    {
        Serial.println("unable to mount LittleFS");
    }


    userWifiInfo = readWiFiInfo();

    Serial.println(userWifiInfo.ssid);
    Serial.println(userWifiInfo.password);
    
    WiFi.mode(WIFI_STA); // setting WiFi mode to station
    WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
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
        Serial.println(WiFi.mode(WIFI_AP));
        WiFi.softAPConfig(local_IP, gateway, subnet);
        Serial.println(WiFi.softAP("Resource Monitor", "12345678"));
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
    server.handleClient();
    if (Firebase.ready() && signupOK ){
    // Write an Int number on the database path test/int
    // if (Firebase.RTDB.setJSON(&fbdo, "test/data", &data)){
    //   Serial.println("PASSED");
    //   Serial.println("PATH: " + fbdo.dataPath());
    //   Serial.println("TYPE: " + fbdo.dataType());
    // }
    // else {
    //   Serial.println("FAILED");
    //   Serial.println("REASON: " + fbdo.errorReason());
    // }
    Serial.print(".");
    }

    // watervalue = analogRead(WATER_SENSOR_PIN);
    // data.set("waterValue", watervalue);
    
}




void handlePortal()
{
    if(server.method() == HTTP_POST)
    {
        strncpy(userWifiInfo.ssid, server.arg("ssid").c_str(), sizeof(userWifiInfo.ssid));
        strncpy(userWifiInfo.password, server.arg("password").c_str(), sizeof(userWifiInfo.password));
        userWifiInfo.password[server.arg("password").length()] = '\0';

        if(writeWiFiInfo(userWifiInfo))
        {
            server.send(200, "text/html", "<html><body>YOUR setting is setup successfully. restarting esp...</body></html>");
            delay(1000);
            ESP.restart();
        }
        else
        {
            server.send(500, "text/html", "<html><body>UNABLE to setup your WiFi. try again.</body></html>");
        }

        
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



bool writeWiFiInfo(const WiFiInfo& userWifiInfo)
{
    File configFile = LittleFS.open("/config.txt","w");
    if(!configFile)
    {
        Serial.println("[writing] : unable to open file");
        return false;
    }
    else
    {
        Serial.println("[writing] : file available");
        configFile.write((uint8_t*)&userWifiInfo, sizeof(WiFiInfo));
        configFile.close();
        return true;
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
