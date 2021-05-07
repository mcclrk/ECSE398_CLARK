#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RH_ASK.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int F_value; // LDR sensor final value

#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif

// RH_ASK driver;
// RH_ASK driver(2000, 4, 5, 0); // ESP8266 or ESP32: do not use pin 11 or 2
RH_ASK driver(2000, 3, 4, 0); // ATTiny, RX on D3 (pin 2 on attiny85) TX on D4 (pin 3 on attiny85),
uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
uint8_t buflen = sizeof(buf);
int system_size;
int password_input;

int active_sensors[1000][2];

void setup()
{
  pinMode(7, INPUT_PULLUP); //rotary encoder button
  pinMode(8, INPUT_PULLUP); //roatary encoder direction 1
  pinMode(9, INPUT_PULLUP); //rotary encoder direction 2
#ifdef RH_HAVE_SERIAL
  Serial.begin(9600);    // Debugging only
#endif
  if (!driver.init())
#ifdef RH_HAVE_SERIAL
    Serial.println("init failed");
#else
    ;
#endif
  system_size = 0;
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //init display
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.display(); //updates display from memory
  delay(1000);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("INIT"); //start-up message
  display.display(); //update
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.display(); //update
  password_input = 0;
}

void loop()
{
  display.setTextSize(1);
  display.setCursor(0, 32);
  display.print("       SCANNING");
  display.display();
  if (driver.recv(buf, &buflen)) // Non-blocking
  {
    int incoming = atoi((char *)buf); //convert incoming to integer
    SerialUSB.println(incoming);
    if (incoming > 20000) { //if alarm message recieved
      if (listening(incoming - 20000)) { //if ID is listed in hub
        alarm(incoming - 20000); //set off alarm
      }
    }
    else if (incoming > 10000) { //if message recieved is an init message (sensor turned on, broadcasting ID)
      if (!listening(incoming - 10000) ) { //if hub is NOT already listening for ID
        add_to_list(incoming - 10000); //add ID to list of sensors
      }
    }
  }
}

bool listening(int ID) {
  bool out = false;
  for (int i = 0; i < system_size; i++) {
    if (active_sensors[i][0] == ID) { //check memory for sensor ID
      out = true; //if ID is in memory, output true
    }
  }
  return out;
}

void add_to_list(int ID) {
  if (display_new_sensor(ID)) { //ask user to arm sensor by pressing rotary button
    active_sensors[system_size][0] = ID; //if armed, add sensor to memory list
    system_size = system_size + 1; //increase size of system by 1
    home_screen(); //return to home screen after update
  } else {

  }
}

bool display_new_sensor(int ID) {
  bool out = false;
  int wait = 0;
  clear_buf(); //clear RF input buffer
  while (wait <= 15) { //wait 15 seconds for confirmation
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(" ID Recieved: ");
    display.print(ID);
    display.setCursor(0, 16);
    display.print(" Accept? ");
    display.print(15 - wait);
    display.display();
    wait = wait + 1;
    if (!digitalRead(9)) { //if rotary button is pushed, return true
      clear_buf();
      return true;
    }
    delay(1000);
  }
  display.clearDisplay(); //if rotary button is not pressed in 15 seconds, return false
  home_screen();
  return out;
}

void home_screen() {
  display.clearDisplay();
  int x = 8; //x location on OLED
  for (int i = 0; i < system_size; i ++) {
    display.setCursor(x, 0);
    if (active_sensors[i][1] == 0) { //if alarm is currently NOT on
      display.print("-");
    } else { //if alarm is currently ON
      display.print("!");
      display.setCursor(0, 32);
      display.print("     ALARM ON: ");
      display.print(password_input);
      display.display();
    }
    display.setCursor(x, 16);
    display.print(active_sensors[i][0]); //print ID
    x = x + 8;
    display.display();
  }
}

int rotary() {
  while (!digitalRead(7) || !digitalRead(8)) { //is rotary encoder turned?

  }
  if (!digitalRead(7)) { //turn left
    return 2;
  }
  if (!digitalRead(8)) { //turn right
    return 1;
  }
  return 0;

}


void alarm(int ID) {
  password_input = 0; //input with rotary encoder
  display.clearDisplay();
  bool a = false;
  int cur = 0;
  for (int i = 0; i < system_size; i ++) {
    if (active_sensors[i][0] == ID) { //find ID matching input
      active_sensors[i][1] = 1; //set alarm state to ON
      home_screen(); //update home screen
      a = true; //alarm is active
    }
  }
  while (a) {
    tone(6,262);
    update_input(); //update password input with rotary encoder
    home_screen(); //update home screen to show password input
    if (!digitalRead(9)) { //if button is pressed
      if (password_input == 1) { //if password is correct
        a = false; //set alarm to OFF
        reset_alarm(); //reset all alarm states in memory
      }
    }
  }

}

void reset_alarm() { //reset all alarm states
  for (int i = 0; i < system_size; i ++) {
    active_sensors[i][1] = 0;
    clear_buf();
  }
  tone(6,0);
  home_screen(); //update home screen

}

void clear_buf(){ //clear message from RF buffer

      driver.recv(buf, &buflen);
    for (int i = 0; i < buflen; i ++){
      buf[i] = 0;
    }
}

void update_input() {
  int cng = rotary();
  if (cng == 1) {
    password_input = password_input + 1; //increment password
  } else if (cng == 2) {
    password_input = password_input - 1; //decrement password
  }
}
