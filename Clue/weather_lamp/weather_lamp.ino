/*
 * Weather Lights
 * Matthew E. Nelson
 * Revised - 5/14/2020
 * SW Rev - 0.1
 * HW Rev - 1.0
 * 
 * Disclaimer
 * Some portions of this code was derived from or copied from various
 * Adafruit and Sparkfun libraries and examples.  Please support them
 * by visiting their website. 
 * 
 * SW description
 * The software will read from several sensors on the Adafruit Clue board
 * including temperature, humidity, pressure, and light.  It will also 
 * read from the AS3935 Sparkfun breakout board to detect lightining events.
 * Information will then be displayed as color represenation on the neopixel
 * and on the Adafruit Clue display.  Information will also be available via
 * the Bluetooth chip provided on the Clue.
 * 
 * HW Description
 * - Adafruit Clue board
 *  - BMP280 Temp/Pressure
 *  - SHT Humidity
 *  - APDS9960 Light sensor 
 * - Neopixel (1x 12 count and 1x 24 count rings)
 * - Sparkfun AS3935 Breakout board
 */

 
 // INCLUDES
 // ---------------------------------------------------
 #include <Adafruit_Arcada.h>
#include <Adafruit_SPIFlash.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM6DS33.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_APDS9960.h>
#include <Adafruit_BMP280.h>
#include <PDM.h>
#include <SPI.h>
#include <Wire.h>
#include "SparkFun_AS3935.h"

// DEFINES
// -----------------------------------------------------
// Defines for the AS3935
#define INDOOR 0x12 
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01

// SPI
SparkFun_AS3935 lightning;

// Interrupt pin for lightning detection 
const int lightningInt = 4; 
// Chip select pin 
int spiCS = 10; 

// Values for modifying the IC's settings. All of these values are set to their
// default values. 
byte noiseFloor = 2;
byte watchDogVal = 2;
byte spike = 2;
byte lightningThresh = 1; 

// This variable holds the number representing the lightning or non-lightning
// event issued by the lightning detector. 
byte intVal = 0; 

// Defines for the Adafruit Clue

#define WHITE_LED 43

Adafruit_Arcada arcada;
Adafruit_LSM6DS33 lsm6ds33;
Adafruit_LIS3MDL lis3mdl;
Adafruit_SHT31 sht30;
Adafruit_APDS9960 apds9960;
Adafruit_BMP280 bmp280;
extern PDMClass PDM;
extern Adafruit_FlashTransport_QSPI flashTransport;
extern Adafruit_SPIFlash Arcada_QSPI_Flash;

uint32_t buttons, last_buttons;
uint8_t j = 0;  // neopixel counter for rainbow

// Check the timer callback, this function is called every millisecond!
volatile uint16_t milliseconds = 0;
void timercallback() {
  analogWrite(LED_BUILTIN, milliseconds);  // pulse the LED
  if (milliseconds == 0) {
    milliseconds = 255;
  } else {
    milliseconds--;
  }
}

