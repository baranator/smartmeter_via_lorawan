/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * Copyright (c) 2018 Terry Moore, MCCI
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!

 * To use this sketch, first register your application and device with
 * the things network, to set or generate an AppEUI, DevEUI and AppKey.
 * Multiple devices can use the same AppEUI, but each device has its own
 * DevEUI and AppKey.
 *
 * Do not forget to define the radio type correctly in
 * arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.
 *
 *******************************************************************************/
#include <Arduino.h>
#include <list>
#include <arduino_lmic_hal_boards.h>
#include <esp_sleep.h>
#define SERIAL_DEBUG true
#include <SerialDebug.h>

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <sml/sml_file.h>

#include <IotWebConf.h>
#include <IotWebConfOptionalGroup.h>
#include <IotWebConfUsing.h>

#include <regex.h>
#include <stdio.h>

#include "arduino_lmic_hal_configuration.h"
#include "config.h"
#include "SensorGroup.h"


//General

boolean firstRun=true;
Sensor * smlSensor;


// IOT Webconf

DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, "7");
iotwebconf::OptionalGroupHtmlFormatProvider optionalGroupHtmlFormatProvider;

SensorGroup sensorGroup("sensorGroup");
char loraDeveuiValue[STRING_LEN];
char loraAppeuiValue[STRING_LEN];
char loraAppkeyValue[STRING_LEN];
char loraIntervalValue[STRING_LEN];

IotWebConfParameterGroup lorawanGroup = IotWebConfParameterGroup("lorawang", "LoRaWan");
iotwebconf::TextParameter loraDeveuiParam = iotwebconf::TextParameter("Dev-EUI (8 Byte, little-endian)", "ldeveui", loraDeveuiValue, STRING_LEN);
iotwebconf::TextParameter loraAppeuiParam = iotwebconf::TextParameter("App-EUI (8 Byte, little-endian)", "lappeui", loraAppeuiValue, STRING_LEN);
iotwebconf::TextParameter loraAppkeyParam = iotwebconf::TextParameter("App-Key (16 Byte big-endian", "lappkey", loraAppkeyValue, STRING_LEN);
iotwebconf::TextParameter loraIntervalParam = iotwebconf::TextParameter("Sendeinterval in min", "linterval", loraIntervalValue, STRING_LEN,"60");


//LoRaWan Stuff

static uint8_t* loraSendBuffer;
static osjob_t sendjob;

const Arduino_LMIC::HalConfiguration_t myConfig;
const lmic_pinmap *pPinMap = Arduino_LMIC::GetPinmap_ThisBoard();



//static const u1_t PROGMEM APPEUI[8]={ 0x77, 0x6D, 0x54, 0xDF, 0x56, 0x54, 0x54, 0x56 };
//void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getArtEui (u1_t* buf) { 
  u1_t APPEUI[8];
  formStringToByteArray(String(loraAppeuiValue), APPEUI, 8);
  memcpy_P(buf, APPEUI, 8);
}


// This should also be in little endian format, see above.
//static const u1_t PROGMEM DEVEUI[8]={ 0x34, 0x2F, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
//void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevEui (u1_t* buf) { 
  u1_t DEVEUI[8];
  formStringToByteArray(String(loraDeveuiValue), DEVEUI, 8);
  memcpy_P(buf, DEVEUI, 8);
}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
//static const u1_t PROGMEM APPKEY[16] = { 0x85, 0x38, 0x35, 0x03, 0x15, 0x10, 0x11, 0x3E, 0x64, 0x12, 0xD9, 0xDD, 0xA9, 0xA1, 0x48, 0xFF };
//void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

void os_getDevKey (u1_t* buf) {  
  u1_t APPKEY[16];
  formStringToByteArray(String(loraAppkeyValue), APPKEY, 16);
  memcpy_P(buf, APPKEY, 16);
}

boolean sending=false;




