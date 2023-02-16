/*  Energy monitor for 3-phase 3-wire or 4-wire installation

    Based on a single phase energy diverter by Martin Roberts 2/12/12, which itself was
    based on emonTx hardware from OpenEnergyMonitor http://openenergymonitor.org/emon/
    this version implements a phase-locked loop to synchronise to the mains supply and
    supports a single Dallas DS18B20 temperature sensor.
    
    Temp fault codes: 300 deg = Sensor has never been detected since power-up/reset. 
                      302 deg = Sensor returned an out-of-range value. 
                      304 deg = Faulty sensor, sensor broken or disconnected.
                      85  deg   although in range, might indicate a wiring fault.

    Three-phase energy monitor
*/

#define VERSION "emonTx_3Phase_PLL - Firmware version 2.1.0 "                      
    
/*
    History (single Phase energy diverter):
    2/12/12  first published version
    3/12/12  diverted power calculation & transmission added
    4/12/12  manual power input added for testing
    10/12/12 high & low energy thresholds added to reduce flicker
    12/10/13 PB added 3rd CT channel to determine diverted power
    09/09/14 EmonTx v3 option added by PB ( http://openenergymonitor.org/emon/node/5714 )
    
    Three-phase energy monitor using "classic JeeLib"
    V 1.0    10/12/17 The original extensively modified with diverter code removed
                     and extended for 3-phase operation. 
    V 1.1    20/02/18 Sleep (sleep_mode()) removed from rfm_sleep() in rfm.ino
    V 1.2    12/03/18 Temperature fault codes were
                      300 deg = Faulty sensor, sensor broken or disconnected.
                      301 deg = Sensor has never been detected since power-up/reset. 
                      302 deg = Sensor returned an out-of-range value. 
    V 1.3    25/09/18 Declaration of showString() added - omission thereof caused 
                      compiler error in Arduino IDE V1.8.7
    V 1.4    22/10/18 Added 3-wire options & switch for 4-wire / 3-wire operation, 
                      free choice of phase for all four c.t's. 
    V 1.5            Documentation change only
    V 1.6    24/2/19  Unwanted space in emonESP output removed. No change to documentation.
    V 1.7            Versions 1.5 & 1.6 showed "V1.4" in serial print. Order of lines 457 & 458 was reversed. Added preprocessor substitution VERSION to replace a variable. 

    V 2.0.0  11/07/21  Derived from the "Classic JeeLib" version. Uses either Jeelib "Classic" or "RFM69 Native" format and OEM "rfm69nTxLib.h" library instead of rfm.ino file.
    V 2.1.0  16/02/23  Support for LowPowerLabs radio format


    emonhub.conf node decoder settings for this sketch:

    [[11]]
        nodename = emonTx_three_phase
        firmware = three_phase
        hardware = emonTx V3.2/V3.4/Shield
    [[[rx]]]
        names = power1, power2, power3, power4, Vrms, temp1, temp2, temp3, temp4, temp5, temp6, pulsecount
        datacodes = h, h, h, h, h, h, h, h, h, h, h, L
        scales = 1,1,1,1,0.01,0.01,0.01,0.01,0.01,0.01,0.01,1
        units =W,W,W,W,V,C,C,C,C,C,C,p
    
    IMPORTANT NOTE:
    When used in the 3-wire configuration with one line conductor being treated as the 'neutral', the
    individual powers recorded for the other two line conductors are meaningless, only the TOTAL power
    of the line conductors has a physical meaning. Therefore, 'power1' is the COMBINED power of inputs 1 & 2
    and all the values for input 2 alone are set to zero.
    Note also: Only one temperature sensor may be connected. All remaining temperatures will read "300.00"

    For serial input, emonHub requires "datacode = 0" in place of "datacodes = ...." as above.
    
    This sketch requires the OEM RFM69CW transmit-only library "rfm69nTxLib.h" and uses the "JeeLib RFM69 Native" message format.
*/

#define RFM69_JEELIB_CLASSIC 1
#define RFM69_JEELIB_NATIVE 2
#define RFM69_LOW_POWER_LABS 3

#define RadioFormat RFM69_LOW_POWER_LABS

#define EMONTX_V34                               // Sets the I/O pin allocation. 
                                                 // use EMONTX_V2 or EMONTX_V32 or EMONTX_V34 or EMONTX_SHIELD as appropriate
                                                 // NOTE: You must still set the correct calibration coefficients

//--------------------------------------------------------------------------------------------------
// #define DEBUGGING                             // enable this line to include debugging print statements
                                                 //  This is turned off when SERIALOUT or EMONESP (see below) is defined.
                                                 
#define SERIALPRINT                              // include 'human-friendly' print statement for commissioning - comment this line to exclude.

// Pulse counting settings
#define USEPULSECOUNT                            // include the ability to count pulses. Comment this line if pulse counting is not required. When enabled, pulse counting can still be turned on and off in the on-line settings.
#define PULSEINT 1                               // Interrupt no. for pulse counting: EmonTx V2 = 0, EmonTx V3 = 1, EmonTx Shield - see Wiki
#define PULSEPIN 3                               // Interrupt input pin: EmonTx V2 = 2, EmonTx V3 = 3, EmonTx Shield - see Wiki
#define PULSEMINPERIOD 100                       // minimum period between pulses (ms) - default pulse output meters = 100ms
                                                 //   Set to 0 for electronic sensor with solid-state output.
                                                 
// Output settings                               // THIS SKETCH WILL NOT WORK WITH THE RFM12B radio.
#define RFM69CW                                  // The type of output: Radio Module, serial or none.
                                                 // Can be RFM69CW 
                                                 //   or SERIALOUT if a wired serial connection is used (space-separated values for the
                                                 //     "direct serial" EmonHubSerialInterfacer
                                                 //     (see https://github.com/openenergymonitor/emonhub/tree/emon-pi/conf/
                                                 //        interfacer_examples/directserial) 
                                                 //   or EMONESP if an ESP WiFi module is used 
                                                 //     (see https://github.com/openenergymonitor/emonTxFirmware/blob/master/emonTxV3/
                                                 //       noRF/emonTxV3_DirectSerial/emonTxV3_DirectSerial.ino)
                                                 //       (key:value pairs for the EmonHubTx3eInterfacer)
                                                 //   or don't define anything if neither radio nor serial connection is required - in which case 
                                                 //      the IDE serial monitor output will be for information and debugging only.
                                                 // The sketch will hang if the wrong radio module is specified, or if one is specified and not fitted.
                                                 // For all serial output, the maximum is 9600 baud. The emonESP module must be set to suit.
                                            
