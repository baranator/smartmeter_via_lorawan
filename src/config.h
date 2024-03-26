
#ifndef CONFIG_H
#define CONFIG_H

#include "Arduino.h"
#include "Sensor.h"






// Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "MeterViaLoRa";

// Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "ghghgh12";

// When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial password to build an AP. (E.g. in case of lost password)
#define CONFIG_PIN 34
#define STATUS_PIN 35

// Go to off-line mode after this time was passed and inital configuration is not done.
#define OFF_LINE_AFTER_MS 10*60*10000

#define STRING_LEN 128
#define NUMBER_LEN 32

static char stypeVals[][STRING_LEN] = { "sml", "s0", "other" };






static const u1_t PROGMEM APPEUI[8]={ 0x77, 0x6D, 0x54, 0xDF, 0x56, 0x54, 0x54, 0x56 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]={ 0x34, 0x2F, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = { 0x85, 0x38, 0x35, 0x03, 0x15, 0x10, 0x11, 0x3E, 0x64, 0x12, 0xD9, 0xDD, 0xA9, 0xA1, 0x48, 0xFF };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

//welche OBIS-Werte sollen übermittelt werden? Standardmäßig Bezugs- und Einspeisezählerstand
//const String obisIds[]={"1-0:1.8.0","1-0:2.8.0"};

bool formStringToByteArray(String fs, uint8_t* bytearray, uint8_t num_bytes){
    fs.replace(" ","");
    uint8_t s_len = fs.length();
    if(s_len != num_bytes*2){
        return false;
    }

    for (int i = 0; i < (s_len / 2); i++) {
        sscanf(fs.c_str() + 2*i, "%02x", &bytearray[i]);
        printf("bytearray %d: %02x\n", i, bytearray[i]);
    }
    return true;
}

bool formObisToArray(String fs, String* stringarray){
    bool retval=true;
    uint8_t MAX_OBIS_FIELDS=8;
    regex_t reegex;
    char buf[STRING_LEN];
    fs.toCharArray(buf, STRING_LEN);
    //regex for multiple, space separated obis identifier
    int v=regcomp( &reegex, "^([0-9]\\-[0-9]{1,2}:)?[0-9]\\.[0-9]\\.[0-9]( ([0-9]\\-[0-9]{1,2}:)?[0-9]\\.[0-9]\\.[0-9]){0,3}$", REG_EXTENDED | REG_NOSUB);
    //Serial.println(buf);
    regmatch_t pmatch[MAX_OBIS_FIELDS+1];   
    //Serial.println(regexec(&reegex, buf, 0, NULL, 0));
    if(regexec(&reegex, buf, MAX_OBIS_FIELDS+1, pmatch, 0)!=0) {
        retval=false;  
    }else{
        int a=0;
        for(int i=0;i<fs.length();i++){
            char c=fs.charAt(i);
            if(c!=' '){
                stringarray[a].concat(c);
            }else{
                a++;
                stringarray[a]="";
            }
        }
        
    }
    return retval;
}



//const uint8_t NUM_OF_SENSORS = sizeof(SENSOR_CONFIGS) / sizeof(SensorConfig);

#endif