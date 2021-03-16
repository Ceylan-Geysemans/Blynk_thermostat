#define BLYNK_PRINT Serial
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <RotaryEncoder.h>
#include <Ticker.h>

#define SEALEVELPRESSURE_HPA (1013.25)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define PIN_A   D5 //ky-040 clk pin, interrupt & add 100nF/0.1uF capacitors between pin & ground!!!
#define PIN_B   D6 //ky-040 dt  pin,             add 100nF/0.1uF capacitors between pin & ground!!!
#define BUTTON  D7 //ky-040 sw  pin, interrupt & add 100nF/0.1uF capacitors between pin & ground!!!

int overheat = 28; //overheat beveileging

int16_t positie = 0;
int blynkset = 0;

RotaryEncoder encoder(PIN_A, PIN_B, BUTTON);

Ticker encoderRotary;
Ticker encoderButton;

#define heater D0

WidgetLED led1(V1);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Adafruit_BME280 bme;

char auth[] = "Blynk token";

char ssid[] = "wifi ssid";
char pass[] = "wifi password";

float temperature, humidity, pressure, altitude, setval;

BlynkTimer timer;

void encoderISR()
{
  encoder.readAB();
}

void encoderButtonISR()
{
  encoder.readPushButton();
}

void setup()
{
  Blynk.virtualWrite(V10, 0);
  encoder.begin();                                                   //set encoders pins as input & enable built-in pullup resistors

  encoderRotary.attach_ms(10, encoderISR);                           //call encoderISR()       every 10 milliseconds/0.010 seconds
  encoderButton.attach_ms(15, encoderButtonISR);                     //call encoderButtonISR() every 15 milliseconds/0.015 seconds

  Serial.begin(9600);
  pinMode(heater, OUTPUT);
  Blynk.begin(auth, ssid, pass, "server.wyns.it", 8081);
  //Temp sens setup
  bme.begin(0x76);
  //display setup
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  delay(2000);
  display.clearDisplay();

  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  timer.setInterval(1000L, givetoblynk);
}

void loop()
{

  Blynk.run();
  timer.run(); // Initiates BlynkTimer
  meausure();
  printval();
  givetoblynk();
  thermostat();
  getencoder();

}

void meausure() {
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
}

void printval() {
  display.clearDisplay();
  display.setTextSize(2.5);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Tem: ");
  display.print(temperature);
  display.setCursor(0, 18);
  display.print("Set: ");
  display.println(setval);
  display.display();
}

void givetoblynk() {
  Blynk.virtualWrite(V4, millis() / 1000);
  Blynk.virtualWrite(V5, setval); //sending to Blynk instelwaarde
  Blynk.virtualWrite(V6, temperature); //sending to Blynk temperatuur
  Blynk.virtualWrite(V7, humidity); //sending to Blynk vochtigheid
  Blynk.virtualWrite(V8, pressure); //sending to Blynk druk
  Blynk.virtualWrite(V9, altitude); //sending to Blynk hoogte

}

void thermostat() {
  if (setval >= temperature) {
    digitalWrite(heater, HIGH);
    led1.on();
  }
  else {
    digitalWrite(heater, LOW);
    led1.off();
  }
  if ( temperature > overheat) {
    display.clearDisplay();
    display.setTextSize(2.5);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Over");
    display.setCursor(0, 18);
    display.print("heat!");
    display.display();
  }
}

void getencoder() {
  if (positie != encoder.getPosition())
  {
    positie = blynkset;
    positie = encoder.getPosition();

  if (positie < 1) {
    positie = 0;
  }

    setval = positie;
    Blynk.virtualWrite(V10, setval);
  }

  if (encoder.getPushButton() == true) Serial.println(F("PRESSED")); //(F()) save string to flash & keep dynamic memory free
}

BLYNK_WRITE(V10)
{
  blynkset = param.asInt();
  if (blynkset < 1) {
    blynkset = 0;
  }
  encoder.setPosition(blynkset);
  positie = blynkset;
  setval = blynkset;
}