#if RadioFormat == RFM69_LOW_POWER_LABS
  #include <RFM69.h>                             // RFM69 LowPowerLabs radio library
#elif RadioFormat == RFM69_JEELIB_CLASSIC
  #include <rfmTxLib.h>                          // RFM69 transmit-only library using "JeeLib RFM69 Native" message format
#elif RadioFormat == RFM69_JEELIB_NATIVE
  #include <rfm69nTxLib.h>                       // RFM69 transmit-only library using "JeeLib RFM69 Native" message format
#endif
  
const int busyThreshold = -97;                   // Signal level below which the radio channel is clear to transmit
const byte busyTimeout = 15;                     // Time in ms to wait for the channel to become clear, before transmitting anyway

typedef uint8_t DeviceAddress[8];


//-------------------- emonTx Settings - Stored in EEPROM and shared with config.ino ---------------
//-------------------- Constants which must be set individually for each system --------------------
#include <emonEProm.h>                           // OEM EEPROM library

struct {
  byte RF_freq = RF69_433MHZ;                     // Frequency of radio module can be RFM_433MHZ, RFM_868MHZ or RFM_915MHZ. 
  byte networkGroup = 210;                       // wireless network group - needs to be same as emonBase and emonGLCD. OEM default is 210
  byte rf_on = 1;                                // RF - 0 = no RF, 1 = RF on.
  byte nodeID = 11;                              // node ID for this emonTx. Or nodeID-1 if DIP switch 1 is ON. 
  byte  rfPower = 25;                            // 7 = -10.5 dBm, 25 = +7 dBm for RFM12B; 0 = -18 dBm, 31 = +13 dBm for RFM69CW. Default = 25 (+7 dBm)
  double vCal  = 268.97;                         // (240V x 13) / 11.6V = 268.97 Calibration for UK AC-AC adapter 77DB-06-09, 
                                                 //   for the EU adapter use 260.00, for the USA adapter use 130.00
  double i1Cal = 90.91;                          // (100 A / 50 mA / 22 Ohm burden) = 90.91 or 60.6 for emonTx Shield
  double i1Lead = 2.00;                          // 2° phase lead
  double i2Cal = 90.91;
  double i2Lead = 2.00;
  double i3Cal = 90.91;
  double i3Lead = 2.00;
  double i4Cal = 16.67;                          // (100 A / 50 mA / 120 Ohm burden) = 16.67
  double i4Lead = 0.20;                          // 0.2° phase lead
  float period = 9.85;                           // datalogging period - should be fractionally less than the PHPFINA database period in emonCMS
 
  bool  pulse_enable = true;                     // pulse counting
  int   pulse_period = 110;                      // pulse min period - 0 = no de-bounce
  bool  temp_enable = true;                      // enable temperature measurement
  DeviceAddress allAddresses[6];                 // sensor address data
  bool  showCurrents = false;                    // Print to serial voltage, current & p.f. values  

} EEProm;
  
uint16_t eepromSig = 0x0016;                     // oemEProm signature - see oemEProm Library documentation for details.
 
#define VCAL_EU 260.0                            // can use DIP switch 2 to set this as the starting value.                     


//--------------------------------------------------------------------------------------------------


#define WIRES 4-WIRE      // either 4-WIRE (default, measure voltage L1 - N) or 3-WIRE (no neutral, measure voltage L1 - L2)

#define CT1Phase PHASE1   // either PHASE1, PHASE2 or PHASE3 to attach c.t.1 to a phase. This c.t. MUST be used.
#define CT2Phase PHASE2   // similarly to attach c.t.2 to a phase. This c.t. MUST be used.
#define CT3Phase PHASE3   // similarly to attach c.t.3 to a phase. This c.t. MUST be used, do not comment this line.
#define CT4Phase PHASE1   // similarly to attach c.t.4 to a phase or comment this line if c.t.4 is not used 
                          //   (See also NUMSAMPLES below)
#define LEDISLOCK         // comment this out for LED pulsed during transmission
                          //  otherwise LED shows loop is locked, and occults to show transmission, but that is not easily visible

                          
//--------------------------------------------------------------------------------------------------
// other system constants
#define SUPPLY_VOLTS 3.3  // used here because it's more accurate than the internal band-gap reference. Use 5.0 for Arduino / emonTx Shield
#define SUPPLY_FREQUENCY 50
#define NUMSAMPLES 36     // number of times to sample each 50/60Hz cycle - for a 4-wire system, this must be a multiple of 3;
                          //   for a 3-wire system, it must be a multiple of 6
                          // Permissible maximum values (serial only) 50 Hz, 3 c.t: 45         60 Hz, 3 c.t: 36
                          //                                          50 Hz, 4 c.t: 36         60 Hz, 4 c.t: 33
#define ADC_BITS 10       // ADC Resolution
#define ADC_RATE 64       // Time between successive ADC conversions in microseconds

#define PLLTIMERRANGE 100 // PLL timer range limit ~ +/-0.5Hz
#define PLLLOCKRANGE 40   // allowable ADC range to enter locked state
#define PLLUNLOCKRANGE 80 // allowable ADC range to remain locked
#define PLLLOCKCOUNT 100  // number of cycles to determine if PLL is locked

//--------------------------------------------------------------------------------------------------
//
//   Users should not need to change anything below here
//
//
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// I/O pins for debugging
// Define these only for testing. On the emonTx V3.x, it may be necessary to comment out and
//  remove external connections to the interrupt pin, the One-wire or DS18B20 power pins.
//#define SYNCPIN 19  // this output will be a 50Hz square wave locked to the 50Hz input
//#define SAMPPIN 19  // this output goes high each time an ADC conversion starts or completes
//#define TXPIN 3     // this output goes high each time a radio transmission takes place
//--------------------------------------------------------------------------------------------------

// Arduino I/O pin usage
#if defined(EMONTX_V2)
// EmonTx v2 Pin references
#undef CT4Phase
#define VOLTSPIN 2
#define CT1PIN 3
#define CT2PIN 0
#define CT3PIN 1
#define LEDPIN 9
#define RFMSELPIN 10
#define RFMIRQPIN 2
#define SDOPIN 12
#define W1PIN 4         // 1-Wire pin for temperature

#elif defined(EMONTX_V32)
// EmonTx v3.2 Pin references
#define VOLTSPIN 0
#define CT1PIN 1
#define CT2PIN 2
#define CT3PIN 3
#define CT4PIN 4
#define LEDPIN 6
#define RFMSELPIN 4     // Pins for the RFM Radio module
#define RFMIRQPIN 3
#define SDOPIN 12
#define W1PIN 5         // 1-Wire pin for temperature
#define DS18B20_PWR 19  // Power for 1-wire temperature sensor

