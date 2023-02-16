/*
  emonTxV3.2 Continuous Sampling
  using EmonLibCM https://github.com/openenergymonitor/EmonLibCM
  Authors: Robin Emley, Robert Wall, Trystan Lea
  
  -----------------------------------------
  Part of the openenergymonitor.org project
  Licence: GNU GPL V3



Change Log:
v1.0: First release of EmonTxV3 Continuous Monitoring Firmware.
v1.1: First stable release, Set default node to 15
v1.2: Enable RF startup test sequence (factory testing), Enable DEBUG by default to support EmonESP
v1.3: Inclusion of watchdog
v1.4: Error checking to EEPROM config
v1.5: Faster RFM factory test
v1.6: Removed reliance on full jeelib for RFM, minimal rfm_send function implemented instead, thanks to Robert Wall
v1.7: Check radio channel is clear before transmit
v1.8: PayloadTx.E1 etc were unsigned long. 
v1.9: Unused variables removed.
v2.0: Power & energy calcs using "Assumed Vrms" added, serial output was switched off when rf output is on.
v2.1: Factory test transmission moved to Grp 1 to avoid interference with recorded data at power-up.  [RW - 30/1/21]
v2.1 (duplicate): printTemperatureSensorAddresses() was inside list_calibration() - reason not recorded [G.Hudson 23/12/21]
v2.3: The two v2.1 versions merged [RW - 9/3/22]

v2.4.0: Common single-phase continuous sampling firmware
        Radio format options:
        1. JeeLib Classic
        2. JeeLib Native
        3. LowPowerLabs
*/
const char *firmware_version = {"2.4.0\n\r"};
/*

emonhub.conf node decoder (nodeid is 15 when switch is off, 16 when switch is on)
See: https://github.com/openenergymonitor/emonhub/blob/emon-pi/configuration.md
copy the following into emonhub.conf:

[[15]]
  nodename = emonTx3cm15
  [[[rx]]]
    names = MSG, Vrms, P1, P2, P3, P4, E1, E2, E3, E4, T1, T2, T3, pulse
    datacodes = L,h,h,h,h,h,l,l,l,l,h,h,h,L
    scales = 1,0.01,1,1,1,1,1,1,1,1,0.01,0.01,0.01,1
    units = n,V,W,W,W,W,Wh,Wh,Wh,Wh,C,C,C,p

*/

#define RFM69_JEELIB_CLASSIC 1
#define RFM69_JEELIB_NATIVE 2
#define RFM69_LOW_POWER_LABS 3

#define RadioFormat RFM69_JEELIB_CLASSIC

// Comment/Uncomment as applicable
#define DEBUG                                              // Debug level print out

// #define EEWL_DEBUG

#define FACTORYTESTGROUP 1                                 // Transmit the Factory Test on Grp 1 
                                                           //   to avoid interference with recorded data at power-up.
#define OFF HIGH
#define ON LOW

#define RFM69CW

#include <Arduino.h>
#include <avr/wdt.h>

#if RadioFormat == RFM69_LOW_POWER_LABS
  #include <RFM69.h>                             // RFM69 LowPowerLabs radio library
#elif RadioFormat == RFM69_JEELIB_CLASSIC
  #include <rfmTxLib.h>                          // RFM69 transmit-only library using "JeeLib RFM69 Native" message format
#elif RadioFormat == RFM69_JEELIB_NATIVE
  #include <rfm69nTxLib.h>                       // RFM69 transmit-only library using "JeeLib RFM69 Native" message format
#endif

#include <emonEProm.h>                                     // OEM EEPROM library
#include <emonLibCM.h>                                     // OEM Continuous Monitoring library

// Include emonTx32_CM_config.ino in the same directory for settings functions & data

// Radio - checks for traffic
const int busyThreshold = -97;                             // Signal level below which the radio channel is clear to transmit
const byte busyTimeout = 15;                               // Time in ms to wait for the channel to become clear, before transmitting anyway

typedef struct {
    unsigned long Msg;
    int Vrms,P1,P2,P3,P4; 
    long E1,E2,E3,E4; 
    int T1,T2,T3;
    unsigned long pulse;
} PayloadTX;
PayloadTX emontx;                                                  // create a data packet for the RFM
static void showString (PGM_P s);
 
