#ifndef SENSORGROUP_H
#define SENSORGROUP_H


#include <IotWebConf.h>
#include <IotWebConfOptionalGroup.h>
#include <IotWebConfUsing.h>
#include "config.h"

class SensorGroup : public iotwebconf::ParameterGroup{
  public:
    char dataPinValue[NUMBER_LEN];
  //  char identifierValue[STRING_LEN];
    char stypeValue[STRING_LEN];
    char opt1Value[LONG_STRING_LEN];
    char timeoutValue[NUMBER_LEN];
    
    //iotwebconf::TextParameter identifierParam = iotwebconf::TextParameter("Bezeichner", identifierId, identifierValue, STRING_LEN);
    IotWebConfSelectParameter stypeParam = IotWebConfSelectParameter("Sensor-Typ", stypeId, stypeValue, STRING_LEN, (char*)stypeVals, (char*)stypeVals, sizeof(stypeVals) / STRING_LEN, STRING_LEN);
    iotwebconf::TextParameter dataPinParam = iotwebconf::TextParameter("Data Pin", dataPinId, dataPinValue, NUMBER_LEN);
    iotwebconf::TextParameter opt1Param = iotwebconf::TextParameter("OBIS-Ids/ppUnit", opt1Id, opt1Value, LONG_STRING_LEN, "1-0:1.8.0 1-0:2.8.0");
    iotwebconf::TextParameter timeoutParam = iotwebconf::TextParameter("Timeout", timeoutId, timeoutValue, NUMBER_LEN,"30");


    SensorGroup(const char* id) : iotwebconf::ParameterGroup(id, "Sensor"){
      // -- Update parameter Ids to have unique ID for all parameters within the application.
      snprintf(dataPinId, STRING_LEN, "%s-datapin", this->getId());
  //    snprintf(identifierId, STRING_LEN, "%s-identifier", this->getId());
      snprintf(stypeId, STRING_LEN, "%s-stype", this->getId());
      snprintf(opt1Id, STRING_LEN, "%s-opt1", this->getId());
      snprintf(timeoutId, STRING_LEN, "%s-timeout", this->getId());

      // -- Add parameters to this group.
      this->addItem(&this->stypeParam);
    //  this->addItem(&this->identifierParam);
      this->addItem(&this->dataPinParam);
      this->addItem(&this->timeoutParam);
      this->addItem(&this->opt1Param);
      
    }
    
  private:
    char dataPinId[STRING_LEN];
//    char identifierId[STRING_LEN];
    char stypeId[STRING_LEN];
    char opt1Id[LONG_STRING_LEN];
    char timeoutId[STRING_LEN];
};

#endif