#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include <SD.h>
#include "wifimanager.h"



struct WiFiInfo
{
    char ssid[30];
    char password[30];
};


void InitWiFiInfo(struct WiFiInfo &WiFiInfo)
{
    EEPROM.begin(sizeof(struct WiFiInfo));
    EEPROM.get(0, WiFiInfo);
}


int connectToWiFi(struct WiFiInfo WiFiInfo, int timeout)
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WiFiInfo.ssid, WiFiInfo.password);

    byte tries = 0;
    while(WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        if(tries > timeout)
        {
            return 0;
        }
        tries++;
    }
    return 1;
}

void createAP(const char name[30], const char pass[30])
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(name, pass);
}

