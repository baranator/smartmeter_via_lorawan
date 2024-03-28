
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







//welche OBIS-Werte sollen übermittelt werden? Standardmäßig Bezugs- und Einspeisezählerstand
//const String obisIds[]={"1-0:1.8.0","1-0:2.8.0"};

boolean isHexString(String s, uint8_t num_bytes){
  regex_t reegex;
  char regstr[50];
  sprintf(regstr, "^[0-9a-f]{2}( [0-9a-f]{2}){%d}$", num_bytes-1);
  char buf[STRING_LEN];
  s.toCharArray(buf, STRING_LEN);
  int v=regcomp( &reegex, regstr, REG_EXTENDED | REG_NOSUB);
  Serial.print("kompil: ");Serial.println(v);
  if(regexec(&reegex, buf, 0, NULL, 0)!=0) {
    Serial.println("notso hexy!");
    return false;
  }
  Serial.println("voll hexig!");
  return true;
}

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

bool formObisToList(String fs, std::list<String> *slist){
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
    if(regexec(&reegex, buf, 0, NULL, 0)!=0) {
        retval=false;  
    }else{
        if(slist != NULL){
            int a=0;
            String so="";
            for(int i=0;i < strlen(buf);i++){
                char c=buf[i];
                if(c != ' ' && c != '\0'){
                    so.concat(c);
                }else{
                    slist->push_back(so);
                }
            }
        }
    }
    return retval;
}



//const uint8_t NUM_OF_SENSORS = sizeof(SENSOR_CONFIGS) / sizeof(SensorConfig);

#endif