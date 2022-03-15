#include <WiFi.h> //Connect to WiFi Network
#include <SPI.h>
#include <TFT_eSPI.h>
#include <mpu6050_esp32.h>
#include<math.h>
#include<string.h>
#include <ArduinoJson.h>

TFT_eSPI tft = TFT_eSPI();
const int SCREEN_HEIGHT = 160;
const int SCREEN_WIDTH = 128;
const int LOOP_PERIOD = 40;

char network[] = "MIT";  //SSID for 6.08 Lab`
char password[] = ""; //Password for 6.08 Lab

//Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const uint16_t IN_BUFFER_SIZE = 3500; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char old_response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
const uint16_t JSON_BODY_SIZE = 3000;
char request[IN_BUFFER_SIZE];
char json_body[JSON_BODY_SIZE];

uint32_t primary_timer;

int old_val;

//buttons for today 
uint8_t BUTTON1 = 45; // Get question: get new question
uint8_t BUTTON2 = 39; // True
uint8_t BUTTON3 = 38; // False
uint8_t BUTTON4 = 34; // Reset: end game and send score to server

/* Global variables*/
uint8_t state; //used for containing button state and detecting edges
int old_state; //used for detecting button edges
uint8_t reset_state; //used for containing button state and detecting edges
int old_reset_state; //used for detecting button edges

uint32_t last_entry_time = 0;
uint16_t entry_timeout = 1500;
uint32_t last_set_time = 0; //POST mode: setting update type
uint16_t set_timeout = 2000;

//enum for button states
enum button_state {S0, S1, S2, S3, S4};

/* CONSTANTS */
//Prefix to POST request:
char server[] = "608dev-2.net";
const char user[] = "spiderman"; //user

//LCD variables
char output[100];
char question[1000];
char answer[20];
char expected_answer[20];
int mode = 0;
int score = 0;
int correct = 0;
int incorrect = 0;
char scoreboard[100];

class Button {
  public:
  uint32_t S2_start_time;
  uint32_t button_change_time;    
  uint32_t debounce_duration;
  uint32_t long_press_duration;
  uint8_t pin;
  uint8_t flag;
  uint8_t button_pressed;
  button_state state; // This is public for the sake of convenience
  Button(int p) {
  flag = 0;  
    state = S0;
    pin = p;
    S2_start_time = millis(); //init
    button_change_time = millis(); //init
    debounce_duration = 7;
    long_press_duration = 1000;
    button_pressed = 0;
  }
  void read() {
    uint8_t button_val = digitalRead(pin);  
    button_pressed = !button_val; //invert button
  }
  int update() {
    read();
    flag = 0;
    if (state == S0) {
      if (button_pressed) {
        state = S1;
        button_change_time = millis();
      }
    } else if (state==S1) {
      if (button_pressed && millis() - button_change_time >= debounce_duration) {
        state = S2;
        S2_start_time = millis();
      } else if (!button_pressed) {
        state = S0;
        button_change_time = millis();
      }
    } else if (state==S2) {
      if (button_pressed && millis() - S2_start_time >= long_press_duration) {
        state = S3;
      } else if (!button_pressed) {
        button_change_time = millis();
        state = S4;
      }
    } else if (state==S3) {
      if (!button_pressed) {
        button_change_time = millis();
        state = S4;
      }
    } else if (state==S4) {      	
      if (!button_pressed && millis() - button_change_time >= debounce_duration) {
        if (millis() - S2_start_time < long_press_duration) {
          flag = 1;
        }
        if (millis() - S2_start_time >= long_press_duration) {
          flag = 2;
        }
        state = S0;
      } else if (button_pressed && millis() - S2_start_time < long_press_duration) {
        button_change_time = millis();
        state = S2;
      } else if (button_pressed && millis() - S2_start_time >= long_press_duration) {
        button_change_time = millis();
        state = S3;
      }
    }
      return flag;
  }
    
};

//HELPER FUNCTIONS
/*----------------------------------
  char_append Function:
  Arguments:
     char* buff: pointer to character array which we will append a
     char c:
     uint16_t buff_size: size of buffer buff

  Return value:
     boolean: True if character appended, False if not appended (indicating buffer full)
*/
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
  int len = strlen(buff);
  if (len > buff_size) return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}

//HELPER FUNCTIONS

/*----------------------------------
   do_http_request Function:
   Arguments:
      char* host: null-terminated char-array containing host to connect to
      char* request: null-terminated char-arry containing properly formatted HTTP request
      char* response: char-array used as output for function to contain response
      uint16_t response_size: size of response buffer (in bytes)
      uint16_t response_timeout: duration we'll wait (in ms) for a response from server
      uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
   Return value:
      void (none)
*/
void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial) {
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      if (serial) Serial.println(response);
      if (strcmp(response, "\r") == 0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis() - count > response_timeout) break;
    }
    memset(response, 0, response_size);
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");
  } else {
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

void setup() {
  Serial.begin(115200); //for debugging if needed.
  WiFi.begin(network, password); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str() , WiFi.SSID().c_str());    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }

  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); //set color of font to green foreground, black background
  tft.setCursor(0, 0, 1);
  tft.println("Start game by pressing button on pin 45!");

  //four pins needed: two inputs, two outputs. Set them up appropriately:
  pinMode(BUTTON1, INPUT_PULLUP); // toggle (pin 45)
  pinMode(BUTTON2, INPUT_PULLUP); //submit button (pin 39)
  pinMode(BUTTON3, INPUT_PULLUP); // increment num entry (pin 38)
  pinMode(BUTTON4, INPUT_PULLUP);

  mode = 0;

  delay(2000);

  primary_timer = millis();
}

