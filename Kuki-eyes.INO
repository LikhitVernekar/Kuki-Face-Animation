#include <U8g2lib.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Use SH1106 constructor to avoid the right-side line
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

int COLOR_WHITE = 1;

// Eye parameters
const int ref_eye_height = 40;
const int ref_eye_width = 40;
const int ref_space_between_eye = 10;
const int ref_corner_radius = 10;

int left_eye_x, left_eye_y;
int right_eye_x, right_eye_y;

void display_clearDisplay() { u8g2.clearBuffer(); }
void display_fillRoundRect(int x,int y,int w, int h, int r, int color) {
  u8g2.setDrawColor(color);
  if( w<2*(r+1) ) r = (w/2)-1;
  if( h<2*(r+1) ) r = (h/2)-1;
  u8g2.drawRBox(x,y,w<1?1:w,h<1?1:h,r);
}
void display_display() { u8g2.sendBuffer(); }

void draw_eyes(int height) {
  display_clearDisplay();
  int x = left_eye_x - ref_eye_width/2;
  int y = left_eye_y - height/2;
  display_fillRoundRect(x, y, ref_eye_width, height, ref_corner_radius, COLOR_WHITE);
  x = right_eye_x - ref_eye_width/2;
  y = right_eye_y - height/2;
  display_fillRoundRect(x, y, ref_eye_width, height, ref_corner_radius, COLOR_WHITE);
  display_display();
}

enum BlinkState { POWERON_HOLD, POWERON_OPENING, WAITING, CLOSING, CLOSED, OPENING };
BlinkState blinkState = POWERON_HOLD;

unsigned long lastStateChange = 0;
unsigned long nextBlink = 0;
int eye_height = 6; // Start closed
const int blink_closed_height = 6;
const int blink_speed = 8; // pixels per frame (faster!)
const int blink_frame_delay = 5; // ms between animation frames (faster!)
const int blink_hold_closed = 80; // ms eyes stay fully closed (shorter hold)
const int poweron_hold_time = 3000; // 3 seconds closed at power-on

void setup() {
  u8g2.setI2CAddress(0x78);
  u8g2.setDisplayRotation(U8G2_R0);
  u8g2.begin();

  left_eye_x = SCREEN_WIDTH/2 - ref_eye_width/2 - ref_space_between_eye/2;
  left_eye_y = SCREEN_HEIGHT/2;
  right_eye_x = SCREEN_WIDTH/2 + ref_eye_width/2 + ref_space_between_eye/2;
  right_eye_y = SCREEN_HEIGHT/2;

  draw_eyes(eye_height);
  lastStateChange = millis();
}

void loop() {
  unsigned long now = millis();

  switch (blinkState) {
    case POWERON_HOLD:
      // Keep eyes closed for 3 seconds at power-on
      if (now - lastStateChange >= poweron_hold_time) {
        blinkState = POWERON_OPENING;
        lastStateChange = now;
      }
      break;
    case POWERON_OPENING:
      if (now - lastStateChange >= blink_frame_delay) {
        if (eye_height < ref_eye_height) {
          eye_height += blink_speed;
          if (eye_height > ref_eye_height) eye_height = ref_eye_height;
          draw_eyes(eye_height);
          lastStateChange = now;
        } else {
          blinkState = WAITING;
          nextBlink = now + random(5000, 10001); // 5-10 seconds
        }
      }
      break;
    case WAITING:
      if (now > nextBlink) {
        blinkState = CLOSING;
        lastStateChange = now;
      }
      break;
    case CLOSING:
      if (now - lastStateChange >= blink_frame_delay) {
        if (eye_height > blink_closed_height) {
          eye_height -= blink_speed;
          if (eye_height < blink_closed_height) eye_height = blink_closed_height;
          draw_eyes(eye_height);
          lastStateChange = now;
        } else {
          blinkState = CLOSED;
          lastStateChange = now;
        }
      }
      break;
    case CLOSED:
      if (now - lastStateChange >= blink_hold_closed) {
        blinkState = OPENING;
        lastStateChange = now;
      }
      break;
    case OPENING:
      if (now - lastStateChange >= blink_frame_delay) {
        if (eye_height < ref_eye_height) {
          eye_height += blink_speed;
          if (eye_height > ref_eye_height) eye_height = ref_eye_height;
          draw_eyes(eye_height);
          lastStateChange = now;
        } else {
          blinkState = WAITING;
          nextBlink = now + random(5000, 10001); // 5-10 seconds
        }
      }
      break;
  }
}