#elif defined EMONTX_SHIELD

// EmonTx Shield Pin references
#define VOLTSPIN 0
#define CT1PIN 1
#define CT2PIN 2
#define CT3PIN 3
#define CT4PIN 4
#define LEDPIN 9
#define RFMSELPIN 5   // See Wiki
#define RFMIRQPIN 3   // See Wiki
#define SDOPIN 12
#define W1PIN 4       // 1-Wire pin for temperature

#else
// EmonTx v3.4 Pin references
#define VOLTSPIN 0
#define CT1PIN 1
#define CT2PIN 2
#define CT3PIN 3
#define CT4PIN 4
#define LEDPIN 6
#define RFMSELPIN 10    // Pins for the RFM Radio module
#define RFMIRQPIN 2
#define SDOPIN 12
#define W1PIN 5         // 1-Wire pin for temperature
#define DS18B20_PWR 19  // Power for 1-wire temperature sensor
#define DIP_SWITCH1 8   // Voltage selection 230 / 110 V AC (switch off = 230V)  - with switch off, D8 is HIGH from internal pullup [Not used]
#define DIP_SWITCH2 9   // RF node ID (off = no change in node ID, switch on = nodeID -1) with switch off, D9 is HIGH from internal pullup

#endif
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// constants calculated at compile time
#define PHASE1 0                                 // No delay for the Phase 1 voltage
#if WIRES == 3-WIRE
#define PHASE2 (NUMSAMPLES/6)                    // Delay for the Phase 2 voltage 
#define PHASE3 (NUMSAMPLES/3)                    // Delay for the Phase 3 voltage
#else
#define PHASE2 (NUMSAMPLES/3)                    // Delay for the Phase 2 voltage 
#define PHASE3 (NUMSAMPLES*2/3)                  // Delay for the Phase 3 voltage
#endif
#define BUFFERSIZE (PHASE3 + 2)                  // Store a little more than 120 / 240 degrees (3-wire/4-wire) of voltage samples

#define ADC_COUNTS (1 << ADC_BITS)               // ADC Resolution in steps
#define SAMPLERATE (360.0 / NUMSAMPLES)          // Sample Rate in degrees


#define TIMERTOP (((1000000/SUPPLY_FREQUENCY/NUMSAMPLES)*16)-1) // terminal count for PLL timer
#define PLLTIMERMAX (TIMERTOP+PLLTIMERRANGE)
#define PLLTIMERMIN (TIMERTOP-PLLTIMERRANGE)
//--------------------------------------------------------------------------------------------------
  
//--------------------------------------------------------------------------------------------------

// Dallas DS18B20 commands
#define SKIP_ROM 0xCC 
#define CONVERT_TEMPERATURE 0x44
#define READ_SCRATCHPAD 0xBE
#define UNUSED_TEMPERATURE 30000                 // this value (300C) is sent if no sensor has ever been detected
#define OUTOFRANGE_TEMPERATURE 30200             // this value (302C) is sent if the sensor reports < -55C or > +125C
#define BAD_TEMPERATURE 30400                    // this value (304C) is sent if no sensor is present or the checksum is bad (corrupted data)
#define TEMP_RANGE_LOW -5500
#define TEMP_RANGE_HIGH 12500
#define MAXONEWIRE 6                             // Max number of temperature sensors 
                                                 //  - 6 for compatibility, only one can be used.

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Pulse counting
volatile byte pulses = 0;
unsigned long pulseTime = 0;                     // Record time of interrupt pulse
const byte PulseMinPeriod = PULSEMINPERIOD;      // minimum period between pulses (ms)
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
#ifdef EMONESP
#undef SERIALPRINT                               // Must not corrupt serial output to emonHub with 'human-friendly' printout
#undef SERIALOUT
#undef DEBUGGING
#endif

#if defined SERIALOUT
#undef EMONESP
#undef SERIALPRINT                               // Must not corrupt serial output to emonHub with 'human-friendly' printout
#undef DEBUGGING
#endif
//--------------------------------------------------------------------------------------------------

#include <OneWire.h>

#if RadioFormat == RFM69_LOW_POWER_LABS
  RFM69 radio;
#endif

// create a data packet for the RFM
struct { int power1, power2, power3, power4, Vrms, temp[MAXONEWIRE] = {UNUSED_TEMPERATURE,UNUSED_TEMPERATURE,
                  UNUSED_TEMPERATURE,UNUSED_TEMPERATURE,UNUSED_TEMPERATURE,UNUSED_TEMPERATURE};
                  unsigned long pulseCount; } emontx;
 
// Intermediate constants
double v_ratio, i1_ratio, i2_ratio, i3_ratio, i4_ratio;
double i1phaseshift, i2phaseshift, i3phaseshift, i4phaseshift;

bool firstCycle = true; 

// Accumulated values over 1 cycle - shared between ISR & main program
volatile unsigned long sumVsq, sumI1sq, sumI2sq, sumI3sq, sumI4sq; 
volatile long sumVavg, sumI1avg, sumI2avg, sumI3avg, sumI4avg;
volatile long sumPower1A, sumPower1B, sumPower2A, sumPower2B, sumPower3A, sumPower3B, sumPower4A, sumPower4B; 
volatile unsigned int sumSamples;

// Accumulated values over the reporting period
uint64_t sumPeriodVsq, sumPeriodI1sq, sumPeriodI2sq, sumPeriodI3sq, sumPeriodI4sq;
int64_t sumPeriodVavg, sumPeriodI1avg, sumPeriodI2avg, sumPeriodI3avg, sumPeriodI4avg;
int64_t sumPeriodPower1A, sumPeriodPower1B, sumPeriodPower2A, sumPeriodPower2B, sumPeriodPower3A, sumPeriodPower3B, sumPeriodPower4A, sumPeriodPower4B; 
unsigned long sumPeriodSamples;

double removeRMSOffset(uint64_t sumSquared, int64_t sum, unsigned long numSamples);
double removePowerOffset(uint64_t power, int64_t sumV, int64_t sumI, unsigned long numSamples);
double x1, x2, x3, x4, y1, y2, y3, y4;  // phase shift coefficients
double applyPhaseShift(double phaseShift, double sampleRate, double A, double B);
double deg_rad(double a);