//CLASS INSTANCES
Button QuestionButton(BUTTON1); //button object!
Button TrueButton(BUTTON2);
Button FalseButton(BUTTON3);
Button ResetButton(BUTTON4);

void loop() {
  reset_state = ResetButton.update();
  if (reset_state != 0 && reset_state != old_reset_state) {
    //send score to server
    if (reset_state == 1) {
      sprintf(scoreboard, "False");
    } else if (reset_state == 2) {
      sprintf(scoreboard, "True");
    }

    int offset = 0;
    // Make a HTTP request:
    Serial.println("SENDING GAME DATA");
    //get location on campus
    request[0] = '\0'; //set 0th byte to null
    offset = 0; //reset offset variable for sprintf-ing
    offset += sprintf(request + offset, "GET http://608dev-2.net/sandbox/sc/jolidou/trivia/get_database.py?user=%s&score=%d&scoreboard=%s&correct=%d&incorrect=%d HTTP/1.1\r\n", user, score, scoreboard, correct, incorrect);
    offset += sprintf(request + offset, "Host: 608dev-2.net\r\n");
    offset += sprintf(request + offset, "Content-Type: application/x-www-form=urlencoded\r\n");
    offset += sprintf(request + offset, "\r\n");
    do_http_request(server, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);

    score = 0;
    correct = 0;
    incorrect = 0;

    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0, 1);
    sprintf(output, "Game reset: press button 45 to start new game!        ");
    tft.println(output);
  }
  old_reset_state = reset_state;

  state = QuestionButton.update();
  if (mode == 0) {
    if (state != 0 && state != old_state) {
      int offset = 0;
      // Make a HTTP request:
      Serial.println("SENDING REQUEST");
      //get location on campus
      request[0] = '\0'; //set 0th byte to null
      offset = 0; //reset offset variable for sprintf-ing
      offset += sprintf(request + offset, "GET http://608dev-2.net/sandbox/sc/jolidou/trivia/questions_request.py HTTP/1.1\r\n");
      offset += sprintf(request + offset, "Host: 608dev-2.net\r\n");
      offset += sprintf(request + offset, "Content-Type: application/x-www-form=urlencoded\r\n");
      offset += sprintf(request + offset, "\r\n");
      do_http_request(server, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);

      //Get question and correct answer:
      char *start = strchr(response+1, '{');
      char *end = strrchr(response, '}');
      start[end - start + 1] = '\0';
      char json[500];
      // char subArray[500] = response[start]
      DynamicJsonDocument doc(500);
      deserializeJson(doc, start);
      sprintf(question, doc["question"]);
      sprintf(expected_answer, doc["correct_answer"]);
      memset(output, 0, sizeof(output));
      sprintf(output, "%s                         ", question);    
      mode = 1;
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 1);
      tft.println(output);    
    }
    old_state = state;
  } else {
    TrueButton.update(); //press button 39 to answer "true"
    FalseButton.update(); //press button 38 to answer "false"

    if (TrueButton.flag != 0 && FalseButton.flag != 0) {
      sprintf(output, "Please select true or false!             ");
    } else {
      if (TrueButton.flag != 0) {
        Serial.flush();
        sprintf(answer, "True");
        if (strncmp(answer,expected_answer,strlen(expected_answer))) {
          memset(output, 0, sizeof(output));
          correct++;
          score = correct - incorrect;
          sprintf(output, "Correct!     \nYou answered:\"%s\"      \nScore:%d            ", answer, score);      
          tft.fillScreen(TFT_BLACK);
        } else {
          memset(output, 0, sizeof(output));
          incorrect++;
          score = correct - incorrect;
          sprintf(output, "Wrong!       \nYou answered:\"%s\"      \nScore:%d             ", answer, score);  
          tft.fillScreen(TFT_BLACK);          
        }
        mode = 0;
      }
      if (FalseButton.flag != 0) {
        Serial.flush();
        sprintf(answer, "False");
        if (strncmp(answer,expected_answer,strlen(expected_answer))) {
          memset(output, 0, sizeof(output));
          correct++;
          score = correct - incorrect;
          sprintf(output, "Correct!     \nYou answered:\"%s\"      \nScore:%d            ", answer, score);      
          tft.fillScreen(TFT_BLACK);
        } else {
          memset(output, 0, sizeof(output));
          incorrect++;
          score = correct - incorrect;
          sprintf(output, "Wrong!       \nYou answered:\"%s\"      \nScore:%d             ", answer, score);  
          tft.fillScreen(TFT_BLACK);          
        }
        mode = 0;
      }
      tft.setCursor(0, 0, 1);
      tft.println(output);  
    } 
  }

  while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  primary_timer = millis();
}