bool formValidator(iotwebconf::WebRequestWrapper* webRequestWrapper){
  bool retval=true;

  
  //lora
  String ldeveui= webRequestWrapper->arg(loraDeveuiParam.getId());
  if(!isHexString(ldeveui,8)){
      loraDeveuiParam.errorMessage = "Bitte als Hex-String xx xx xx... angeben";
      retval=false;
  }
  String lappeui= webRequestWrapper->arg(loraAppeuiParam.getId());
  if(!isHexString(lappeui,8)){
      loraAppeuiParam.errorMessage = "Bitte als Hex-String xx xx xx... angeben";
      retval=false;
  }
  String lappkey= webRequestWrapper->arg(loraAppkeyParam.getId());
  if(!isHexString(lappkey,16)){
      loraAppkeyParam.errorMessage = "Bitte als Hex-String xx xx xx... angeben";
      retval=false;
  }
  //Sending interval
  uint16_t interval;
  if(sscanf (loraIntervalValue,"%u",&interval)!=1 || interval <5 ){
    loraIntervalParam.errorMessage= "Sendeintervall in Minuten (min. 5) angeben";
    retval=false;
  }

  //sensor
  //Bezeichner
  String identifier= webRequestWrapper->arg(sensorGroup.identifierParam.getId()); 
  if(identifier.length()==0){
      sensorGroup.identifierParam.errorMessage = "Bezeichner darf nicht leer sein!";
      retval=false;
  }

    
  //OBIS
  String stype= webRequestWrapper->arg(sensorGroup.stypeParam.getId());
  String opt1= webRequestWrapper->arg(sensorGroup.opt1Param.getId());
    
  if(stype.compareTo("sml")==0){
      if( !formObisToList(opt1, NULL)){
        sensorGroup.opt1Param.errorMessage = "Zu lesende OBIS-Ids durch Leerzeichen getrennt eingeben!";
        retval=false;  
      }
  }


  return retval;
}


