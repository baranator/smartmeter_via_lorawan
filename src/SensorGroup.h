#ifndef SENSORGROUP_H
#define SENSORGROUP_H


#include <IotWebConf.h>
#include <IotWebConfOptionalGroup.h>
#include <IotWebConfUsing.h>
#include "config.h"

class SensorGroup : public iotwebconf::OptionalParameterGroup{
  public:
    char dataPinValue[STRING_LEN];
    char identifierValue[STRING_LEN];
    char stypeValue[STRING_LEN];
    char opt1Value[STRING_LEN];
    
    iotwebconf::TextParameter identifierParam = iotwebconf::TextParameter("Bezeichner", identifierId, identifierValue, STRING_LEN);
    IotWebConfSelectParameter stypeParam = IotWebConfSelectParameter("Sensor-Typ", stypeId, stypeValue, STRING_LEN, (char*)stypeVals, (char*)stypeVals, sizeof(stypeVals) / STRING_LEN, STRING_LEN);
    iotwebconf::TextParameter dataPinParam = iotwebconf::TextParameter("Data Pin", dataPinId, dataPinValue, STRING_LEN);
    iotwebconf::TextParameter opt1Param = iotwebconf::TextParameter("OBIS-Ids/ppUnit", opt1Id, opt1Value, STRING_LEN);
    
    SensorGroup(const char* id) : OptionalParameterGroup(id, "Sensor",false){
      // -- Update parameter Ids to have unique ID for all parameters within the application.
      snprintf(dataPinId, STRING_LEN, "%s-datapin", this->getId());
      snprintf(identifierId, STRING_LEN, "%s-identifier", this->getId());
      snprintf(stypeId, STRING_LEN, "%s-stype", this->getId());
      snprintf(opt1Id, STRING_LEN, "%s-op1", this->getId());

      // -- Add parameters to this group.
      this->addItem(&this->stypeParam);
      this->addItem(&this->identifierParam);
      this->addItem(&this->dataPinParam);
      this->addItem(&this->opt1Param);
      
    }
    
  private:
    char dataPinId[STRING_LEN];
    char identifierId[STRING_LEN];
    char stypeId[STRING_LEN];
    char opt1Id[STRING_LEN];
};

#endif