float Vrms, I1rms, I2rms, I3rms, I4rms;
long sumTimerCount;
float realPower1,apparentPower1,powerFactor1;
float realPower2,apparentPower2,powerFactor2;
float realPower3,apparentPower3,powerFactor3;
float realPower4,apparentPower4,powerFactor4;
float powerFactor12;
float frequency;
volatile word timerCount=TIMERTOP;
volatile word pllUnlocked=PLLLOCKCOUNT;
word sumCycleCount;
volatile bool newsumCycle;
unsigned long nextTransmitTime;
bool rfmXmit = false;

OneWire oneWire(W1PIN);
bool hasTemperatureSensor = false;

bool calibration_enable = false;                           // Enable on-line calibration when running. 
                                                           //  For safety, thus MUST default to false. (Required due to faulty ESP8266 software.)

static void showString (PGM_P s);

void setup()
{
  #if defined(DS18B20_PWR)
  pinMode(DS18B20_PWR, OUTPUT);
  digitalWrite(DS18B20_PWR, HIGH);
  #endif
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  #ifdef EMONTX_V34
  //READ DIP SWITCH 1 POSITION 
  pinMode(DIP_SWITCH1, INPUT_PULLUP);
  if (digitalRead(DIP_SWITCH1)==LOW) 
    EEProm.nodeID++;  //If DIP switch 1 is switched on then add 1 to the nodeID
  //READ DIP SWITCH 2 POSITION 
  pinMode(DIP_SWITCH2, INPUT_PULLUP);
  if (digitalRead(DIP_SWITCH2)==LOW) 
    EEProm.vCal = VCAL_EU;  //If DIP switch 2 is switched on then start with calibration for EU a.c. adapter
  #endif
  #ifdef SYNCPIN
  pinMode(SYNCPIN, OUTPUT);
  digitalWrite(SYNCPIN, LOW);
  #endif
  #ifdef SAMPPIN
  pinMode(SAMPPIN, OUTPUT);
  digitalWrite(SAMPPIN, LOW);
  #endif
  pinMode (RFMSELPIN, OUTPUT);
  digitalWrite(RFMSELPIN,HIGH);

  
  for (byte i=0; i<4; i++)
  {
      digitalWrite(LEDPIN, LOW); delay(200); digitalWrite(LEDPIN, HIGH); delay(200);
  }
    
 // start the SPI library:
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(0);
  SPI.setClockDivider(SPI_CLOCK_DIV8);
  // initialise RFM69
  #ifdef RFM69CW
    delay(200); // wait for RFM69CW POR
    if (EEProm.rf_on)
    #if RadioFormat == RFM69_LOW_POWER_LABS
      radio.initialize(RF69_433MHZ,EEProm.nodeID,EEProm.networkGroup);  
      radio.encrypt("89txbe4p8aik5kt3");                                                      // initialize RFM
      radio.setPowerLevel(EEProm.rfPower);
    #else
      rfm_init();                                                        // initialize RFM
    #endif
  #endif
  
  #ifdef USEPULSECOUNT
  pinMode(PULSEPIN, INPUT_PULLUP);               // Set interrupt pulse counting pin as input
  attachInterrupt(PULSEINT, onPulse, RISING);    // Attach pulse counting interrupt pulse counting
  #endif
  emontx.pulseCount=0;                           // Make sure pulse count starts at zero
    
  Serial.begin(9600);                            // Do NOT set greater than 9600
  digitalWrite(LEDPIN, LOW);   
  Serial.println(F("OpenEnergyMonitor.org"));
  #if !defined SERIALOUT && !defined EMONESP
   #ifdef EMONTX_V2
     Serial.print(F("emonTx V2"));
   #endif
   #ifdef EMONTX_V32
     Serial.print(F("emonTx V3.2"));
   #endif
   #ifdef EMONTX_V34
     Serial.print(F("emonTx V3.4"));
   #endif
   #ifdef EMONTX_SHIELD    
     Serial.print(F("emonTx Shield"));
   #endif    
   Serial.println(F(VERSION));


   #ifdef RFM69CW
     Serial.println(F("Using RFM69CW Radio"));
   #endif
   #ifdef SERIALOUT
     Serial.println(F("Using wired serial output"));
   #endif
   #ifdef EMONESP
     Serial.println(F("Using ESP8266 serial output"));
   #endif
   load_config(true);                                 // Load RF config from EEPROM (if any exists)
  #else   // #if !defined SERIALOUT && !defined EMONESP 
   load_config(false);   
  #endif  // #if !defined SERIALOUT && !defined EMONESP 
  

  #if !defined SERIALOUT && !defined EMONESP
   Serial.print(F("Network: ")); 
   Serial.println(EEProm.networkGroup);
  
   Serial.print(F("Node: ")); 
   Serial.print(EEProm.nodeID); 

   Serial.print(F(" Freq: ")); 
   if (EEProm.RF_freq == RF69_433MHZ) Serial.println(F("433MHz"));
   if (EEProm.RF_freq == RF69_868MHZ) Serial.println(F("868MHz"));
   if (EEProm.RF_freq == RF69_915MHZ) Serial.println(F("915MHz"));
  #endif  // #if !defined SERIALOUT && !defined EMONESP 
  
  calculateConstants();
  calculateTiming();

  if (isTemperatureSensor())
  {      
    hasTemperatureSensor = true;
    convertTemperature(); // start initial temperature conversion
  }

  #ifdef DEBUGGING
    Serial.println(F("Phase shift coefficients:"));
    Serial.print(F("x1 = "));Serial.print(x1);Serial.print(F("  y1 = "));Serial.println(y1);
    Serial.print(F("x2 = "));Serial.print(x2);Serial.print(F("  y2 = "));Serial.println(y2);
    Serial.print(F("x3 = "));Serial.print(x3);Serial.print(F("  y3 = "));Serial.println(y3);
    Serial.print(F("x4 = "));Serial.print(x4);Serial.print(F("  y4 = "));Serial.println(y4);
  #endif
  
  // change ADC prescaler to /64 = 250kHz clock
  // slightly out of spec of 200kEEProm.nodeIDHz but should be OK
  ADCSRA &= 0xf8;  // remove bits set by Arduino library
  ADCSRA |= 0x06; 

  //set timer 1 interrupt for required sumPeriod
  noInterrupts();
  TCCR1A = 0; // clear control registers
  TCCR1B = 0;
  TCNT1  = 0; // clear counter
  OCR1A = TIMERTOP; // set compare reg for timer sumPeriod
  bitSet(TCCR1B,WGM12); // CTC mode
  bitSet(TCCR1B,CS10); // no prescaling
  bitSet(TIMSK1,OCIE1A); // enable timer 1 compare interrupt
  bitSet(ADCSRA,ADIE); // enable ADC interrupt
  interrupts();

  nextTransmitTime=millis() + EEProm.period*1000 + (EEProm.nodeID * 20);
}