void handleRoot(){
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 16 OffLine</title></head><body>";
  s += "Go to <a href='config'>configure page</a> to change settings.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

void goOff(){
  iotWebConf.goOffLine();
  Serial.println(F("AP-Modus beendet, Gerät neustarten um zu konfigurieren"));
}

void iotwSetup(){
  //general
  iotWebConf.setHtmlFormatProvider(&optionalGroupHtmlFormatProvider);
  iotWebConf.setStatusPin(STATUS_PIN, HIGH);
  iotWebConf.setConfigPin(CONFIG_PIN);  

  //callbacks
  iotWebConf.setConfigSavedCallback(goOff);
  iotWebConf.setFormValidator(&formValidator);

  //system
  iotWebConf.getWifiSsidParameter()->visible=false;
  iotWebConf.getWifiPasswordParameter()->visible=false;

  //lora
  lorawanGroup.addItem(&loraDeveuiParam);
  lorawanGroup.addItem(&loraAppeuiParam);
  lorawanGroup.addItem(&loraAppkeyParam);
  lorawanGroup.addItem(&loraIntervalParam);
  iotWebConf.addParameterGroup(&lorawanGroup);
  
  //sensors
  iotWebConf.addParameterGroup(&sensorGroup);

  
  
  // -- Initializing the configuration
  iotWebConf.init();
  
  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  DEBUG(F("iotwebconf inited"));
}



//start is the number of the byte where the uint32 val starts, beginning at zero
void fillbufferwithu32(uint32_t v, uint8_t start){
    start+=1; //reserve one byte at the beginning for indication of given values as a mask
    loraSendBuffer[start+0] = v & 0xFF; // 0x78
    loraSendBuffer[start+1] = (v >> 8) & 0xFF; // 0x56
    loraSendBuffer[start+2] = (v >> 16) & 0xFF; // 0x34
    loraSendBuffer[start+3] = (v >> 24) & 0xFF; // 0x12
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        DEBUG(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, loraSendBuffer, sizeof(loraSendBuffer), 0);
        DEBUG(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void publish(Sensor *sensor, sml_file *file){
    if(sending){
        return;
    }
        
    for (int i = 0; i < file->messages_len; i++){
        sml_message *message = file->messages[i];
        if (*message->message_body->tag == SML_MESSAGE_GET_LIST_RESPONSE){
            sml_list *entry;
            sml_get_list_response *body;
            body = (sml_get_list_response *)message->message_body->data;
            for (entry = body->val_list; entry != NULL; entry = entry->next){
                if (!entry->value){ // do not crash on null value
                    continue;
                }

                char obisIdentifier[32];
                uint32_t intval=0;
                sprintf(obisIdentifier, "%d-%d:%d.%d.%d/%d",
                    entry->obj_name->str[0], entry->obj_name->str[1],
                    entry->obj_name->str[2], entry->obj_name->str[3],
                    entry->obj_name->str[4], entry->obj_name->str[5]);

               // String obisIds[]={"1-0:1.8.0","1-0:2.8.0"};

                std::list<String> obisIds;
                formObisToList(sensorGroup.opt1Value, &obisIds);
                loraSendBuffer=(uint8_t*)malloc(obisIds.size()+1);

                int o=0;
                	for (std::list<String>::iterator it = obisIds.begin(); it != obisIds.end(); ++it, o++){

                    if(String(obisIdentifier).startsWith( (*it))){
                        DEBUG("OBIS-Id: %s",obisIdentifier);

                        if (((entry->value->type & SML_TYPE_FIELD) == SML_TYPE_INTEGER) ||
                        ((entry->value->type & SML_TYPE_FIELD) == SML_TYPE_UNSIGNED)){
                            double value = sml_value_to_double(entry->value);
                            int scaler = (entry->scaler) ? *entry->scaler : 0;
                            int prec = -scaler;
                            if (prec < 0)
                                prec = 0;
                            value = value * pow(10, scaler);
            
                            //ignore sign and store with as int with factor 10
                            // this allows vals from 0 to 429496729,5 to be stored
                            intval=abs(round(value*10));
                            fillbufferwithu32(intval,o);
                            loraSendBuffer[0]= loraSendBuffer[0] | (uint8_t)(1 << o);
                            

                        }else if (false && !sensor->config->numeric_only){    //ignore for now
                            if (entry->value->type == SML_TYPE_OCTET_STRING){
                                char *value;
                                sml_value_to_strhex(entry->value, &value, true);
                                //publish(entryTopic + "value", value);
                                free(value);
                            }else if (entry->value->type == SML_TYPE_BOOLEAN){
                                //publish(entryTopic + "value", entry->value->data.boolean ? "true" : "false");
                            }
                        }

                    
                    }
                }
              do_send(&sendjob);

            }
        }
    }  
}

void process_message(byte *buffer, size_t len, Sensor *sensor){
	// Parse
	sml_file *file = sml_file_parse(buffer + 8, len - 16);



	publish(sensor, file);

	// free the malloc'd memory
	sml_file_free(file);
}




void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            DEBUG(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            DEBUG(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            DEBUG(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            DEBUG(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            DEBUG(F("EV_JOINING"));
            break;
        case EV_JOINED:
            DEBUG(F("EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              DEBUG(netid, DEC);
              Serial.print("devaddr: ");
              DEBUG(devaddr, HEX);
              Serial.print("AppSKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                printHex2(artKey[i]);
              }
              DEBUG("");
              Serial.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              Serial.print("-");
                      printHex2(nwkKey[i]);
              }
              DEBUG("");
            }
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
      // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     DEBUG(F("EV_RFU1"));
        ||     break;
        */
        case EV_JOIN_FAILED:
            DEBUG(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            DEBUG(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            DEBUG(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              DEBUG(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              DEBUG(F(" bytes of payload"));
            }
            // Schedule next transmission

              
            // Konfiguriere den Wake-up-Trigger. Hier verwenden wir die Timer-Funktion.
            esp_sleep_enable_timer_wakeup(atoi(loraIntervalValue)*60*1000*1000); // 10.000.000 Mikrosekunden = 10 Sekunden

            // Gehe in den Deep-Sleep-Modus.
            esp_deep_sleep_start();

            break;
        case EV_LOST_TSYNC:
            DEBUG(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            DEBUG(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            DEBUG(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            DEBUG(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            DEBUG(F("EV_LINK_ALIVE"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    DEBUG(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            DEBUG(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            DEBUG(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            DEBUG(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;

        default:
            Serial.print(F("Unknown event: "));
            DEBUG((unsigned) ev);
            break;
    }
}

void pseudoSetup(){
  //called once when device is in normal offline operation  
      

    #ifdef DEBUG
	// Delay for getting a serial console attached in time
	    delay(2000);
    #endif

	// Setup reading heads
	//DEBUG("Setting up max. %d configured sensors...", MAX_NUM_OF_SENSORS);
	//const SensorConfig *config = SENSOR_CONFIGS;


  static const SensorConfig* config = new SensorConfig{
    .pin = (uint8_t)atoi(sensorGroup.dataPinValue),
    .numeric_only = false,
    .interval = 99 //only once
  };

  
	smlSensor = new Sensor(config, process_message);

  DEBUG(F("Sensor setup done.Starting"));
//    DEBUG(RST_LoRa);

    // LMIC init
  os_init_ex(pPinMap);
   // os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
    // Start job (sending automatically starts OTAA too)
    //do_send(&sendjob);
}


void setup() {
  SERIAL_DEBUG_SETUP(9600);

  DEBUG(F("Starting up..."));

  iotwSetup();

    
}

void pseudoLoop(){
    smlSensor->loop();
    os_runloop_once();
}

void loop() {


      // -- doLoop should be called as frequently as possible.
  iotWebConf.doLoop();

  if (iotWebConf.getState() == iotwebconf::NotConfigured){
    unsigned long now = millis();
    if (OFF_LINE_AFTER_MS < (now - iotWebConf.getApStartTimeMs())){

    }
  }
  else if (iotWebConf.getState() == iotwebconf::OffLine){
    if(firstRun){
      firstRun=false;
      pseudoSetup();
      DEBUG(F("Gerät gestartet; Pseudo-Setup"));
    }
    DEBUG(F("loop"));
    pseudoLoop();
    
  }


}