#define MAX_TEMPS 3                                        // The maximum number of temperature sensors
 
//---------------------------- emonTx Settings - Stored in EEPROM and shared with config.ino ------------------------------------------------
struct {
  byte RF_freq = RF69_433MHZ;                               // Frequency of radio module can be RF69_433MHZ, RF69_868MHZ or RF69_915MHZ. 
  byte networkGroup = 210;                                 // wireless network group, must be the same as emonBase / emonPi and emonGLCD. OEM default is 210
  byte  nodeID = 15;                                       // node ID for this emonTx.
  byte  rf_on = 1;                                         // RF - 0 = no RF, 1 = RF on.
  byte  rfPower = 25;                                      // 7 = -10.5 dBm, 25 = +7 dBm for RFM12B; 0 = -18 dBm, 31 = +13 dBm for RFM69CW. Default = 25 (+7 dBm)
  float vCal  = 268.97;                                    // (240V x 13) / 11.6V = 268.97 Calibration for UK AC-AC adapter 77DB-06-09
  float vCal_USA = 130.0;                                  // (120V × 13) / 12.0V = 130.0  Calibration for US AC-AC adapter 77DA-10-09 
  float assumedVrms = 240.0;                               // Assumed Vrms when no a.c. is detected
  float lineFreq = 50;                                     // Line Frequency = 50 Hz
  float i1Cal = 90.9;                                      // (100 A / 50 mA / 22 Ohm burden) = 90.9
  float i1Lead = 0.2;                                      // 0.2° phase lead
  float i2Cal = 90.9;
  float i2Lead = 0.2;
  float i3Cal = 90.9;
  float i3Lead = 0.2;
  float i4Cal = 16.67;                                     // (100 A / 50 mA / 120 Ohm burden) = 16.67
  float i4Lead = 2.2;                                      // 2.2° phase lead
  float period = 9.85;                                     // datalogging period - should be fractionally less than the PHPFINA database period in emonCMS
  bool  pulse_enable = true;                               // pulse counting
  int   pulse_period = 100;                                // pulse min period - 0 = no de-bounce
  bool  temp_enable = true;                                // enable temperature measurement
  DeviceAddress allAddresses[MAX_TEMPS];                   // sensor address data
  bool  showCurrents = false;                              // Print to serial voltage, current & p.f. values
  bool  json_enabled = false;                              // JSON Enabled - false = key,Value pair, true = JSON, default = false: Key,Value pair.  
} EEProm;

uint16_t eepromSig = 0x0014;                               // oemEProm signature - see oemEProm Library documentation for details.

#ifdef EEWL_DEBUG
  extern EEWL EVmem;
#endif

#if RadioFormat == RFM69_LOW_POWER_LABS
  RFM69 radio;
#endif

DeviceAddress allAddresses[MAX_TEMPS];                     // Array to receive temperature sensor addresses
/*   Example - how to define temperature sensors, prevents an automatic search
DeviceAddress allAddresses[] = {       
    {0x28, 0x81, 0x43, 0x31, 0x7, 0x0, 0xFF, 0xD9}, 
    {0x28, 0x8D, 0xA5, 0xC7, 0x5, 0x0, 0x0, 0xD5},         // Use the actual addresses, as many as required
    {0x28, 0xC9, 0x58, 0x32, 0x7, 0x0, 0x0, 0x89}          // up to a maximum of 6    
};
*/

int allTemps[MAX_TEMPS];                                   // Array to receive temperature measurements

bool  USA=false;

bool calibration_enable = true;                           // Enable on-line calibration when running. 
                                                           // For safety, thus MUST default to false. (Required due to faulty ESP8266 software.)

//----------------------------emonTx V3 hard-wired connections-----------------------------------
const byte LEDpin      = 6;  // emonTx V3 LED
const byte DS18B20_PWR = 19; // DS18B20 Power

//---------------------------------CT availability status----------------------------------------
byte CT_count = 0;
bool CT1, CT2, CT3, CT4;     // Record if CT present during startup


