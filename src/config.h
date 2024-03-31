#ifndef CONFIG_H
#define CONFIG_H

#include "Arduino.h"
#include "Sensor.h"



// Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "MeterViaLoRa";

// Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "MeterViaLoRa";

// When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial password to build an AP. (E.g. in case of lost password)
//#define CONFIG_PIN 34
#define STATUS_PIN 35

// Go to off-line mode after this time was passed and inital configuration is not done.
#define OFF_LINE_AFTER_MS 1*30*1000

#define LONG_STRING_LEN 128
#define STRING_LEN 64
#define NUMBER_LEN 32

static char stypeVals[][STRING_LEN] = { "sml", "s0", "other" };

boolean isHexString(String s, uint8_t num_bytes){
  regex_t reegex;
  char regstr[50];
  s.toLowerCase();
  sprintf(regstr, "^[0-9a-f]{2}( [0-9a-f]{2}){%d}$", num_bytes-1);
  char buf[STRING_LEN];
  s.toCharArray(buf, STRING_LEN);
  int v=regcomp( &reegex, regstr, REG_EXTENDED | REG_NOSUB);
  if(regexec(&reegex, buf, 0, NULL, 0)!=0) {
    DEBUG("notso hexy!");
    return false;
  }
  DEBUG("voll hexig!");
  return true;
}

bool formStringToByteArray(char* fs, uint8_t* bytearray, uint8_t num_bytes){
    uint16_t s_len = strlen(fs);
    for(int i=0;i<s_len;i++){
        fs[i]=tolower(fs[i]);
    }

  //  DEBUG(fs);
    
    if(s_len != 3*num_bytes-1){
        return false;
    }

    for (int i = 0; i < num_bytes; i++) {
        sscanf( fs+ 3*i, "%2hhx", bytearray+i);
       //printf("bytearray %d: %02x\n", i, bytearray[i]);
    }
    return true;
}

bool formObisToList(String fs, std::list<String> *slist){
    bool retval=true;
    regex_t reegex;
    char buf[STRING_LEN];
    fs.toCharArray(buf, STRING_LEN);
    //regex for multiple, space separated obis identifier
    int v=regcomp( &reegex, "^([0-9]\\-[0-9]{1,2}:)?[0-9]\\.[0-9]\\.[0-9]( ([0-9]\\-[0-9]{1,2}:)?[0-9]\\.[0-9]\\.[0-9]){0,3}$", REG_EXTENDED | REG_NOSUB);
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

bool isNumeric(String fs){
    regex_t reegex;
    char buf[STRING_LEN];
    fs.toCharArray(buf, STRING_LEN);
    //regex for multiple, space separated obis identifier
    int v=regcomp( &reegex, "^[0-9]{1,4}$", REG_EXTENDED | REG_NOSUB);
    if(regexec(&reegex, buf, 0, NULL, 0)!=0) {
        return false;  
    } 
    return true;
}


#endif