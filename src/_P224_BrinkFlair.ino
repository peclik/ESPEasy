#ifdef USES_P224

// #######################################################################################################
// ############################# Plugin 224: HVAC - Brink Flair ##########################################
// #######################################################################################################

/*

   Circuit wiring
    GPIO Setting 1 -> RX
    GPIO Setting 2 -> TX
 */

#define PLUGIN_224
#define PLUGIN_ID_224 224
#define PLUGIN_NAME_224 "HVAC - Brink Flair (json)"

#define PLUGIN_225
#define PLUGIN_ID_225 225
#define PLUGIN_NAME_225 "HVAC - Brink Flair (quad val.)"


#define P224_DEV_ID         PCONFIG(0)
#define P224_DEV_ID_LABEL   PCONFIG_LABEL(0)
#define P224_MODEL          PCONFIG(1)
#define P224_MODEL_LABEL    PCONFIG_LABEL(1)
#define P224_BAUDRATE       PCONFIG(2)
#define P224_BAUDRATE_LABEL PCONFIG_LABEL(2)
#define P224_QUERY1         PCONFIG(3)
#define P224_QUERY2         PCONFIG(4)
#define P224_QUERY3         PCONFIG(5)
#define P224_QUERY4         PCONFIG(6)
#define P224_DEPIN          -1

#define P224_NR_OUTPUT_VALUES   1
#define P225_NR_OUTPUT_VALUES   VARS_PER_TASK

#define P224_QUERY1_CONFIG_POS  3

#define P224_QUERY_CUR_SUP_PRES  0
#define P224_QUERY_CUR_EXH_PRES  1
#define P224_QUERY_SET_SUP_FLOW  2
#define P224_QUERY_CUR_SUP_FLOW  3
#define P224_QUERY_CUR_SUP_RPM   4
#define P224_QUERY_CUR_SUP_TEMP  5
#define P224_QUERY_CUR_SUP_RH    6
#define P224_QUERY_SET_EXH_FLOW  7
#define P224_QUERY_CUR_EXH_FLOW  8
#define P224_QUERY_CUR_EXH_RPM   9
#define P224_QUERY_CUR_EXH_TEMP 10
#define P224_QUERY_CUR_EXH_RH   11
#define P224_QUERY_CUR_BYPASS   12
#define P224_QUERY_STS_PREHEAT  13
#define P224_QUERY_CUR_PREHEAT  14
#define P224_QUERY_STS_FILTER   15
#define P224_NR_OUTPUT_OPTIONS  16 // Must be the last one

#define P224_QUERY_JSON          (P224_NR_OUTPUT_OPTIONS + 1)

#define P224_DEV_ID_DFLT   20      // Modbus communication address
#define P224_MODEL_DFLT    0       //
#define P224_BAUDRATE_DFLT 4       // 19200 baud

#define P224_QUERY1_DFLT   P224_QUERY_JSON

#define P225_QUERY1_DFLT   P224_QUERY_CUR_SUP_FLOW
#define P225_QUERY2_DFLT   P224_QUERY_CUR_EXH_FLOW
#define P225_QUERY3_DFLT   P224_QUERY_CUR_BYPASS
#define P225_QUERY4_DFLT   P224_QUERY_CUR_PREHEAT

#define P224_MODBUS_TIMEOUT 30

#include <ArduinoJson.h>
#include <ESPeasySerial.h>

struct P224_data_struct : public PluginTaskData_base {
  P224_data_struct() {}

  ~P224_data_struct() {
    reset();
  }

  void reset() {
    modbus.reset();
    readIdx = 0;
  }

  bool init(const int16_t rxPin, const int16_t txPin, int8_t dere_pin,
            unsigned int baudrate, uint8_t modbusAddress) {
    bool res = modbus.init(rxPin, txPin, baudrate, modbusAddress, dere_pin);
    modbus.setModbusTimeout(P224_MODBUS_TIMEOUT);
    return res;
  }

  bool isInitialized() const {
    return modbus.isInitialized();
  }

  struct DataBuf {
    DataBuf()
      : success(false), value(0)
    {}
    bool  success;
    float value;
  };

  ModbusRTU_struct modbus;

  // index for continuous data reading
  int16_t  readIdx;
  DataBuf  dataBuf[P224_NR_OUTPUT_OPTIONS];
};