//----------------------------------------Setup--------------------------------------------------
void setup() 
{  
  wdt_enable(WDTO_8S);
  
  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin,HIGH);
  
  // Serial---------------------------------------------------------------------------------
  Serial.begin(115200);

  // ---------------------------------------------------------------------------------------

  #ifdef DEBUG
    Serial.print(F("emonTx V3.2 Continuous Monitoring V")); Serial.write(firmware_version);
    Serial.println(F("OpenEnergyMonitor.org"));
  #else
    Serial.println(F("describe:EmonTX3CM"));
  #endif
  
  #if RadioFormat == RFM69_JEELIB_CLASSIC
    EEProm.rf_on = 2;
  #endif
 
  load_config(true);                                                   // Load RF config from EEPROM (if any exists)
  
  if (EEProm.rf_on)
  {
    #ifdef DEBUG
      #ifdef RFM12B
      Serial.print(F("RFM12B "));
      #endif
      #ifdef RFM69CW
      Serial.print(F("RFM69CW "));
      #endif
      Serial.print(F(" Freq: "));
      if (EEProm.RF_freq == RF69_433MHZ) Serial.print(F("433MHz"));
      if (EEProm.RF_freq == RF69_868MHZ) Serial.print(F("868MHz"));
      if (EEProm.RF_freq == RF69_915MHZ) Serial.print(F("915MHz"));
      Serial.print(F(" Group: ")); Serial.print(EEProm.networkGroup);
      Serial.print(F(" Node: ")); Serial.print(EEProm.nodeID);
      Serial.println(F(" "));
    #endif
  }
  
  // Check connected CT sensors ------------------------------------------------------------
  if (analogRead(1) > 0) {CT1 = 1; CT_count++;} else CT1=0;            // check to see if CT is connected to CT1 input
  if (analogRead(2) > 0) {CT2 = 1; CT_count++;} else CT2=0;            // check to see if CT is connected to CT2 input
  if (analogRead(3) > 0) {CT3 = 1; CT_count++;} else CT3=0;            // check to see if CT is connected to CT3 input
  if (analogRead(4) > 0) {CT4 = 1; CT_count++;} else CT4=0;            // check to see if CT is connected to CT4 input

  // ---------------------------------------------------------------------------------------

  if (EEProm.rf_on)
  {
    Serial.println(F("Factory Test"));
    #if RadioFormat == RFM69_LOW_POWER_LABS
      radio.initialize(RF69_433MHZ,EEProm.nodeID,EEProm.networkGroup);  
      radio.encrypt("89txbe4p8aik5kt3");                                                      // initialize RFM
      radio.setPowerLevel(EEProm.rfPower);
    #else
      rfm_init();                                                        // initialize RFM
    #endif
    for (int i=10; i>=0; i--)                                          // Send RF test sequence using test Group (for factory testing)
    {
      emontx.P1=i;
      #if RadioFormat == RFM69_LOW_POWER_LABS
        radio.send(5, (const void*)(&emontx), sizeof(emontx));
      #else
        PayloadTX tmp = emontx;
        if (EEProm.rf_on == 2) {
            byte WHITENING = 0x55;
            for (byte i = 0, *p = (byte *)&tmp; i < sizeof tmp; i++, p++)
                *p ^= (byte)WHITENING;
        }
        rfm_send((byte *)&tmp, sizeof(tmp), FACTORYTESTGROUP, EEProm.nodeID, EEProm.RF_freq, EEProm.rfPower, busyThreshold, busyTimeout);
      #endif
      delay(100);
    }
    #if RadioFormat == RFM69_LOW_POWER_LABS
      radio.sleep();
    #endif
    emontx.P1=0;
    delay(EEProm.nodeID * 20);                                 // try to avoid r.f. collisions at start-up
  }
  
  // ---------------------------------------------------------------------------------------
  #ifdef DEBUG
    if (CT_count==0) {
      Serial.println(F("NO CT's detected"));
    } else {
      if (CT1) { Serial.print(F("CT1 detected, i1Cal:")); Serial.println(EEProm.i1Cal); }
      if (CT2) { Serial.print(F("CT2 detected, i2Cal:")); Serial.println(EEProm.i2Cal); }
      if (CT3) { Serial.print(F("CT3 detected, i3Cal:")); Serial.println(EEProm.i3Cal); }
      if (CT4) { Serial.print(F("CT4 detected, i4Cal:")); Serial.println(EEProm.i4Cal); }
    }
    delay(200);
  #endif