void loop()
{
  getSettings();

  
  if(newsumCycle && !firstCycle)
    addsumCycle(); // a new mains sumCycle has been sampled
  firstCycle = false;
  
  if((millis()>=nextTransmitTime) && ((millis()-nextTransmitTime)<0x80000000L)) // check for overflow
  {
    #ifndef LEDISLOCK
      digitalWrite(LEDPIN,HIGH);
    #else
      digitalWrite(LEDPIN,LOW);
    #endif
    calculateVIPF();

    if (hasTemperatureSensor && EEProm.temp_enable)
        emontx.temp[0]=readTemperature();
    
    if (pulses)                                      // if the ISR has counted some pulses, update the total count
    {
        cli();                                       // Disable interrupt just in case a pulse comes in while we are updating the count
        emontx.pulseCount += pulses;
        pulses = 0;
        sei();                                       // Re-enable interrupts
    }

    sendResults();
    if (EEProm.temp_enable)
      convertTemperature(); // start next conversion
    nextTransmitTime+=EEProm.period*1000;
    #ifndef LEDISLOCK
      digitalWrite(LEDPIN,LOW);
    #else
      digitalWrite(LEDPIN,HIGH);
    #endif
  }
}

// timer 1 interrupt handler
ISR(TIMER1_COMPA_vect)
{
  #ifdef SAMPPIN
  digitalWrite(SAMPPIN,HIGH);
  #endif
  ADMUX = _BV(REFS0) | CT1PIN; // start ADC conversion for first current
  ADCSRA |= _BV(ADSC);
  #ifdef SAMPPIN
  digitalWrite(SAMPPIN,LOW);
  #endif
}

// ADC interrupt handler
ISR(ADC_vect)
{
  static int newV, lastV, sampleI1, sampleI2, sampleI3, sampleI4;
  static int storedV[BUFFERSIZE];  // Array to store >240 degrees of voltage samples
  int result;
  static int Vindex = 0;
  
  #ifdef SAMPPIN
  digitalWrite(SAMPPIN,HIGH);
  #endif
  result = ADCL;
  result |= ADCH<<8;
  // remove the nominal offset 
  result -=(ADC_COUNTS >> 1);
  // determine which conversion just completed
  switch(ADMUX & 0x0f)
  {
    case CT1PIN:
      ADMUX = _BV(REFS0) | CT2PIN; // start CT2 conversion
      ADCSRA |= _BV(ADSC);
      sampleI1 = result;
      sumI1sq += (long)sampleI1 * sampleI1;
      sumI1avg += sampleI1;
      break;
    case CT2PIN:
      ADMUX = _BV(REFS0) | CT3PIN; // start CT3 conversion
      ADCSRA |= _BV(ADSC);
      sampleI2 = result;
      sumI2sq += (long)sampleI2 * sampleI2;
      sumI2avg += sampleI2; 
     break;
    case CT3PIN:
    #ifdef CT4Phase
      ADMUX = _BV(REFS0) | CT4PIN; // start CT4 conversion
    #else
      ADMUX = _BV(REFS0) | VOLTSPIN; // start Voltage conversion        
    #endif
      ADCSRA |= _BV(ADSC);
      sampleI3 = result;
      sumI3sq += (long)sampleI3 * sampleI3;
      sumI3avg += sampleI3; 
      break;
    #ifdef CT4Phase
    case CT4PIN:
      ADMUX = _BV(REFS0) | VOLTSPIN; // start Voltage conversion
      ADCSRA |= _BV(ADSC);
      sampleI4 = result;
      sumI4sq += (long)sampleI4 * sampleI4;
      sumI4avg += sampleI4; 
      break;
    #endif
    case VOLTSPIN:
      lastV=newV;
      newV = result;
      storedV[Vindex] = newV;        // store this voltage sample in circular buffer
      sumVsq += ((long)newV * newV);
      sumVavg += newV; 

      sumPower1A += (long)storedV[(Vindex+BUFFERSIZE-CT1Phase)%BUFFERSIZE] * sampleI1;  // Use stored & delayed voltage for power calculation CT 1
      sumPower1B += (long)storedV[(Vindex+BUFFERSIZE-CT1Phase-1)%BUFFERSIZE] * sampleI1;

      sumPower2A += (long)storedV[(Vindex+BUFFERSIZE-CT2Phase)%BUFFERSIZE] * sampleI2;  // Use stored & delayed voltage for power calculation CT 2
      sumPower2B += (long)storedV[(Vindex+BUFFERSIZE-CT2Phase-1)%BUFFERSIZE] * sampleI2;

      sumPower3A += (long)storedV[(Vindex+BUFFERSIZE-CT3Phase)%BUFFERSIZE] * sampleI3;  // Use stored & delayed voltage for power calculation CT 3
      sumPower3B += (long)storedV[(Vindex+BUFFERSIZE-CT3Phase-1)%BUFFERSIZE] * sampleI3;

#ifdef CT4Phase
      sumPower4A += (long)storedV[(Vindex+BUFFERSIZE-CT4Phase)%BUFFERSIZE] * sampleI4;  // Use stored & delayed voltage for power calculation CT 4
      sumPower4B += (long)storedV[(Vindex+BUFFERSIZE-CT4Phase-1)%BUFFERSIZE] * sampleI4;
#endif      
      sumSamples++;
      updatePLL(newV,lastV);
      ++Vindex %= BUFFERSIZE;    
      break;
      
  }
  #ifdef SAMPPIN
  digitalWrite(SAMPPIN,LOW);
  #endif

}

