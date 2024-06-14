#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
#include "Adafruit_SGP40.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>

#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

  // For the breakout board, you can use any 2 or 3 pins.
  // These pins will also work for the 1.8" TFT shield.
#define TFT_CS        10
#define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         8

// Color definitions
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF
#define GREY     0x8C51

// 55, 80
GFXcanvas1 canvas(55, 80);

Adafruit_AM2320 am2320 = Adafruit_AM2320();
Adafruit_SGP40 sgp;

// For 1.14", 1.3", 1.54", 1.69", and 2.0" TFT with ST7789:
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// SGP40 vars
uint16_t sgpRaw;
int32_t vocIndex;

// am2320 vars
float temp;
float humid;

bool updateDot = false;

uint16_t lastTempStatus = 0;
uint16_t lastHumiStatus = 0;
uint16_t lastVocsStatus = 0;

float tempArr[4];
float humidArr[4];
float vocsArr[4];

// Index tracking for 1 hour arrays
int pc = 0;
int inArr = 0;

float avgTemp;
float avgHumid;
float avgVocs;

uint16_t currentTempStatus;
uint16_t currentHumiStatus;
uint16_t currentVocsStatus;

float lastavgTemp = 0;
float lastavgHumid = 0;
float lastavgVocs = 0;


void getStatus(float t, float h, int v) {
  // status dots: checking their new values
  // temp limits: Y 21 - 23, G 23 - 28, Y 28-30
  if(t >= 23 && t <= 28) {
    currentTempStatus = GREEN;
  } else if((t > 21 && t < 23) || (t > 28 && t < 30)) {
    currentTempStatus = YELLOW;
  } else {
    currentTempStatus = RED;
  }

  // humid limits G 0 - 35, Y 35 - 50
  if(h <= 35) {
    currentHumiStatus = GREEN;
  } else if(h < 50) {
    currentHumiStatus = YELLOW;
  } else {
    currentHumiStatus = RED;
  }

  // VOC range: step = 100 G => GY => Y => Orange => R
  if(v <= 100) {
    currentVocsStatus = GREEN;
  } else if(v <= 200) {
    currentVocsStatus = 0xDF83;
  } else if(v <= 300) {
    currentVocsStatus = YELLOW;
  } else if(v <= 400) {
    currentVocsStatus = 0xFC20;
  } else {
    currentVocsStatus = RED;
  }
}


// If the status dots changes colour, update them
void updateStatus(float lt, float lh, int lv, int xRec, int yRec, uint16_t ct, uint16_t ch, uint16_t cv) {
  if(ct != lt) {
    // tft.fillCircle(xRec, 10, 8, ct);
    tft.fillRoundRect(xRec, yRec, 60, 20, 3, ct);
  }
  if(ch != lh) {
    //tft.fillCircle(xRec, 60, 8, ch);
    tft.fillRoundRect(xRec, yRec + 28, 60, 20, 3, ch);
  }
  if(cv != lv) {
    //tft.fillCircle(xRec, 70, 8, cv);
    tft.fillRoundRect(xRec, yRec + 56, 60, 20, 3, cv);
  }
}


void updateScreen() {
  
  // reading values
  canvas.fillScreen(BLACK);
  canvas.setCursor(0, 17);
  canvas.println(temp, 1);
  canvas.println(humid, 1);
  canvas.println(vocIndex);
  tft.drawBitmap(130, 0, canvas.getBuffer(), canvas.width(), canvas.height(), WHITE, BLACK);

  // update dot (pulse to show sensors are still refreshing)
  if(updateDot) {
    tft.fillCircle(310, 10, 6, BLACK);
    updateDot = false;
  } else {
    tft.fillCircle(310, 10, 6, WHITE);
    updateDot = true;
  }

  getStatus(temp, humid, vocIndex);
  updateStatus(lastTempStatus, lastHumiStatus, lastVocsStatus, 230, 1, currentTempStatus, currentHumiStatus, currentVocsStatus);

  // Cycle counter, check for 15 minutes average sampling time 
  pc += 1;
  if(pc >= 450) {
    tempArr[inArr] = temp;
    humidArr[inArr] = humid;
    vocsArr[inArr] = vocIndex;
    inArr += 1;
    pc = 0;

    // update hour average
    avgTemp = (tempArr[0] + tempArr[1] + tempArr[2] + tempArr[3])/4;
    avgHumid = (humidArr[0] + humidArr[1] + humidArr[2] + humidArr[3])/4;
    avgVocs = (vocsArr[0] + vocsArr[1] + vocsArr[2] + vocsArr[3])/4;

    canvas.fillScreen(BLACK);
    canvas.setCursor(0, 17);
    canvas.println(avgTemp, 1);
    canvas.println(avgHumid, 1);
    canvas.println(avgVocs, 1);
    tft.drawBitmap(130, 117, canvas.getBuffer(), canvas.width(), canvas.height(), WHITE, BLACK);

    getStatus(avgTemp, avgHumid, avgVocs);
    updateStatus(lastavgTemp, lastavgHumid, lastavgVocs, 230, 115, currentTempStatus, currentHumiStatus, currentVocsStatus);
    
    lastavgTemp = avgTemp;
    lastavgHumid = avgHumid;
    lastavgVocs = avgVocs;

    // reset "average" arrays index
    if(inArr >= 4) {
      inArr = 0;
    }
  }
}