#ifdef EEWL_DEBUG
  Serial.print("End of mem=");Serial.print(E2END);
  Serial.print("  Avail mem=");Serial.print((E2END>>2) * 3);
  Serial.print("  Start addr=");Serial.print(E2END - (((E2END>>2) * 3) / (sizeof(mem)+1))*(sizeof(mem)+1));
  Serial.print("  Num blocks=");Serial.println(((E2END>>2) * 3) / 21);
  EVmem.dump_buffer();
#endif

  // ----------------------------------------------------------------------------
  // EmonLibCM config
  // ----------------------------------------------------------------------------
  EmonLibCM_SetADC_VChannel(0, EEProm.vCal);                           // ADC Input channel, voltage calibration - for Ideal UK Adapter = 268.97
  EmonLibCM_SetADC_IChannel(1, EEProm.i1Cal, EEProm.i1Lead);           // ADC Input channel, current calibration, phase calibration
  EmonLibCM_SetADC_IChannel(2, EEProm.i2Cal, EEProm.i2Lead);           // The current channels will be read in this order
  EmonLibCM_SetADC_IChannel(3, EEProm.i3Cal, EEProm.i3Lead);           // 90.91 for 100 A : 50 mA c.t. with 22R burden - v.t. leads c.t by ~4.2 degrees
  EmonLibCM_SetADC_IChannel(4, EEProm.i4Cal, EEProm.i4Lead);           // 16.67 for 100 A : 50 mA c.t. with 120R burden - v.t. leads c.t by ~1 degree

  EmonLibCM_ADCCal(3.3);                                               // ADC Reference voltage, (3.3 V for emonTx,  5.0 V for Arduino)
  // EmonLibCM_cycles_per_second(60);                                  // uncomment or set in EEPROM for mains frequency 60Hz
  EmonLibCM_datalog_period(EEProm.period);                             // period of readings in seconds - normal value for emoncms.org  

  EmonLibCM_setAssumedVrms(EEProm.assumedVrms);

  EmonLibCM_setPulseEnable(EEProm.pulse_enable);                       // Enable pulse counting
  EmonLibCM_setPulsePin(2);
  EmonLibCM_setPulseMinPeriod(EEProm.pulse_period);

  EmonLibCM_setTemperatureDataPin(5);                                  // OneWire data pin (emonTx V3.2)
  EmonLibCM_setTemperaturePowerPin(19);                                // Temperature sensor Power Pin - 19 for emonTx V3.2  (-1 = Not used. No sensors, or sensor are permanently powered.)
  EmonLibCM_setTemperatureResolution(11);                              // Resolution in bits, allowed values 9 - 12. 11-bit resolution, reads to 0.125 degC
  EmonLibCM_setTemperatureAddresses(EEProm.allAddresses);              // Name of array of temperature sensors
  EmonLibCM_setTemperatureArray(allTemps);                             // Name of array to receive temperature measurements
  EmonLibCM_setTemperatureMaxCount(MAX_TEMPS);                         // Max number of sensors, limited by wiring and array size.
  
  {
    long e0=0, e1=0, e2=0, e3=0;
    unsigned long p=0;
    
    recoverEValues(&e0,&e1,&e2,&e3,&p);
    EmonLibCM_setWattHour(0, e0);
    EmonLibCM_setWattHour(1, e1);
    EmonLibCM_setWattHour(2, e2);
    EmonLibCM_setWattHour(3, e3);
    EmonLibCM_setPulseCount(p);
  }

#ifdef EEWL_DEBUG
  EVmem.dump_control();
  EVmem.dump_buffer();  