void updatePLL(int newV, int lastV)
{
  static byte samples=0;
  static int oldV;
  bool rising;

  rising=(newV>lastV); // synchronise to rising zero crossing
  
  samples++;
  if(samples>=NUMSAMPLES) // end of one 50Hz sumCycle
  {
    #ifdef SYNCPIN
    digitalWrite(SYNCPIN,HIGH);
    #endif
    samples=0;
    if(rising)
    {
      // if we're in the rising part of the 50Hz sumCycle adjust the final timer count
      // to move newV towards 0, only adjust if we're moving in the wrong direction
      if(((newV<0)&&(newV<=oldV))||((newV>0)&&(newV>=oldV))) timerCount-=newV;
      // limit range of PLL frequency
      timerCount=constrain(timerCount,PLLTIMERMIN,PLLTIMERMAX);
      OCR1A=timerCount;
      if(abs(newV)>PLLUNLOCKRANGE) pllUnlocked=PLLLOCKCOUNT; // we're unlocked
      else if(pllUnlocked) pllUnlocked--;
      #ifdef LEDISLOCK
        digitalWrite(LEDPIN,pllUnlocked?LOW:HIGH);
      #endif
    }
    else // in the falling part of the sumCycle, we shouldn't be here
    {
      OCR1A=PLLTIMERMAX; // shift out of this region fast
      pllUnlocked=PLLLOCKCOUNT; // and we can't be locked
    }
    
    oldV=newV;
    
    newsumCycle=true; // flag new sumCycle to outer loop
  }
  else if(samples==(NUMSAMPLES/2))
  {
    // negative zero crossing
    #ifdef SYNCPIN
    digitalWrite(SYNCPIN,LOW);
    #endif
  }
  #ifdef SAMPPIN
  digitalWrite(SAMPPIN,LOW);
  #endif
}

// add data for new 50Hz sumCycle to total for the period (called from loop() )
void addsumCycle()
{
  // save results for outer loop
  noInterrupts();
  sumPeriodVsq      += sumVsq;
  sumPeriodVavg     += sumVavg;
  sumPeriodI1sq     += sumI1sq;
  sumPeriodI1avg    += sumI1avg; 
  sumPeriodI2sq     += sumI2sq;
  sumPeriodI2avg    += sumI2avg; 
  sumPeriodI3sq     += sumI3sq;
  sumPeriodI3avg    += sumI3avg; 
  sumPeriodI4sq     += sumI4sq;
  sumPeriodI4avg    += sumI4avg; 

  sumPeriodPower1A  += sumPower1A;
  sumPeriodPower1B  += sumPower1B;
  sumPeriodPower2A  += sumPower2A;
  sumPeriodPower2B  += sumPower2B;
  sumPeriodPower3A  += sumPower3A;
  sumPeriodPower3B  += sumPower3B;
  sumPeriodPower4A  += sumPower4A;
  sumPeriodPower4B  += sumPower4B;
  sumPeriodSamples  += sumSamples;

  sumVsq	    = 0;
  sumVavg	    = 0;
  sumI1sq	    = 0;
  sumI1avg	    = 0; 
  sumI2sq	    = 0;
  sumI2avg	    = 0; 
  sumI3sq	    = 0;
  sumI3avg	    = 0; 
  sumI4sq       = 0;
  sumI4avg	    = 0; 
      
  sumPower1A	= 0;
  sumPower1B	= 0;
  sumPower2A	= 0;
  sumPower2B	= 0;
  sumPower3A	= 0;
  sumPower3B	= 0;
  sumPower4A	= 0;
  sumPower4B	= 0;
  sumSamples	= 0;

  sumTimerCount+=(timerCount+1); // for average frequency calculation
  sumCycleCount++;
  newsumCycle=false;
  interrupts();
}


double removeRMSOffset(uint64_t sumSquared, int64_t sum, unsigned long numSamples)
{
    double x = ((double)sumSquared / numSamples) - ((double)sum * sum / numSamples / numSamples);
    return (x<0.0 ? 0.0 : sqrt(x));
}

double removePowerOffset(int64_t power, int64_t sumV, int64_t sumI, unsigned long numSamples)
{
    return (((double)power / numSamples) - ((double)sumV * sumI / numSamples / numSamples));
}

double deg_rad(double a)
{
    return (0.01745329*a);
}

double applyPhaseShift(double phaseShift, double sampleRate, double A, double B)
{
  double y = sin(deg_rad(phaseShift)) / sin(deg_rad(sampleRate));
  double x = cos(deg_rad(phaseShift)) - y * cos(deg_rad(sampleRate));
  return (A * x + B * y); 
}


