#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include "ccs811.h"

//ESP32
#define TFT_CS         5
#define TFT_RST        4                                           
#define TFT_DC         2
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define si7021Addr 0x40

Adafruit_BMP085 bmp;

CCS811 ccs811(23);
 
float val1, val2;

void setup() {
  Wire.begin();
  Serial.begin(9600);
  Wire.beginTransmission(si7021Addr);
  Wire.endTransmission();

  if (!bmp.begin()) {
	Serial.println("Could not find a valid BMP085/BMP180 sensor, check wiring!");
	while (1) {}
  }

  // Enable CCS811
  ccs811.set_i2cdelay(50); // Needed for ESP8266 because it doesn't handle I2C clock stretch correctly
  bool ok = ccs811.begin();
  if ( !ok ) Serial.println("setup: CCS811 begin FAILED");
 
  // Print CCS811 versions
  Serial.print("setup: hardware    version: "); Serial.println(ccs811.hardware_version(), HEX);
  Serial.print("setup: bootloader  version: "); Serial.println(ccs811.bootloader_version(), HEX);
  Serial.print("setup: application version: "); Serial.println(ccs811.application_version(), HEX);
 
  // Start measuring
  ok = ccs811.start(CCS811_MODE_1SEC);
  if ( !ok ) Serial.println("setup: CCS811 start FAILED");

  SPI.begin();

  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(2);
  tft.setTextSize(2);
  //tft.drawRect(0,0,158,126,ST7735_WHITE);
  //tft.drawRect(2,2,154,122,ST7735_RED);
}

