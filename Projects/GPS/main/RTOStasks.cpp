#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "Arduino.h"
#include "HardwareSerial.h"
#include "SoftwareSerial.h"
#include "TinyGPS++.h"
#include "Source.h"
#include <string>

extern "C" void vGPSTask(void *pvParameters)
{
    ParamsStruct *Params = (ParamsStruct *)pvParameters;
    TinyGPSPlus gps;
    SoftwareSerial ss(RXPIN, TXPIN);
    Serial.begin(115200);
    ss.begin(9600);

    while(1)
    {
        //printf("%d\n", ss.read());
        while(ss.available() > 0)
        {
            gps.encode(ss.read());
            printf("\nLatitude:\n    ");
            Serial.print(gps.location.lat(), 6);
            double lat = gps.location.lat();
            memcpy(Params->latitude, std::to_string(lat).c_str(), sizeof(lat)); 
//            memcpy(Params->latitude, &lat, sizeof(lat)); // This triggers Guru Meditation Error (StoreProhibited)

            printf("\nLongitude:\n    ");
            Serial.print(gps.location.lng(), 6);
            double lng = gps.location.lng();
            memcpy(Params->latitude, std::to_string(lng).c_str(), sizeof(lng));
        }
        if (millis() > 5000 && gps.charsProcessed() < 10)
        {
            Serial.println(F("No GPS detected: check wiring."));
            while(true);
        }
        vTaskDelay(5);
    }
}