// calculate voltage, current, power and frequency
void calculateVIPF()
{  
  if(sumPeriodSamples==0) return; // just in case
  
  frequency=((float)sumCycleCount*16000000)/(((float)sumTimerCount)*NUMSAMPLES);

  // rms values - voltage & current
  
  // Vrms still contains the fine voltage offset. Correct this by subtracting the "Offset V^2" before the sq. root.
  Vrms = v_ratio * removeRMSOffset(sumPeriodVsq, sumPeriodVavg, sumPeriodSamples);
   
  // Similarly the 4 currents
  I1rms = i1_ratio * removeRMSOffset(sumPeriodI1sq, sumPeriodI1avg, sumPeriodSamples);
  I2rms = i2_ratio * removeRMSOffset(sumPeriodI2sq, sumPeriodI2avg, sumPeriodSamples);
  I3rms = i3_ratio * removeRMSOffset(sumPeriodI3sq, sumPeriodI3avg, sumPeriodSamples);
  #ifdef CT4Phase
  I4rms = i4_ratio * removeRMSOffset(sumPeriodI4sq, sumPeriodI4avg, sumPeriodSamples);
  #endif

  // Power contains both voltage & current offsets. Correct this by subtracting the "Offset Power": Vavg * Iavg.
  // Apply timing/phase compensation to obtain real power.
  //  real power = Ical * Vcal * (powerA * PHASECAL - powerB * (PHASECAL - 1));
  // or more accurately:
  //  y = sin(phase_shift) / sin(sampleRate);
  //  x = cos(phase_shift) - y * cos(sampleRate);
  // realPower = Ical * Vcal * (powerA * x + powerB * y);
  // [sampleRate] is the angle between sample sets in radians - and is different for 
  //   50 Hz and 60 Hz systems
  // [phase_shift] will vary according to the time delay between the current and voltage samples
  //   as well as the difference in phase leads of the two transformers.
  // x & y have been calculated in setup() as they won't change.

  realPower1 = v_ratio * i1_ratio * (x1 * removePowerOffset(sumPeriodPower1A, sumPeriodVavg, sumPeriodI1avg, sumPeriodSamples) 
                                   + y1 * removePowerOffset(sumPeriodPower1B, sumPeriodVavg, sumPeriodI1avg, sumPeriodSamples));
  apparentPower1 = I1rms * Vrms; 
  if (apparentPower1 > 0.1)        // suppress "nan" values
    powerFactor1 = realPower1 / apparentPower1;
  else
    powerFactor1 = 0.0;
  
  realPower2 = v_ratio * i2_ratio * (x2 * removePowerOffset(sumPeriodPower2A, sumPeriodVavg, sumPeriodI2avg, sumPeriodSamples)
                                   + y2 * removePowerOffset(sumPeriodPower2B, sumPeriodVavg, sumPeriodI2avg, sumPeriodSamples));
  apparentPower2 = I2rms * Vrms; 
  if (apparentPower2 > 0.1) 
    powerFactor2 = realPower2 / apparentPower2;
  else
    powerFactor2 = 0.0;
  realPower3 = v_ratio * i3_ratio * (x3 * removePowerOffset(sumPeriodPower3A, sumPeriodVavg, sumPeriodI3avg, sumPeriodSamples)
                                   + y3 * removePowerOffset(sumPeriodPower3B, sumPeriodVavg, sumPeriodI3avg, sumPeriodSamples));
  apparentPower3 = I3rms * Vrms; 
  if (apparentPower3 > 0.1) 
    powerFactor3 = realPower3 / apparentPower3;
  else
    powerFactor3 = 0.0;
  #ifdef CT4Phase
  realPower4 = v_ratio * i4_ratio * (x4 * removePowerOffset(sumPeriodPower4A, sumPeriodVavg, sumPeriodI4avg, sumPeriodSamples)
                                    +y4 * removePowerOffset(sumPeriodPower4B, sumPeriodVavg, sumPeriodI4avg, sumPeriodSamples));
  apparentPower4 = I4rms * Vrms; 
  if (apparentPower4 > 0.1) 
    powerFactor4 = realPower4 / apparentPower4;
  else
    powerFactor4 = 0.0;
  #endif
  
  if (apparentPower1 > 0.1 && apparentPower2 > 0.1 ) // suppress "nan" values
    powerFactor12 = (realPower1 + realPower2) / (apparentPower1 + apparentPower2);
  else
    powerFactor12 = 0.0;

  #if WIRES == 3-WIRE
  emontx.power1=(int)(realPower1+0.5) + (int)(realPower2+0.5);
  emontx.power2=0;
  #else
  emontx.power1=(int)(realPower1+0.5);
  emontx.power2=(int)(realPower2+0.5);
  #endif
  emontx.power3=(int)(realPower3+0.5);
  emontx.power4=(int)(realPower4+0.5);
  emontx.Vrms=(int)(Vrms*100+0.5);

  sumPeriodVsq      = 0;
  sumPeriodVavg     = 0;
  sumPeriodI1sq     = 0;
  sumPeriodI1avg    = 0;
  sumPeriodI2sq     = 0;
  sumPeriodI2avg    = 0;
  sumPeriodI3sq     = 0;
  sumPeriodI3avg    = 0;
  sumPeriodI4sq     = 0;
  sumPeriodI4avg    = 0;

  sumPeriodPower1A  = 0;
  sumPeriodPower1B  = 0;
  sumPeriodPower2A  = 0;
  sumPeriodPower2B  = 0;
  sumPeriodPower3A  = 0;
  sumPeriodPower3B  = 0;
  sumPeriodPower4A  = 0;
  sumPeriodPower4B  = 0;
  sumPeriodSamples  = 0;

  sumCycleCount=0;
  sumTimerCount=0;

}

void calculateConstants(void)
{
  // Intermediate calculations

  v_ratio = EEProm.vCal  * SUPPLY_VOLTS / 1024; 
  i1_ratio = EEProm.i1Cal  * SUPPLY_VOLTS / 1024;
  i2_ratio = EEProm.i2Cal  * SUPPLY_VOLTS / 1024;
  i3_ratio = EEProm.i3Cal  * SUPPLY_VOLTS / 1024; 
  i4_ratio = EEProm.i4Cal  * SUPPLY_VOLTS / 1024; 

  #ifdef CT4Phase
    i1phaseshift = (4 * ADC_RATE * 3.6e-4 * SUPPLY_FREQUENCY - EEProm.i1Lead); // in degrees
    i2phaseshift = (3 * ADC_RATE * 3.6e-4 * SUPPLY_FREQUENCY - EEProm.i2Lead);
    i3phaseshift = (2 * ADC_RATE * 3.6e-4 * SUPPLY_FREQUENCY - EEProm.i3Lead);
    i4phaseshift = (1 * ADC_RATE * 3.6e-4 * SUPPLY_FREQUENCY - EEProm.i4Lead);
  #else
    i1phaseshift = (3 * ADC_RATE * 3.6e-4 * SUPPLY_FREQUENCY - EEProm.i1Lead); // in degrees
    i2phaseshift = (2 * ADC_RATE * 3.6e-4 * SUPPLY_FREQUENCY - EEProm.i2Lead);
    i3phaseshift = (1 * ADC_RATE * 3.6e-4 * SUPPLY_FREQUENCY - EEProm.i3Lead);
  #endif    

}

void calculateTiming(void)
{
  // Pre-calculate the constants for phase/timing correction
  y1 = sin(deg_rad(i1phaseshift)) / sin(deg_rad(SAMPLERATE));
  x1 = cos(deg_rad(i1phaseshift)) - y1 * cos(deg_rad(SAMPLERATE));
  y2 = sin(deg_rad(i2phaseshift)) / sin(deg_rad(SAMPLERATE));
  x2 = cos(deg_rad(i2phaseshift)) - y2 * cos(deg_rad(SAMPLERATE));
  y3 = sin(deg_rad(i3phaseshift)) / sin(deg_rad(SAMPLERATE));
  x3 = cos(deg_rad(i3phaseshift)) - y3 * cos(deg_rad(SAMPLERATE));
  #ifdef CT4Phase
    y4 = sin(deg_rad(i4phaseshift)) / sin(deg_rad(SAMPLERATE));
    x4 = cos(deg_rad(i4phaseshift)) - y4 * cos(deg_rad(SAMPLERATE));
  #endif
}