#endif
  
  EmonLibCM_TemperatureEnable(EEProm.temp_enable);  
  EmonLibCM_Init();                                                    // Start continuous monitoring.
  emontx.Msg = 0;
  printTemperatureSensorAddresses();

  byte numSensors = EmonLibCM_getTemperatureSensorCount();
  if (numSensors==0) {
    // Serial.println(F("No temperature sensors detected, disabling temperature"));
    // EEProm.temp_enable = 0;
    // EmonLibCM_TemperatureEnable(EEProm.temp_enable); 
  }
  
  // Speed up startup by making first reading 2s
  EmonLibCM_datalog_period(2.0);
}

void loop()             
{
  getSettings();
  
  if (EmonLibCM_Ready())   
  {
    #ifdef DEBUG
    if (emontx.Msg==0) 
    {
      digitalWrite(LEDpin,LOW);
      EmonLibCM_datalog_period(EEProm.period); 
      if (EmonLibCM_acPresent())
        Serial.println(F("AC present - Real Power calc enabled"));
      else
      {
        Serial.print(F("AC missing - Apparent Power calc enabled, assuming ")); Serial.print(EEProm.assumedVrms); Serial.println(F(" V"));
      }
    }
    delay(5);
    #endif

    emontx.Msg++;

    // Other options calculated by EmonLibCM
    // RMS Current:    EmonLibCM_getIrms(ch)
    // Apparent Power: EmonLibCM_getApparentPower(ch)
    // Power Factor:   EmonLibCM_getPF(ch)
    
    emontx.P1 = EmonLibCM_getRealPower(0); 
    emontx.E1 = EmonLibCM_getWattHour(0); 

    emontx.P2 = EmonLibCM_getRealPower(1); 
    emontx.E2 = EmonLibCM_getWattHour(1); 
    
    emontx.P3 = EmonLibCM_getRealPower(2); 
    emontx.E3 = EmonLibCM_getWattHour(2); 
  
    emontx.P4 = EmonLibCM_getRealPower(3); 
    emontx.E4 = EmonLibCM_getWattHour(3); 

    if (EmonLibCM_acPresent()) {
      emontx.Vrms = EmonLibCM_getVrms() * 100;
    } else {
      emontx.Vrms = EmonLibCM_getAssumedVrms() * 100;
    }
    
    emontx.T1 = allTemps[0];
    emontx.T2 = allTemps[1];
    emontx.T3 = allTemps[2];

    emontx.pulse = EmonLibCM_getPulseCount();
    
    if (EEProm.rf_on)
    {
      #if RadioFormat == RFM69_LOW_POWER_LABS
        radio.sendWithRetry(5, (const void*)(&emontx), sizeof(emontx));
        radio.sleep();
      #else
        PayloadTX tmp = emontx;
        if (EEProm.rf_on == 2) {
            byte WHITENING = 0x55;
            for (byte i = 0, *p = (byte *)&tmp; i < sizeof tmp; i++, p++)
                *p ^= (byte)WHITENING;
        }
        rfm_send((byte *)&tmp, sizeof(tmp), EEProm.networkGroup, EEProm.nodeID, EEProm.RF_freq, EEProm.rfPower, busyThreshold, busyTimeout);     //send data
      #endif

      /*
      if (rf.sendWithRetry(5,(byte *)&tmp, sizeof(tmp))) {
        Serial.println("ack");
        emontx.T1 += rf.retry_count();
      } else {
        emontx.T1 += rf.retry_count()+1;
      }
      */
      
      delay(50);
    }

    if (EEProm.json_enabled) {
      // ---------------------------------------------------------------------
      // JSON Format
      // ---------------------------------------------------------------------
      Serial.print(F("{\"MSG\":")); Serial.print(emontx.Msg);
      Serial.print(F(",\"Vrms\":")); Serial.print(emontx.Vrms*0.01);

      if (CT1) { Serial.print(F(",\"P1\":")); Serial.print(emontx.P1); }
      if (CT2) { Serial.print(F(",\"P2\":")); Serial.print(emontx.P2); }
      if (CT3) { Serial.print(F(",\"P3\":")); Serial.print(emontx.P3); }
      if (CT4) { Serial.print(F(",\"P4\":")); Serial.print(emontx.P4); }

      if (CT1) { Serial.print(F(",\"E1\":")); Serial.print(emontx.E1); }
      if (CT2) { Serial.print(F(",\"E2\":")); Serial.print(emontx.E2); }
      if (CT3) { Serial.print(F(",\"E3\":")); Serial.print(emontx.E3); }
      if (CT4) { Serial.print(F(",\"E4\":")); Serial.print(emontx.E4); }

      if (emontx.T1!=30000) { Serial.print(F(",\"T1\":")); Serial.print(emontx.T1*0.01); }
      if (emontx.T2!=30000) { Serial.print(F(",\"T2\":")); Serial.print(emontx.T2*0.01); }
      if (emontx.T3!=30000) { Serial.print(F(",\"T3\":")); Serial.print(emontx.T3*0.01); }

      Serial.print(F(",\"pulse\":")); Serial.print(emontx.pulse);
      Serial.println(F("}"));
      delay(60);
      
    } else {
  
      // ---------------------------------------------------------------------
      // Key:Value format, used by EmonESP & emonhub EmonHubOEMInterfacer
      // ---------------------------------------------------------------------
      Serial.print(F("MSG:")); Serial.print(emontx.Msg);
      Serial.print(F(",Vrms:")); Serial.print(emontx.Vrms*0.01);
      
      if (CT1) { Serial.print(F(",P1:")); Serial.print(emontx.P1); }
      if (CT2) { Serial.print(F(",P2:")); Serial.print(emontx.P2); }
      if (CT3) { Serial.print(F(",P3:")); Serial.print(emontx.P3); }
      if (CT4) { Serial.print(F(",P4:")); Serial.print(emontx.P4); }
         
      if (CT1) { Serial.print(F(",E1:")); Serial.print(emontx.E1); }
      if (CT2) { Serial.print(F(",E2:")); Serial.print(emontx.E2); }
      if (CT3) { Serial.print(F(",E3:")); Serial.print(emontx.E3); }
      if (CT4) { Serial.print(F(",E4:")); Serial.print(emontx.E4); }
       
      if (emontx.T1!=30000) { Serial.print(F(",T1:")); Serial.print(emontx.T1*0.01); }
      if (emontx.T2!=30000) { Serial.print(F(",T2:")); Serial.print(emontx.T2*0.01); }
      if (emontx.T3!=30000) { Serial.print(F(",T3:")); Serial.print(emontx.T3*0.01); }
  
      Serial.print(F(",pulse:")); Serial.print(emontx.pulse);

      if (!EEProm.showCurrents) {
        Serial.println();
        delay(40);
      } else {
        // to show voltage, current & power factor for calibration:
        Serial.print(F(",I1:")); Serial.print(EmonLibCM_getIrms(EmonLibCM_getLogicalChannel(1)),3);
        Serial.print(F(",I2:")); Serial.print(EmonLibCM_getIrms(EmonLibCM_getLogicalChannel(2)),3);
        Serial.print(F(",I3:")); Serial.print(EmonLibCM_getIrms(EmonLibCM_getLogicalChannel(3)),3);
        Serial.print(F(",I4:")); Serial.print(EmonLibCM_getIrms(EmonLibCM_getLogicalChannel(4)),3);

        Serial.print(F(",pf1:")); Serial.print(EmonLibCM_getPF(EmonLibCM_getLogicalChannel(1)),4);
        Serial.print(F(",pf2:")); Serial.print(EmonLibCM_getPF(EmonLibCM_getLogicalChannel(2)),4);
        Serial.print(F(",pf3:")); Serial.print(EmonLibCM_getPF(EmonLibCM_getLogicalChannel(3)),4);
        Serial.print(F(",pf4:")); Serial.println(EmonLibCM_getPF(EmonLibCM_getLogicalChannel(4)),4);
        delay(80);
      }
    }
    digitalWrite(LEDpin,HIGH); delay(50);digitalWrite(LEDpin,LOW);
    // End of print out ----------------------------------------------------
    storeEValues(emontx.E1,emontx.E2,emontx.E3,emontx.E4,emontx.pulse);
  }
  wdt_reset();
  delay(20);
}
