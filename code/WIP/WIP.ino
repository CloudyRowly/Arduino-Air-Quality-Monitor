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

float tempArr[60];
float humidArr[60];
float vocsArr[60];

float temp1min[30];
float humid1min[30];
float voc1min[30];

// Index tracking for 1 hour arrays
int ti = 0;

// Index tracking for 1 min arrays
int t1i = 0;

float avgTemp;
float avgHumid;
float avgVocs;

void updateScreen() {
  uint16_t currentTempStatus;
  uint16_t currentHumiStatus;
  uint16_t currentVocsStatus;
  
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

  // status dots: checking their new values
  // temp limits: Y 21 - 23, G 23 - 28, Y 28-30
  if(temp >= 23 && temp <= 28) {
    currentTempStatus = GREEN;
  } else if((temp > 21 && temp < 23) || (temp > 28 && temp < 30)) {
    currentTempStatus = YELLOW;
  } else {
    currentTempStatus = RED;
  }

  // humid limits G 0 - 35, Y 35 - 50
  if(humid <= 35) {
    currentHumiStatus = GREEN;
  } else if(humid < 50) {
    currentHumiStatus = YELLOW;
  } else {
    currentHumiStatus = RED;
  }

  // VOC range: step = 100 G => GY => Y => Orange => R
  if(vocIndex <= 100) {
    currentVocsStatus = GREEN;
  } else if(vocIndex <= 200) {
    currentVocsStatus = 0xDF83;
  } else if(vocIndex <= 300) {
    currentVocsStatus = YELLOW;
  } else if(vocIndex <= 400) {
    currentVocsStatus = 0xFC20;
  } else {
    currentVocsStatus = RED;
  }

  int xRec = 230;
  // If the status dots changes colour, update them
  if(currentTempStatus != lastTempStatus) {
    // tft.fillCircle(xRec, 10, 8, currentTempStatus);
    tft.fillRoundRect(xRec, 2, 40, 16, 5, currentTempStatus);
  }
  if(currentHumiStatus != lastHumiStatus) {
    //tft.fillCircle(xRec, 40, 8, currentHumiStatus);
    tft.fillRoundRect(xRec, 32, 40, 16, 5, currentHumiStatus);
  }
  if(currentVocsStatus != lastVocsStatus) {
    //tft.fillCircle(xRec, 70, 8, currentVocsStatus);
    tft.fillRoundRect(xRec, 62, 40, 16, 5, currentVocsStatus);
  }

/**
  // avg reading 1 min
  temp1min[t1i] = temp;
  humid1min[t1i] = humid;
  voc1min[t1i] = vocIndex;

  t1i += 1;

  // write minute array into hour array
  if(t1i >= 30) {
    float tmpSum = 0;
    for(float n : temp1min) {
      tmpSum += n;
    }

    tmpSum = tmpSum / 30;

    tempArr[ti] = tmpSum;

    tmpSum = 0;
    for(float n : humid1min) {
      tmpSum += n;
    }

    tmpSum = tmpSum / 30;

    humidArr[ti] = tmpSum;

    tmpSum = 0;
    for(float n : voc1min) {
      tmpSum += n;
    }

    tmpSum = tmpSum / 30;

    vocsArr[ti] = tmpSum;
  
    ti += 1;
    t1i = 0;
  }

  avgVocs = 0;
  for(float n : vocsArr) {
    avgVocs += n;
  }
  avgVocs = avgVocs / 60;

  avgTemp = 0;
  for(float n : vocsArr) {
    avgTemp += n;
  }
  avgTemp = avgTemp / 60;
  Serial.println(avgTemp);

  avgHumid = 0;
  for(float n : vocsArr) {
    avgHumid += n;
  }
  avgHumid = avgHumid / 60;

  if(ti >= 60) {
    ti = 0;
  }
*/


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
  
  updateScreen();

  delay(2000);
}





// int x0, int y0, int x1, int y1, float minX, float maxX, float minY, float maxY, int stepX, int stepY
// createGraph(0, 85, 315, 135, 0, 280, 15, 30, 14, 3, RED);
// createGraph(0, 137, 315, 187, 0, 280, 0, 100, 14, 5, BLUE);
// createGraph(0, 189, 315, 239, 0, 280, 0, 400, 14, 4, MAGENTA);


/** Create graph template (WIP)
 * x0, y0: coordinate of upper left corner of graph
 * x1, y1: coordinate of lower right corner of graph
 * 
**/
void createGraph(int x0, int y0, int x1, int y1, float minX, float maxX, float minY, float maxY, int stepX, int stepY, uint16_t colour) {
  int xIndent = 20;
  int yIndent = 10;
  tft.setTextSize(1);
  tft.setFont();

  // draw x division lines
  for(int i = 1; i <= stepX + 1; i++) {
    int currentX = xIndent + (((x1 - x0 - xIndent + 1)/(stepX)) * i);
    tft.drawLine(currentX, y0, currentX, y1, GREY);
  }

  // draw y division lines
  for(int i = 1; i < stepY; i++) {
    int currentY = y0 + (y1 - y0 + 1)/stepY * i;
    tft.drawLine(x0 + xIndent, currentY, x1, currentY, GREY);
  }

  // draw axis
  tft.drawLine(x0 + xIndent, y0, x0 + xIndent, y1, colour);
  tft.drawLine(x0 + xIndent, y1, x1, y1, colour);

  // showing axis values
  tft.setCursor(x0, y0);
  tft.print(maxY, 0);
  tft.setCursor(x0, y1 - yIndent);
  tft.print(minY, 0);

  tft.setCursor(x0 + xIndent, y1 - yIndent);
  tft.print(minX, 0);
  tft.setCursor(x1 - xIndent, y1 - yIndent);
  tft.print(maxX, 0);
}

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