void Plugin_224_add2json(int16_t valueIdx, ArduinoJson::JsonDocument& json, float value) {
  const String& key = Plugin_224_valuename(valueIdx, false);
  String strVal;

  switch (valueIdx) {
    case P224_QUERY_CUR_SUP_PRES:
    case P224_QUERY_CUR_EXH_PRES:
    case P224_QUERY_SET_SUP_FLOW:
    case P224_QUERY_CUR_SUP_FLOW:
    case P224_QUERY_CUR_SUP_RPM:
    case P224_QUERY_CUR_SUP_RH:
    case P224_QUERY_SET_EXH_FLOW:
    case P224_QUERY_CUR_EXH_FLOW:
    case P224_QUERY_CUR_EXH_RPM:
    case P224_QUERY_CUR_EXH_RH:
    case P224_QUERY_CUR_PREHEAT:
      json[key] = (int) value;
      break;

    case P224_QUERY_CUR_SUP_TEMP:
    case P224_QUERY_CUR_EXH_TEMP:
      json[key] = value;
      break;

    case P224_QUERY_CUR_BYPASS:
      switch ((int) value) {
        case 0:
          strVal = F("init");   break;
        case 1:
        case 3:
          strVal = F("open");   break;
        case 2:
        case 4:
          strVal = F("closed"); break;
        default:
          strVal = String(value, 0); break;
      }
      json[key] = strVal;
      break;

    case P224_QUERY_STS_PREHEAT:
      switch ((int) value) {
        case 0:
          strVal = F("init"); break;
        case 1:
          strVal = F("off");  break;
        case 2:
          strVal = F("on");   break;
        case 3:
          strVal = F("test"); break;
        default:
          strVal = String(value, 0); break;
      }
      json[key] = strVal;
      break;

    case P224_QUERY_STS_FILTER:
      switch ((int) value) {
        case 0:
          strVal = F("clean"); break;
        case 1:
          strVal = F("dirty"); break;
        default:
          strVal = String(value, 0); break;
      }
      json[key] = strVal;
      break;

    default:
      break;
  }
}


void p224_showValueLoadPage(int16_t valueIdx, struct P224_data_struct *taskData) {
  byte errorcode;
  addRowLabel(Plugin_224_valuename(valueIdx, true));
  float res = p224_readValue(valueIdx, taskData, errorcode);
  if (errorcode == 0)
    addHtml(String(res));
  else
    addHtml(String(F("bus err=")) + errorcode);
}