void setup() {
  Serial.begin(115200);

  tft.init(240, 320);  // init ST7789 2.0" 320x240 TFT
  tft.setRotation(3);  // set display rotation

  // screen setup
  tft.fillScreen(BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.setFont(&FreeSans12pt7b);

  while (!Serial) {
    Serial.print("Serial not connected");
    tft.println("Serial not connected, retrying in 1 second");
    delay(1000); // hang out until serial port opens
  }
  
  // start am2320 temp+humid sensor
  if (! am2320.begin()){
    Serial.println("am2320 sensor not found :(");
    tft.setTextColor(RED);
    tft.println("am2320 sensor not found :(");
    while (! am2320.begin()) {
      Serial.println("Retrying to connect am2320");
      tft.println("Retrying to connect am2320");
      delay(2000);
    }
  }
  tft.setTextColor(GREEN);
  Serial.println("am2320 started");
  tft.println("am2320 started");

  // start sgp40 VOC sensor
  if (! sgp.begin()){
    tft.setTextColor(RED);
    Serial.println("SGP40 sensor not found :(");
    tft.println("SGP40 sensor not found :(");
    while (! sgp.begin()) {
      Serial.println("Retrying to connect SGP40");
      tft.println("Retrying to connect SGP40");
      delay(2000);
    }
  }
  tft.setTextColor(GREEN);
  Serial.println("SGP40 started");
  tft.println("SGP40 started");

  delay(700);
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);

  canvas.setFont(&FreeSans12pt7b);
  canvas.setTextWrap(false);

  // set texts
  tft.setCursor(0, 17);
  tft.println("Temp     : ");
  tft.println("Humidity : ");
  tft.println("VOC index: ");
  tft.print("\n");
  tft.println("Avg temp : ");
  tft.println("Avg Humid: ");
  tft.println("Avg VOCs : ");

  tft.setCursor(185, 17);
  tft.print("C");
  tft.setCursor(185, 47);
  tft.print("\%");

  // print average readings
  canvas.fillScreen(BLACK);
  canvas.setCursor(0, 17);
  canvas.println(avgTemp, 1);
  canvas.println(avgHumid, 1);
  canvas.println(avgVocs, 1);
  tft.drawBitmap(130, 117, canvas.getBuffer(), canvas.width(), canvas.height(), WHITE, BLACK);

  // display texts for avg readings
  tft.setCursor(185, 134);
  tft.print("C");
  tft.setCursor(185, 164);
  tft.print("\%");

  // Display status bars for avg readings
  getStatus(25, 30, 50);
  updateStatus(20, 29, 49, 230, 115, 21, 28, 48);
}


void loop() {
  temp  = am2320.readTemperature();
  humid = am2320.readHumidity(); 

  /* 
   * Get VOC index 
   * VOC index can directly indicate the condition of air quality. The larger the value, the worse the air quality
   *    0-100，  Excellent          , no need to ventilate
   *    100-200，Good               , no need to ventilate
   *    200-300，Lightly polluted   , ventilate suggested
   *    300-400，Moderately polluted, increase ventilate with clean air
   *    400-500, Heavily polluted   , Optimize ventilation
   * Return VOC index, range: 0-500
   */
  sgpRaw = sgp.measureRaw(temp, humid);
  vocIndex = sgp.measureVocIndex(temp, humid);
  Serial.print("temp: ");  Serial.print(temp);  Serial.print("\t\t");  Serial.print("humid: ");  Serial.print(humid);  Serial.print("\t\t");  Serial.print("vocs: ");  Serial.println(vocIndex);
  updateScreen();

  delay(2000);
}