void sendResults()
{
  #ifdef RFM69CW                                 // *SEND RF DATA*
    if (EEProm.rf_on)
      #if RadioFormat == RFM69_LOW_POWER_LABS
        radio.sendWithRetry(5, (const void*)(&emontx), sizeof(emontx));
        radio.sleep();
      #else   
        rfm_send((byte *)&emontx, sizeof(emontx), EEProm.networkGroup, EEProm.nodeID, EEProm.RF_freq, EEProm.rfPower, busyThreshold, busyTimeout);
      #endif
  #else
    #ifdef TXPIN
      digitalWrite(TXPIN,HIGH);
      delay(5);
      digitalWrite(TXPIN,LOW);
    #endif
  #endif

  #if defined SERIALOUT && !defined EMONESP
    Serial.print(EEProm.nodeID); Serial.print(F(' '));
    #if WIRES == 3-WIRE
    Serial.print((int)(realPower1+0.5) + (int)(realPower2+0.5));   // These for compatibility, but whatever you need if emonHub is configured to suit. 
    Serial.print(F(" 0.0 "));
    #else
    Serial.print((int)(realPower1+0.5)); Serial.print(F(" "));   // These for compatibility, but whatever you need if emonHub is configured to suit. 
    Serial.print((int)(realPower2+0.5)); Serial.print(F(" "));
    #endif
    Serial.print((int)(realPower3+0.5)); Serial.print(F(" "));
    Serial.print((int)(realPower4+0.5)); Serial.print(F(" "));
    Serial.print((int)(Vrms*100));
    Serial.print(F(" "));
   
    for(byte j=0;j<MAXONEWIRE;j++)
    {
    Serial.print(emontx.temp[j]);
    Serial.print(F(" "));
    }
    Serial.println(emontx.pulseCount);

  #endif  // if defined SERIALOUT && !defined EMONESP

  #if defined EMONESP && !defined SERIALOUT
    #if WIRES == 3-WIRE
    Serial.print(F("ct1:")); Serial.print(realPower1+realPower2);            // These for compatibility, but whatever you need if the receiver is configured to suit. 
    Serial.print(F(",ct2:0.0"));
    #else
    Serial.print(F("ct1:")); Serial.print(realPower1);
    Serial.print(F(",ct2:")); Serial.print(realPower2);
    #endif    
    Serial.print(F(",ct3:")); Serial.print(realPower3);
    Serial.print(F(",ct4:")); Serial.print(realPower4);
    Serial.print(F(",vrms:")); Serial.print(Vrms);

    
    if (EEProm.temp_enable)
    {
      for(byte j=0;j<MAXONEWIRE;j++)
      {
        Serial.print(F(",t")); Serial.print(j+1); Serial.print(F(":"));
        Serial.print(emontx.temp[j]/100.0);
      }
    }
    Serial.print(F(",pulses:"));Serial.print(emontx.pulseCount);
    Serial.println();
    delay(50);
  #endif

  
  #if defined SERIALPRINT && !defined EMONESP
    Serial.print(Vrms);
    Serial.print(F(" "));
    Serial.print(I1rms,3);
    Serial.print(F(" "));
    Serial.print(I2rms,3);
    Serial.print(F(" "));
    Serial.print(I3rms,3);
    Serial.print(F(" "));
    Serial.print(I4rms,3);
    Serial.print(F(" "));
    #if WIRES == 3-WIRE
    Serial.print(realPower1 + realPower2);
    Serial.print(F(" 0.0 "));
    #else
    Serial.print(realPower1);
    Serial.print(F(" "));
    Serial.print(realPower2);
    Serial.print(F(" "));
    #endif
    Serial.print(realPower3);
    Serial.print(F(" "));
    Serial.print(realPower4);
    Serial.print(F(" "));

    Serial.print(frequency,3);
    Serial.print(F(" "));

    #if WIRES == 3-WIRE
    Serial.print(powerFactor12,4);
    Serial.print(F(" 0.0 "));
    #else
    Serial.print(powerFactor1,4);
    Serial.print(F(" "));
    Serial.print(powerFactor2,4);
    Serial.print(F(" "));
    #endif
    Serial.print(powerFactor3,4);
    Serial.print(F(" "));
    Serial.print(powerFactor4,4);
    Serial.print(F(" "));
    if (EEProm.temp_enable)
      Serial.print((float)emontx.temp[0]/100);

    #ifdef USEPULSECOUNT
      Serial.print(F(" Pulses=")); Serial.print(emontx.pulseCount);
    #endif
    
    Serial.print(F(" "));
    if(pllUnlocked) Serial.print(F(" PLL is unlocked "));
    else Serial.print(F(" PLL is locked "));
    Serial.println();
  #endif
  
  if (EEProm.showCurrents)
  {
    // to show voltage, current & power factor for calibration:
    Serial.print(F("Vrms:")); Serial.print(emontx.Vrms*0.01); 
    Serial.print(F(",I1:")); Serial.print(I1rms);
    Serial.print(F(",I2:")); Serial.print(I2rms);
    Serial.print(F(",I3:")); Serial.print(I3rms);
    Serial.print(F(",I4:")); Serial.print(I4rms);

    Serial.print(F(",pf1:")); Serial.print(powerFactor1,4);
    Serial.print(F(",pf2:")); Serial.print(powerFactor2,4);
    Serial.print(F(",pf3:")); Serial.print(powerFactor3,4);
    Serial.print(F(",pf4:")); Serial.println(powerFactor4,4);
  }

}

bool isTemperatureSensor(void)
{
  return oneWire.reset();
}

void convertTemperature()
{
  oneWire.reset();
  oneWire.write(SKIP_ROM);
  oneWire.write(CONVERT_TEMPERATURE);
}

int readTemperature()
{
  byte buf[9];
  int result;
  
  if (oneWire.reset())
  {
      oneWire.write(SKIP_ROM);
      oneWire.write(READ_SCRATCHPAD);
      for(int i=0; i<9; i++) buf[i]=oneWire.read();
      if(oneWire.crc8(buf,8)!=buf[8])
        return BAD_TEMPERATURE;
      result=(buf[1]<<8)|buf[0];
        // result is temperature x16, multiply by 6.25 to convert to temperature x100
      result=(result*6)+(result>>2);
      if (result <= TEMP_RANGE_LOW || result >= TEMP_RANGE_HIGH)
        return OUTOFRANGE_TEMPERATURE;     // return value ('Out of range')
      return result;
  }
  return BAD_TEMPERATURE;
}
/*
    Temp fault codes: BAD_TEMPERATURE          = Faulty sensor, sensor broken or disconnected.
                      UNUSED_TEMPERATURE       = Sensor has never been detected since power-up/reset. 
                      OUTOFRANGE_TEMPERATURE   = Sensor returned an out-of-range value. 
                      85  deg                    although in range, might indicate a wiring fault.
*/
                      


#ifdef USEPULSECOUNT
//-------------------------------------------------------------------------------------------------------------------------------------------
// The Interrupt Service Routine - runs each time a falling edge of a pulse is detected
//-------------------------------------------------------------------------------------------------------------------------------------------
void onPulse()                  
{
  if (EEProm.pulse_enable)
  {
    if (EEProm.pulse_period)
    {
      if ((millis() - pulseTime) > EEProm.pulse_period) {  // Check that contact bounce has finished
        pulses++;
      }
      pulseTime=millis();
    }
    else                                                   // No 'debounce' required - electronic switch presumed 	
      pulses++;
  }    
}

#endif