boolean Plugin_224(byte function, struct EventStruct *event, String& string) {
  boolean success = false;

  switch (function) {
    case PLUGIN_DEVICE_ADD: {
      Device[++deviceCount].Number           = PLUGIN_ID_224;
      Device[deviceCount].Type               = DEVICE_TYPE_DUAL;
      Device[deviceCount].VType              = SENSOR_TYPE_STRING;
      Device[deviceCount].Ports              = 0;
      Device[deviceCount].PullUpOption       = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption      = false;
      Device[deviceCount].ValueCount         = 1;
      Device[deviceCount].SendDataOption     = true;
      Device[deviceCount].TimerOption        = true;
      Device[deviceCount].GlobalSyncOption   = true;
      success = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME: {
      string = F(PLUGIN_NAME_224);
      success = true;
      break;
    }

    case PLUGIN_READ: {
      P224_data_struct *taskData =
        static_cast<P224_data_struct *>(getPluginTaskData(event->TaskIndex));

      if ((nullptr == taskData) || !taskData->isInitialized())
        break;

      success = true;

      //todo String json(F("{"));
      ArduinoJson::DynamicJsonDocument json(512);

      // copy values from continuous read buffer
      for (int i = 0; i < P224_NR_OUTPUT_OPTIONS; ++i) {
        P224_data_struct::DataBuf& dataBuf = taskData->dataBuf[i];
        if (dataBuf.success) {
          Plugin_224_add2json(i, json, dataBuf.value);

          //todo json += String(F("\"")) + Plugin_224_valuename(i, false)
          //todo   + F("\":") + Plugin_224_formatValue(i, dataBuf.value) + F(",");
        }
        else {
          success = false;
          break;
        }
      }

      if (success) {
        // remove last comma
        //json.remove(json.length() - 1, 1);
        //json += F("}");
        serializeJson(json, event->String2);
        //sendData(event);
      }

      // triger new reading
      taskData->readIdx = 0;

      break;
    }

    default:
      success = Plugin_X224(function, event, string, PLUGIN_ID_224);
      break;
  }

  return success;
}

boolean Plugin_225(byte function, struct EventStruct *event, String& string) {
  boolean success = false;

  switch (function) {
    case PLUGIN_DEVICE_ADD: {
      Device[++deviceCount].Number           = PLUGIN_ID_225;
      Device[deviceCount].Type               = DEVICE_TYPE_DUAL;
      Device[deviceCount].VType              = SENSOR_TYPE_QUAD;
      Device[deviceCount].Ports              = 0;
      Device[deviceCount].PullUpOption       = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption      = true;
      Device[deviceCount].ValueCount         = P225_NR_OUTPUT_VALUES;
      Device[deviceCount].SendDataOption     = true;
      Device[deviceCount].TimerOption        = true;
      Device[deviceCount].GlobalSyncOption   = true;
      success = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME: {
      string = F(PLUGIN_NAME_225);
      success = true;
      break;
    }

    case PLUGIN_READ: {
      P224_data_struct *taskData =
        static_cast<P224_data_struct *>(getPluginTaskData(event->TaskIndex));

      if ((nullptr == taskData) || !taskData->isInitialized())
        break;

      success = true;

      // copy values from continuous read buffer
      for (int i = 0; i < P225_NR_OUTPUT_VALUES; ++i) {
        int16_t valueIdx = PCONFIG(P224_QUERY1_CONFIG_POS + i);
        P224_data_struct::DataBuf& dataBuf = taskData->dataBuf[valueIdx];
        if (dataBuf.success)
          UserVar[event->BaseVarIndex + i] = dataBuf.value;
        else
          success = false;
      }

      // triger new reading
      taskData->readIdx = 0;

      break;
    }

    default:
      success = Plugin_X224(function, event, string, PLUGIN_ID_225);
      break;
  }

  return success;
}

boolean Plugin_X224(byte function, struct EventStruct *event, String& string, int pluginId) {
  boolean success = false;

  switch (function) {

    case PLUGIN_GET_DEVICEVALUENAMES: {
      int nrOutputValues = (pluginId == PLUGIN_ID_224) ? P224_NR_OUTPUT_VALUES : P225_NR_OUTPUT_VALUES;

      for (byte i = 0; i < VARS_PER_TASK; ++i) {
        if (i < nrOutputValues) {
          const byte pconfigIndex = i + P224_QUERY1_CONFIG_POS;
          byte choice             = PCONFIG(pconfigIndex);
          safe_strncpy(
            ExtraTaskSettings.TaskDeviceValueNames[i],
            Plugin_224_valuename(choice, false),
            sizeof(ExtraTaskSettings.TaskDeviceValueNames[i]));
        } else {
          ZERO_FILL(ExtraTaskSettings.TaskDeviceValueNames[i]);
        }
      }
      break;
    }

    case PLUGIN_GET_DEVICEGPIONAMES: {
      serialHelper_getGpioNames(event);
      //event->String3 = formatGpioName_output_optional("DE");
      break;
    }

    case PLUGIN_WEBFORM_SHOW_CONFIG:
    {
      string += serialHelper_getSerialTypeLabel(event);
      success = true;
      break;
    }

    case PLUGIN_SET_DEFAULTS: {
      P224_DEV_ID   = P224_DEV_ID_DFLT;
      P224_MODEL    = P224_MODEL_DFLT;
      P224_BAUDRATE = P224_BAUDRATE_DFLT;

      if (pluginId == PLUGIN_ID_224) {
        P224_QUERY1   = P224_QUERY1_DFLT;
      }
      else {
        P224_QUERY1   = P225_QUERY1_DFLT;
        P224_QUERY2   = P225_QUERY2_DFLT;
        P224_QUERY3   = P225_QUERY3_DFLT;
        P224_QUERY4   = P225_QUERY4_DFLT;
      }

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_LOAD: {
      serialHelper_webformLoad(event);

      {
        // Modbus parameters put in scope to make sure the String array will not keep memory occupied.
        String options_baudrate[6];

        for (int i = 0; i < 6; ++i) {
          options_baudrate[i] = String(p224_storageValueToBaudrate(i));
        }
        addFormNumericBox(F("Modbus Address"), P224_DEV_ID_LABEL, P224_DEV_ID, 1,
                          247);
        addFormSelector(F("Baud Rate"), P224_BAUDRATE_LABEL, 6, options_baudrate,
                        NULL, P224_BAUDRATE);
      }

      P224_data_struct *taskData =
        static_cast<P224_data_struct *>(getPluginTaskData(event->TaskIndex));

      if ((nullptr != taskData) && taskData->isInitialized()) {
        String detectedString = taskData->modbus.detected_device_description;

        if (detectedString.length() > 0) {
          addFormNote(detectedString);
        }
        addRowLabel(F("Checksum (pass/fail/nodata)"));
        uint32_t reads_pass, reads_crc_failed, reads_nodata;
        taskData->modbus.getStatistics(reads_pass, reads_crc_failed, reads_nodata);
        String chksumStats;
        chksumStats  = reads_pass;
        chksumStats += '/';
        chksumStats += reads_crc_failed;
        chksumStats += '/';
        chksumStats += reads_nodata;
        addHtml(chksumStats);

        addFormSubHeader(F("Brink options"));

        // Config data is stored in the Brink device, not in the settings of ESPeasy.
        {
          byte errorcode;
          int value;

          value = taskData->modbus.readHoldingRegister(8000, errorcode);

          if (errorcode != 0 ||  value < 0 || value > 2)
            value = 3;
          String mbCtrl[5] = {F("off"), F("preset"), F("value"), F("n/a")};
          addFormSelector(F("Modbus control"), F("p224_fr_modbus_ctrl"), 4, mbCtrl, NULL, value);
        }

        // Check and save to reset the warning
        addFormCheckBox(F("Reset Filter Warning"), F("p224_reset_filter_warn"), false);
        addFormNote(F("Will reset filter usage time counter"));

        addFormSubHeader(F("Read test"));
        p224_showValueLoadPage(P224_QUERY_CUR_SUP_PRES, taskData);
        p224_showValueLoadPage(P224_QUERY_CUR_EXH_PRES, taskData);
        p224_showValueLoadPage(P224_QUERY_SET_SUP_FLOW, taskData);
        p224_showValueLoadPage(P224_QUERY_CUR_SUP_FLOW, taskData);
        p224_showValueLoadPage(P224_QUERY_CUR_SUP_RPM,  taskData);
        p224_showValueLoadPage(P224_QUERY_CUR_SUP_TEMP, taskData);
        p224_showValueLoadPage(P224_QUERY_CUR_SUP_RH,   taskData);
        p224_showValueLoadPage(P224_QUERY_SET_EXH_FLOW, taskData);
        p224_showValueLoadPage(P224_QUERY_CUR_EXH_FLOW, taskData);
        p224_showValueLoadPage(P224_QUERY_CUR_EXH_RPM,  taskData);
        p224_showValueLoadPage(P224_QUERY_CUR_EXH_TEMP, taskData);
        p224_showValueLoadPage(P224_QUERY_CUR_EXH_RH,   taskData);
        p224_showValueLoadPage(P224_QUERY_CUR_BYPASS,   taskData);
        p224_showValueLoadPage(P224_QUERY_STS_PREHEAT,  taskData);
        p224_showValueLoadPage(P224_QUERY_CUR_PREHEAT,  taskData);
        p224_showValueLoadPage(P224_QUERY_STS_FILTER,   taskData);
      }

      if (pluginId == PLUGIN_ID_225) {
        sensorTypeHelper_webformLoad_header();
        String options[P224_NR_OUTPUT_OPTIONS];

        for (int i = 0; i < P224_NR_OUTPUT_OPTIONS; ++i) {
          options[i] = Plugin_224_valuename(i, true);
        }

        for (byte i = 0; i < P225_NR_OUTPUT_VALUES; ++i) {
          const byte pconfigIndex = i + P224_QUERY1_CONFIG_POS;
          sensorTypeHelper_loadOutputSelector(event, pconfigIndex, i, P224_NR_OUTPUT_OPTIONS, options);
        }
      }

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE: {
      serialHelper_webformSave(event);

      // Save normal parameters
      for (int i = 0; i < P224_QUERY1_CONFIG_POS; ++i) {
        pconfig_webformSave(event, i);
      }

      // Save output selector parameters
      if (pluginId == PLUGIN_ID_225) {
        for (byte i = 0; i < P225_NR_OUTPUT_VALUES; ++i) {
          const byte pconfigIndex = i + P224_QUERY1_CONFIG_POS;
          const byte choice       = PCONFIG(pconfigIndex);
          sensorTypeHelper_saveOutputSelector(event, pconfigIndex, i, Plugin_224_valuename(choice, false));
        }
      }

      P224_data_struct *taskData =
        static_cast<P224_data_struct *>(getPluginTaskData(event->TaskIndex));

      if ((nullptr == taskData) || !taskData->isInitialized())
        break;

      byte errorcode = 0;

      uint16_t mbCtrl = getFormItemInt(F("p224_fr_modbus_ctrl"));
      if (mbCtrl >= 0 && mbCtrl <= 2)
        taskData->modbus.writeSingleRegister(8000, mbCtrl, errorcode);

      delay(1);

      // reset filter warning
      if (isFormItemChecked(F("p224_reset_filter_warn")))
      {
        taskData->modbus.writeSingleRegister(8010, 0x01, errorcode);
      }

      success = true;
      break;
    }

    case PLUGIN_INIT: {
      // apply INIT only if PORT is in range. Do not start INIT if port not set in the device page.
        const int16_t rxPin = CONFIG_PIN1;
        const int16_t txPin = CONFIG_PIN2;

      if (rxPin < 0 || rxPin > PIN_D_MAX || txPin < 0 || txPin > PIN_D_MAX)
        break;

      initPluginTaskData(event->TaskIndex, new P224_data_struct());

      P224_data_struct *taskData =
        static_cast<P224_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr == taskData)
        break;

      int baudRate = p224_storageValueToBaudrate(P224_BAUDRATE);

      if (!taskData->init(rxPin, txPin, P224_DEPIN, baudRate, P224_DEV_ID)) {
        clearPluginTaskData(event->TaskIndex);
        break;
      }

      success = true;
      break;
    }

    case PLUGIN_EXIT: {
      clearPluginTaskData(event->TaskIndex);
      success = true;
      break;
    }

    case PLUGIN_TEN_PER_SECOND: {
      // read data continuously one by one,
      // so that we will not block system in one bulk reading

      P224_data_struct *taskData =
        static_cast<P224_data_struct *>(getPluginTaskData(event->TaskIndex));

      if ((nullptr == taskData) || !taskData->isInitialized())
        break;

      int16_t valueIdx;

      if  (pluginId == PLUGIN_ID_224) {
        if (taskData->readIdx < 0 || taskData->readIdx >= P224_NR_OUTPUT_OPTIONS)
          break;

        valueIdx = taskData->readIdx;
      } else {
        if (taskData->readIdx < 0 || taskData->readIdx >= P225_NR_OUTPUT_VALUES)
          break;

        // allways read set supply flow so that we can compare when requesting new value
        if (taskData->readIdx == 0)
          P224_readValue2Buf(P224_QUERY_SET_SUP_FLOW, taskData);

        valueIdx = PCONFIG(P224_QUERY1_CONFIG_POS + taskData->readIdx);
      }

      P224_readValue2Buf(valueIdx, taskData);

      taskData->readIdx++;

      break;
    }

    case PLUGIN_WRITE: {
      P224_data_struct *taskData =
        static_cast<P224_data_struct *>(getPluginTaskData(event->TaskIndex));

      if ((nullptr == taskData) || !taskData->isInitialized())
        break;

      // Commands:
      // BrinkFlairCMD,ModBusCtrl,[0|1|2] (off|preset|value)
      // BrinkFlairCMD,Bypass,[0|1|2]     (automatic|closed|open)
      // BrinkFlairCMD,FlowSet,[0|1|2|3]  (holiday|low|normal|high)
      // BrinkFlairCMD,FlowVal,[0..325]

      String tmpString = string;
      String cmd = parseString(tmpString, 1);

      if (!cmd.equalsIgnoreCase(F("BrinkFlairCMD")))
        break;

      short value = 0;
      short mbAddress = -1;

      String setting = parseString(tmpString, 2);
      String valueStr = parseString(tmpString, 3);

      P224_data_struct::DataBuf& flowDataBuf = taskData->dataBuf[P224_QUERY_SET_SUP_FLOW];

      if (setting.equalsIgnoreCase(F("ModBusCtrl"))) {
        mbAddress = 8000;

        if (valueStr.equalsIgnoreCase(F("0")) || valueStr.equalsIgnoreCase(F("off")))
           value = 0;
        else if (valueStr.equalsIgnoreCase(F("1")) || valueStr.equalsIgnoreCase(F("preset")))
           value = 1;
        else if (valueStr.equalsIgnoreCase(F("2")) || valueStr.equalsIgnoreCase(F("value")))
           value = 2;
        else
           mbAddress = -1;
      }

      if (setting.equalsIgnoreCase(F("Bypass"))) {
        mbAddress = 6100;

        if (valueStr.equalsIgnoreCase(F("0")) || valueStr.equalsIgnoreCase(F("automatic")))
           value = 0;
        else if (valueStr.equalsIgnoreCase(F("1")) || valueStr.equalsIgnoreCase(F("closed")))
           value = 1;
        else if (valueStr.equalsIgnoreCase(F("2")) || valueStr.equalsIgnoreCase(F("open")))
           value = 2;
        else
           mbAddress = -1;
      }

      if (setting.equalsIgnoreCase(F("FlowSet"))) {
        mbAddress = 8001;

        if (valueStr.equalsIgnoreCase(F("0")) || valueStr.equalsIgnoreCase(F("holiday")))
           value = 0;
        else if (valueStr.equalsIgnoreCase(F("1")) || valueStr.equalsIgnoreCase(F("low")))
           value = 1;
        else if (valueStr.equalsIgnoreCase(F("2")) || valueStr.equalsIgnoreCase(F("normal")))
           value = 2;
        else if (valueStr.equalsIgnoreCase(F("3")) || valueStr.equalsIgnoreCase(F("high")))
           value = 3;
        else
           mbAddress = -1;
      }

      if (setting.equalsIgnoreCase(F("FlowVal"))) {
        mbAddress = 8002;

        value = atoi(valueStr.c_str());
        if (value < 0 || value > 500)
           mbAddress = -1;
        else {
          // don't set the same value again
          if (flowDataBuf.success && value == flowDataBuf.value) {
            addLog(LOG_LEVEL_DEBUG, String(F("BrinkFlair[")) + P224_DEV_ID
              + String(F("]: ")) + setting + String(F(" = ")) + value + String(F(" already set ")));
            success = true;
            break;
          }
        }
      }

      if (mbAddress < 0) {
        addLog(LOG_LEVEL_ERROR, String(F("BrinkFlair[")) + P224_DEV_ID
          + String(F("]: invalid ")) + setting + String(F(" = ")) + valueStr);
        break;
      }

      byte errorcode;
      taskData->modbus.writeSingleRegister(mbAddress, value, errorcode);

      if (errorcode != 0) {
        addLog(LOG_LEVEL_ERROR, String(F("BrinkFlair[")) + P224_DEV_ID
          + String(F("]: ")) + setting + String(F(" = ")) + value + String(F(", err = ")) + errorcode);
        break;
      }

      if (mbAddress == 8002) {
        // save flow setting now, actual value is beeing read-out on period only, and before that
        // we can receive more value setting commands
        flowDataBuf.value = value;
        flowDataBuf.success = true;
      }

      addLog(LOG_LEVEL_DEBUG, String(F("BrinkFlair[")) + P224_DEV_ID
        + String(F("]: ")) + setting + String(F(" = ")) + value);
      success = true;

      break;
    }

  } // switch
  return success;
}

const __FlashStringHelper* Plugin_224_valuename(byte value_nr, bool longDescr) {
  switch (value_nr) {
    case P224_QUERY_JSON:         return longDescr ? F("JSON")                            : F("data");
    case P224_QUERY_CUR_SUP_PRES: return longDescr ? F("Current Supply Pressure [Pa]")    : F("SuplPres");
    case P224_QUERY_CUR_EXH_PRES: return longDescr ? F("Current Exhaust Pressure [Pa]")   : F("ExhPres");
    case P224_QUERY_SET_SUP_FLOW: return longDescr ? F("Setpoint Supply Flow [m3/h]")     : F("SuplFlowReq");
    case P224_QUERY_CUR_SUP_FLOW: return longDescr ? F("Current Supply Flow [m3/h]")      : F("SuplFlow");
    case P224_QUERY_CUR_SUP_RPM:  return longDescr ? F("Current Supply Fan RPM")          : F("SuplFanRPM");
    case P224_QUERY_CUR_SUP_TEMP: return longDescr ? F("Current Supply Temp. [^C]")       : F("SuplTemp");
    case P224_QUERY_CUR_SUP_RH:   return longDescr ? F("Current Supply Rel Humid. [%]")   : F("SuplRH");
    case P224_QUERY_SET_EXH_FLOW: return longDescr ? F("Setpoint Exhaust Flow [m3/h]")    : F("ExhFlowReq");
    case P224_QUERY_CUR_EXH_FLOW: return longDescr ? F("Current Exhaust Flow [m3/h]")     : F("ExhFlow");
    case P224_QUERY_CUR_EXH_RPM:  return longDescr ? F("Current Exhaust Fan RPM")         : F("ExhFanRPM");
    case P224_QUERY_CUR_EXH_TEMP: return longDescr ? F("Current Exhaust Temp. [^C]")      : F("ExhTemp");
    case P224_QUERY_CUR_EXH_RH:   return longDescr ? F("Current Exhaust Rel. Humid. [%]") : F("ExhRH");
    case P224_QUERY_CUR_BYPASS:   return longDescr ? F("Bypass status [0-init/1,3-open/2,4-closed/255-error]") : F("BypassStat");
    case P224_QUERY_STS_PREHEAT:  return longDescr ? F("Preheater status [0-init/1-off/2-on/3-test]")          : F("PreheaterStat");
    case P224_QUERY_CUR_PREHEAT:  return longDescr ? F("Preheater power [%]")             : F("PreheaterPower");
    case P224_QUERY_STS_FILTER:   return longDescr ? F("Filter status [0-clean/1-dirty]") : F("FilterDirty");
  }
  return F("");
}

int p224_storageValueToBaudrate(byte baudrate_setting) {
  switch (baudrate_setting) {
    case 0:
      return 1200;
    case 1:
      return 2400;
    case 2:
      return 4800;
    case 3:
      return 9600;
    case 4:
      return 19200;
    case 5:
      return 38500;
  }
  return 19200;
}


bool P224_readValue2Buf(int16_t valueIdx, struct P224_data_struct *taskData) {
  P224_data_struct::DataBuf& dataBuf  = taskData->dataBuf[valueIdx];
  byte errorcode;

  float res = p224_readValue(valueIdx, taskData, errorcode);

  dataBuf.success = (errorcode == 0);

  if (dataBuf.success)
    dataBuf.value = res;

  return dataBuf.success;
}


float p224_readValue(int16_t valueIdx, struct P224_data_struct *taskData, byte &errorcode) {
  float divider = 1;
  short address;

  switch (valueIdx) {
    case P224_QUERY_CUR_SUP_PRES: address = 4023; break;
    case P224_QUERY_CUR_EXH_PRES: address = 4024; break;
    case P224_QUERY_SET_SUP_FLOW: address = 4031; break;
    case P224_QUERY_CUR_SUP_FLOW: address = 4032; break;
    case P224_QUERY_CUR_SUP_RPM:  address = 4034; break;
    case P224_QUERY_CUR_SUP_TEMP: address = 4036; divider = 10; break;
    case P224_QUERY_CUR_SUP_RH:   address = 4037; break;
    case P224_QUERY_SET_EXH_FLOW: address = 4041; break;
    case P224_QUERY_CUR_EXH_FLOW: address = 4042; break;
    case P224_QUERY_CUR_EXH_RPM:  address = 4044; break;
    case P224_QUERY_CUR_EXH_TEMP: address = 4046, divider = 10; break;
    case P224_QUERY_CUR_EXH_RH:   address = 4047; break;
    case P224_QUERY_CUR_BYPASS:   address = 4050; break;
    case P224_QUERY_STS_PREHEAT:  address = 4060; break;
    case P224_QUERY_CUR_PREHEAT:  address = 4061; break;
    case P224_QUERY_STS_FILTER:   address = 4100; break;

    default:
      errorcode = -1;
      return 0.0;
  }

  return taskData->modbus.readInputRegister(address, errorcode) / divider;
}


#endif // USES_P224