void setup() {

  // Setup Serial for debugging
  Serial.begin(115200); 
  // Delay for debugging, allows time to turn on the serial display
  delay(5000);
  Serial.println("Weather lantern Operating System");
  Serial.println("OS VER - 1.0");
  Serial.println("Booting up...");
  Serial.println("Initializing Hardware...");

  // Configure Adafruit Clue

  // enable NFC pins  
  if ((NRF_UICR->NFCPINS & UICR_NFCPINS_PROTECT_Msk) == (UICR_NFCPINS_PROTECT_NFC << UICR_NFCPINS_PROTECT_Pos)){
    Serial.println("Fix NFC pins");
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
    NRF_UICR->NFCPINS &= ~UICR_NFCPINS_PROTECT_Msk;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
    Serial.println("Done");
    NVIC_SystemReset();
  }

  pinMode(WHITE_LED, OUTPUT);
  digitalWrite(WHITE_LED, LOW);

  Serial.println("Hello! Arcada Clue test");
  if (!arcada.arcadaBegin()) {
    Serial.print("Failed to begin");
    while (1);
  }
  arcada.displayBegin();
  Serial.println("Arcada display begin");

  for (int i=0; i<250; i+=10) {
    arcada.setBacklight(i);
    delay(1);
  }

  arcada.display->setCursor(0, 0);
  arcada.display->setTextWrap(true);
  arcada.display->setTextSize(2);
  
  /********** Check MIC */
  PDM.onReceive(onPDMdata);
  if (!PDM.begin(1, 16000)) {
    Serial.println("**Failed to start PDM!");
  }
  
  /********** Check QSPI manually */
  if (!Arcada_QSPI_Flash.begin()){
    Serial.println("Could not find flash on QSPI bus!");
    arcada.display->setTextColor(ARCADA_RED);
    arcada.display->println("QSPI Flash FAIL");
  } else {
    uint32_t jedec;
    jedec = Arcada_QSPI_Flash.getJEDECID();
    Serial.print("JEDEC ID: 0x"); Serial.println(jedec, HEX);
    arcada.display->setTextColor(ARCADA_GREEN);
    arcada.display->print("QSPI JEDEC: 0x"); arcada.display->println(jedec, HEX);
  }
  
   /********** Check filesystem next */
  if (!arcada.filesysBegin()) {
    Serial.println("Failed to load filesys");
    arcada.display->setTextColor(ARCADA_YELLOW);
    arcada.display->println("Filesystem not found");
  } else {
    Serial.println("Filesys OK!");
    arcada.display->setTextColor(ARCADA_GREEN);
    arcada.display->println("Filesystem OK");
  }

  arcada.display->setTextColor(ARCADA_WHITE);
  arcada.display->println("Sensors Found: ");

  /********** Check APDS */
  if (!apds9960.begin()) {
    Serial.println("No APDS9960 found");
    arcada.display->setTextColor(ARCADA_RED);
  } else {
    Serial.println("**APDS9960 OK!");
    arcada.display->setTextColor(ARCADA_GREEN);
    apds9960.enableColor(true);
  }
  arcada.display->print("APDS9960 ");

  /********** Check LSM6DS33 */
  if (!lsm6ds33.begin_I2C()) {
    Serial.println("No LSM6DS33 found");
    arcada.display->setTextColor(ARCADA_RED);
  } else {
    Serial.println("**LSM6DS33 OK!");
    arcada.display->setTextColor(ARCADA_GREEN);
  }
  arcada.display->println("LSM6DS33 ");
  
  /********** Check LIS3MDL */
  if (!lis3mdl.begin_I2C()) {
    Serial.println("No LIS3MDL found");
    arcada.display->setTextColor(ARCADA_RED);
  } else {
    Serial.println("**LIS3MDL OK!");
    arcada.display->setTextColor(ARCADA_GREEN);
  }
  arcada.display->print("LIS3MDL ");

  /********** Check SHT3x */
  if (!sht30.begin(0x44)) {
    Serial.println("No SHT30 found");
    arcada.display->setTextColor(ARCADA_RED);
  } else {
    Serial.println("**SHT30 OK!");
    arcada.display->setTextColor(ARCADA_GREEN);
  }
  arcada.display->print("SHT30 ");

  /********** Check BMP280 */
  if (!bmp280.begin()) {
    Serial.println("No BMP280 found");
    arcada.display->setTextColor(ARCADA_RED);
  } else {
    Serial.println("**BMP280 OK!");
    arcada.display->setTextColor(ARCADA_GREEN);
  }
  arcada.display->println("BMP280");

  buttons = last_buttons = 0;
  arcada.timerCallback(1000, timercallback);
  arcada.display->setTextWrap(false);
  

  // When lightning is detected the interrupt pin goes HIGH.
  pinMode(lightningInt, INPUT); 

  Serial.print("Initializing AS3935 Franklin Lightning Detector..."); 

   SPI.begin(); // For SPI
  if( !lightning.beginSPI(spiCS, 2000000) ) { 
    Serial.println ("FAILED");
    Serial.println ("Lightning Detector did not start up, freezing!"); 
    while(1); 
  }
  else
    Serial.println("OK");

  // "Disturbers" are events that are false lightning events. If you find
  // yourself seeing a lot of disturbers you can have the chip not report those
  // events on the interrupt lines. 

  lightning.maskDisturber(true); 

  int maskVal = lightning.readMaskDisturber();
  Serial.println("Configuring AS3935.");
  Serial.print("Are disturbers being masked... "); 
  if (maskVal == 1)
    Serial.println("YES"); 
  else if (maskVal == 0)
    Serial.println("NO"); 

  // The lightning detector defaults to an indoor setting (less
  // gain/sensitivity), if you plan on using this outdoors 
  // uncomment the following line:

  lightning.setIndoorOutdoor(OUTDOOR); 

  int enviVal = lightning.readIndoorOutdoor();
  Serial.print("AS3935 Mode... ");  
  if( enviVal == INDOOR )
    Serial.println("Indoor.");  
  else if( enviVal == OUTDOOR )
    Serial.println("Outdoor.");  
  else 
    Serial.println(enviVal, BIN); 

  // Noise floor setting from 1-7, one being the lowest. Default setting is
  // two. If you need to check the setting, the corresponding function for
  // reading the function follows.    

  lightning.setNoiseLevel(noiseFloor);  

  int noiseVal = lightning.readNoiseLevel();
  Serial.print("Noise Level is set at: ");
  Serial.println(noiseVal);

  // Watchdog threshold setting can be from 1-10, one being the lowest. Default setting is
  // two. If you need to check the setting, the corresponding function for
  // reading the function follows.    

  lightning.watchdogThreshold(watchDogVal); 

  int watchVal = lightning.readWatchdogThreshold();
  Serial.print("Watchdog Threshold is set to: ");
  Serial.println(watchVal);

  // Spike Rejection setting from 1-11, one being the lowest. Default setting is
  // two. If you need to check the setting, the corresponding function for
  // reading the function follows.    
  // The shape of the spike is analyzed during the chip's
  // validation routine. You can round this spike at the cost of sensitivity to
  // distant events. 

  lightning.spikeRejection(spike); 

  int spikeVal = lightning.readSpikeRejection();
  Serial.print("Spike Rejection is set to: ");
  Serial.println(spikeVal);


  // This setting will change when the lightning detector issues an interrupt.
  // For example you will only get an interrupt after five lightning strikes
  // instead of one. Default is one, and it takes settings of 1, 5, 9 and 16.   
  // Followed by its corresponding read function. Default is zero. 

  lightning.lightningThreshold(lightningThresh); 

  uint8_t lightVal = lightning.readLightningThreshold();
  Serial.print("The number of strikes before interrupt is triggerd: "); 
  Serial.println(lightVal); 

  // When the distance to the storm is estimated, it takes into account other
  // lightning that was sensed in the past 15 minutes. If you want to reset
  // time, then you can call this function. 

  //lightning.clearStatistics(); 

  // The power down function has a BIG "gotcha". When you wake up the board
  // after power down, the internal oscillators will be recalibrated. They are
  // recalibrated according to the resonance frequency of the antenna - which
  // should be around 500kHz. It's highly recommended that you calibrate your
  // antenna before using these two functions, or you run the risk of schewing
  // the timing of the chip. 

  //lightning.powerDown(); 
  //delay(1000);
  //if( lightning.wakeUp() ) 
   // Serial.println("Successfully woken up!");  
  //else 
    //Serial.println("Error recalibrating internal osciallator on wake up."); 
  
  // Set too many features? Reset them all with the following function.
  // lightning.resetSettings();

  
  
}