void loop() {
  // Si7021 Sensor
  unsigned int data[2];
   
  Wire.beginTransmission(si7021Addr);
  //Send humidity measurement command
  Wire.write(0xF5);
  Wire.endTransmission();
  delay(500);
     
  // Request 2 bytes of data
  Wire.requestFrom(si7021Addr, 2);
  // Read 2 bytes of data to get humidity
  if(Wire.available() == 2)
  {
    data[0] = Wire.read();
    data[1] = Wire.read();
  }
     
  // Convert the data
  float humidity  = ((data[0] * 256.0) + data[1]);
  humidity = ((125 * humidity) / 65536.0) - 6;
 
  Wire.beginTransmission(si7021Addr);
  // Send temperature measurement command
  Wire.write(0xF3);
  Wire.endTransmission();
  delay(500);
     
  // Request 2 bytes of data
  Wire.requestFrom(si7021Addr, 2);
   
  // Read 2 bytes of data for temperature
  if(Wire.available() == 2)
  {
    data[0] = Wire.read();
    data[1] = Wire.read();
  }
 
  // Convert the data
  float temp  = ((data[0] * 256.0) + data[1]);
  float celsTemp = ((175.72 * temp) / 65536.0) - 46.85;
  float fahrTemp = celsTemp * 1.8 + 32;
    
  // Output data to serial monitor
  Serial.print("Humidity : ");
  Serial.print(humidity);
  Serial.println(" % RH");
  Serial.print("Celsius : ");
  Serial.print(celsTemp);
  Serial.println(" C");
  Serial.print("Fahrenheit : ");
  Serial.print(fahrTemp);
  Serial.println(" F");

  // BMP180 Sensor
  Serial.print("Temperature = ");
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");
    
  Serial.print("Pressure = ");
  Serial.print(bmp.readPressure());
  Serial.println(" Pa");
    
  // Calculate altitude assuming 'standard' barometric
  // pressure of 1013.25 millibar = 101325 Pascal
  Serial.print("Altitude = ");
  Serial.print(bmp.readAltitude());
  Serial.println(" meters");

  Serial.print("Pressure at sealevel (calculated) = ");
  Serial.print(bmp.readSealevelPressure());
  Serial.println(" Pa");

  // you can get a more precise measurement of altitude
  // if you know the current sea level pressure which will
  // vary with weather and such. If it is 1015 millibars
  // that is equal to 101500 Pascals.
  Serial.print("Real altitude = ");
  Serial.print(bmp.readAltitude(102000));
  Serial.println(" meters");
    
  Serial.println();

  //CCS811 Sensor
  // Read
  uint16_t eco2, etvoc, errstat, raw;
  ccs811.read(&eco2, &etvoc, &errstat, &raw);
 
  // Print measurement results based on status
  if ( errstat == CCS811_ERRSTAT_OK )
  {
    val1 = eco2;
    val2 = etvoc;
 
    Serial.print("CCS811: ");
    Serial.print("eco2=");
    Serial.print(val1);
    Serial.print(" ppm  ");
 
    Serial.print("etvoc=");
    Serial.print(val2);
    Serial.print(" ppb  ");
    Serial.println();
  }
  else if ( errstat == CCS811_ERRSTAT_OK_NODATA )
  {
    Serial.println("CCS811: waiting for (new) data");
  } else if ( errstat & CCS811_ERRSTAT_I2CFAIL )
  {
    Serial.println("CCS811: I2C error");
  }
  else
  {
    Serial.print("CCS811: errstat=");
    Serial.print(errstat, HEX);
    Serial.print("=");
    Serial.println( ccs811.errstat_str(errstat) );
  }

  // TFT LCD Si7021 Humidity
  tft.setTextSize(1);
  tft.setTextColor(ST7735_RED);
  tft.setCursor(5, 5);
  tft.print("Luftfuktighet:");

  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(5, 15);
  tft.print(humidity);

  tft.setTextSize(1);
  tft.setTextColor(ST7735_BLUE);
  tft.setCursor(40, 15);
  tft.print("%");

  // TFT LCD Si7021 Temperature
  tft.setTextSize(1);
  tft.setTextColor(ST7735_RED);
  tft.setCursor(5, 25);
  tft.print("Temperatur:");

  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(5, 35);
  tft.print(celsTemp);

  tft.setTextSize(1);
  tft.setTextColor(ST7735_BLUE);
  tft.setCursor(40, 35);
  tft.print("C");

  // TFT LCD BMP180 Pressure
  tft.setTextSize(1);
  tft.setTextColor(ST7735_RED);
  tft.setCursor(5, 45);
  tft.print("Lufttryck");

  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(5, 55);
  tft.print(bmp.readPressure());

  tft.setTextSize(1);
  tft.setTextColor(ST7735_BLUE);
  tft.setCursor(40, 55);
  tft.print("hPa");

  // TFT LCD BMP180 Temperature
  tft.setTextSize(1);
  tft.setTextColor(ST7735_RED);
  tft.setCursor(5, 65);
  tft.print("Temperatur:");

  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(5, 75);
  tft.print(bmp.readTemperature());

  tft.setTextSize(1);
  tft.setTextColor(ST7735_BLUE);
  tft.setCursor(40, 75);
  tft.print("C");

  // TFT LCD BMP180 Altitude
  tft.setTextSize(1);
  tft.setTextColor(ST7735_RED);
  tft.setCursor(5, 85);
  tft.print("Altitud:");

  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(5, 95);
  tft.print(bmp.readAltitude());

  tft.setTextSize(1);
  tft.setTextColor(ST7735_BLUE);
  tft.setCursor(45, 95);
  tft.print("meter");

  // TFT LCD CCS811 CO2
  tft.setTextSize(1);
  tft.setTextColor(ST7735_RED);
  tft.setCursor(5, 105);
  tft.print("CO2");

  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(5, 115);
  tft.print(val1);

  tft.setTextSize(1);
  tft.setTextColor(ST7735_BLUE);
  tft.setCursor(50, 115);
  tft.print("ppm");

  // TFT LCD CCS811 VOC
  tft.setTextSize(1);
  tft.setTextColor(ST7735_RED);
  tft.setCursor(5, 125);
  tft.print("VOC");

  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(5, 135);
  tft.print(val2);

  tft.setTextSize(1);
  tft.setTextColor(ST7735_BLUE);
  tft.setCursor(45, 135);
  tft.print("ppb");

  //tft.drawFastHLine(0, 37, tft.width(), ST7735_WHITE);
  //tft.drawFastHLine(0, 77, tft.width(), ST7735_WHITE);
  //tft.drawFastVLine(60, 0, 77, ST7735_WHITE);

  delay(5000);

  tft.fillScreen(ST7735_BLACK);
}