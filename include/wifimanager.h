#include <ESP8266WebServer.h>


struct WiFiInfo
{
    char ssid[30];
    char password[30];
};

void InitWiFiInfo(struct WiFiInfo WiFiInfo);

int connectToWiFi(struct WiFiInfo WiFiInfo, int timeout);

void createAP(const char name[30], const char pass[30]);

// void handlePortal(ESP8266WebServer &server, struct WiFiInfo WiFiInfo);

void resetEEPROM(struct WiFiInfo WiFiInfo);