void loop() {

  if(digitalRead(lightningInt) == HIGH){
    // Hardware has alerted us to an event, now we read the interrupt register
    // to see exactly what it is. 
    intVal = lightning.readInterruptReg();
    if(intVal == NOISE_INT){
      Serial.println("Noise."); 
    }
    else if(intVal == DISTURBER_INT){
      Serial.println("Disturber."); 
    }
    else if(intVal == LIGHTNING_INT){
      Serial.println("Lightning Strike Detected!"); 
      // Lightning! Now how far away is it? Distance estimation takes into
      // account previously seen events. 
      byte distance = lightning.distanceToStorm(); 
      Serial.print("Approximately: "); 
      Serial.print(distance); 
      Serial.println("km away!"); 

      // "Lightning Energy" and I do place into quotes intentionally, is a pure
      // number that does not have any physical meaning. 
      long lightEnergy = lightning.lightningEnergy(); 
      Serial.print("Lightning Energy: "); 
      Serial.println(lightEnergy); 
      digitalWrite(WHITE_LED, HIGH);

    }
  }
  
  arcada.display->setTextColor(ARCADA_WHITE, ARCADA_BLACK);
  arcada.display->setCursor(0, 100);
  
  arcada.display->print("Temp: ");
  arcada.display->print(bmp280.readTemperature());
  arcada.display->print(" C");
  arcada.display->println("         ");
  
  arcada.display->print("Baro: ");
  arcada.display->print(bmp280.readPressure()/100);
  arcada.display->print(" hPa");
  arcada.display->println("         ");
  
  arcada.display->print("Humid: ");
  arcada.display->print(sht30.readHumidity());
  arcada.display->print(" %");
  arcada.display->println("         ");

  uint16_t r, g, b, c;
  //wait for color data to be ready
  while(! apds9960.colorDataReady()) {
    delay(5);
  }
  apds9960.getColorData(&r, &g, &b, &c);
  arcada.display->print("Light: ");
  arcada.display->print(c);
  arcada.display->println("         ");

  sensors_event_t accel, gyro, mag, temp;
  lsm6ds33.getEvent(&accel, &gyro, &temp);
  lis3mdl.getEvent(&mag);
  arcada.display->print("Accel:");
  arcada.display->print(accel.acceleration.x, 1);
  arcada.display->print(",");
  arcada.display->print(accel.acceleration.y, 1);
  arcada.display->print(",");
  arcada.display->print(accel.acceleration.z, 1);
  arcada.display->println("         ");

  arcada.display->print("Gyro:");
  arcada.display->print(gyro.gyro.x, 1);
  arcada.display->print(",");
  arcada.display->print(gyro.gyro.y, 1);
  arcada.display->print(",");
  arcada.display->print(gyro.gyro.z, 1);
  arcada.display->println("         ");
  
  arcada.display->print("Mag:");
  arcada.display->print(mag.magnetic.x, 1);
  arcada.display->print(",");
  arcada.display->print(mag.magnetic.y, 1);
  arcada.display->print(",");
  arcada.display->print(mag.magnetic.z, 1);
  arcada.display->println("         ");

  uint32_t pdm_vol = getPDMwave(256);
  Serial.print("PDM volume: "); Serial.println(pdm_vol);
  arcada.display->print("Mic: ");
  arcada.display->print(pdm_vol);
  arcada.display->println("      ");
    
  Serial.printf("Drawing %d NeoPixels\n", arcada.pixels.numPixels());  
  for(int32_t i=0; i< arcada.pixels.numPixels(); i++) {
     arcada.pixels.setPixelColor(i, Wheel(((i * 256 / arcada.pixels.numPixels()) + j*5) & 255));
  }
  arcada.pixels.show();
  j++;

  uint8_t pressed_buttons = arcada.readButtons();
  
  if (pressed_buttons & ARCADA_BUTTONMASK_A) {
    Serial.println("BUTTON A");
    tone(ARCADA_AUDIO_OUT, 4000, 100);
  } else {
    tone(ARCADA_AUDIO_OUT, 0);
  }
  if (pressed_buttons & ARCADA_BUTTONMASK_B) {
    Serial.println("BUTTON B");
    digitalWrite(WHITE_LED, HIGH);
  } else {
    digitalWrite(WHITE_LED, LOW);
  }
  last_buttons = buttons;
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return arcada.pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return arcada.pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return arcada.pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


/*****************************************************************/

int16_t minwave, maxwave;
short sampleBuffer[256];// buffer to read samples into, each sample is 16-bits
volatile int samplesRead;// number of samples read

int32_t getPDMwave(int32_t samples) {
  minwave = 30000;
  maxwave = -30000;
  
  while (samples > 0) {
    if (!samplesRead) {
      yield();
      continue;
    }
    for (int i = 0; i < samplesRead; i++) {
      minwave = min(sampleBuffer[i], minwave);
      maxwave = max(sampleBuffer[i], maxwave);
      //Serial.println(sampleBuffer[i]);
      samples--;
    }
    // clear the read count
    samplesRead = 0;
  }
  return maxwave-minwave;  
}


void onPDMdata() {
  // query the number of bytes available
  int bytesAvailable = PDM.available();

  // read into the sample buffer
  PDM.read(sampleBuffer, bytesAvailable);

  // 16-bit, 2 bytes per sample
  samplesRead = bytesAvailable / 2;
}
