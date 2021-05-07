#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif

//RH_ASK driver;
// RH_ASK driver(2000, 4, 5, 0); // ESP8266 or ESP32: do not use pin 11 or 2
RH_ASK driver(2000, 3, 4, 0); // ATTiny, RX on D3 (pin 2 on attiny85) TX on D4 (pin 3 on attiny85),

int ID = 02;
int start = 0;
int alarm = 0;

void setup()
{
#ifdef RH_HAVE_SERIAL
  SerialUSB.begin(9600);    // Debugging only
#endif
  if (!driver.init())
#ifdef RH_HAVE_SERIAL
    SerialUSB.println("init failed");
#else
    ;
#endif
  delay(1000);
  start = 10000 + ID;
  alarm = 20000 + ID;
  char msg[5];
  itoa(start, msg, 10);
  SerialUSB.println(atoi(msg));
  driver.send((uint8_t *)msg, strlen(msg));
  pinMode(7, INPUT_PULLDOWN);
  pinMode(0, INPUT_PULLDOWN);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  delay(5000);
}

void loop()
{
    char msg[5];
  if (digitalRead(7)) {

    itoa(alarm, msg, 10);
    driver.send((uint8_t *)msg, strlen(msg));
    driver.waitPacketSent();
    delay(200);
  }
  if (digitalRead(0)) {
    itoa(start, msg, 10);
    for (int i = 0; i < 10; i ++){
    driver.send((uint8_t *)msg, strlen(msg));
    driver.waitPacketSent();
    delay(100);
    }
  }
  delay(200);
}
