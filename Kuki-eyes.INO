#include <U8g2lib.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

int COLOR_WHITE = 1;

// Eye parameters
const int ref_eye_height = 40;
const int ref_eye_width = 40;
const int ref_space_between_eye = 10;
const int ref_corner_radius = 10;

const int center_left_eye_x = SCREEN_WIDTH/2 - ref_eye_width/2 - ref_space_between_eye/2;
const int center_right_eye_x = SCREEN_WIDTH/2 + ref_eye_width/2 + ref_space_between_eye/2;
const int center_eye_y = SCREEN_HEIGHT/2;

int left_eye_x = center_left_eye_x;
int left_eye_y = center_eye_y;
int right_eye_x = center_right_eye_x;
int right_eye_y = center_eye_y;

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

enum BlinkState { POWERON_HOLD, POWERON_OPENING, WAITING, CLOSING, CLOSED, OPENING, LOOK_SIDE, RETURN_CENTER };
BlinkState blinkState = POWERON_HOLD;

unsigned long lastStateChange = 0;
unsigned long nextBlink = 0;
unsigned long lookSideStart = 0;
int eye_height = 6; // Start closed
const int blink_closed_height = 6;
const int blink_speed = 8; // pixels per frame (faster!)
const int blink_frame_delay = 5; // ms between animation frames (faster!)
const int blink_hold_closed = 80; // ms eyes stay fully closed (shorter hold)
const int poweron_hold_time = 3000; // 3 seconds closed at power-on

// Look left/right parameters
bool lookLeft = true;
int look_offset_x = 18; // how far to move eyes horizontally
int look_offset_y = 0;  // will be randomized
const int look_side_min_time = 5000; // ms to hold look side

void setup() {
  u8g2.setI2CAddress(0x78);
  u8g2.setDisplayRotation(U8G2_R0);
  u8g2.begin();

  left_eye_x = center_left_eye_x;
  left_eye_y = center_eye_y;
  right_eye_x = center_right_eye_x;
  right_eye_y = center_eye_y;

  draw_eyes(eye_height);
  lastStateChange = millis();
}

void loop() {
  unsigned long now = millis();

  switch (blinkState) {
    case POWERON_HOLD:
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
        // Randomly decide: blink or look side
        if (random(0, 2) == 0) {
          blinkState = CLOSING;
        } else {
          // Prepare to look left or right
          lookLeft = random(0, 2) == 0;
          look_offset_y = random(-10, 11); // random vertical offset
          blinkState = LOOK_SIDE;
          lookSideStart = now;
          // Move eyes to side instantly
          if (lookLeft) {
            left_eye_x = center_left_eye_x - look_offset_x;
            right_eye_x = center_right_eye_x - look_offset_x;
          } else {
            left_eye_x = center_left_eye_x + look_offset_x;
            right_eye_x = center_right_eye_x + look_offset_x;
          }
          left_eye_y = center_eye_y + look_offset_y;
          right_eye_y = center_eye_y + look_offset_y;
          draw_eyes(eye_height);
        }
        lastStateChange = now;
      }
      break;
    case LOOK_SIDE:
      // Hold the look for at least 5 seconds
      if (now - lookSideStart >= look_side_min_time) {
        // Instantly return to center
        left_eye_x = center_left_eye_x;
        right_eye_x = center_right_eye_x;
        left_eye_y = center_eye_y;
        right_eye_y = center_eye_y;
        draw_eyes(eye_height);
        blinkState = WAITING;
        nextBlink = now + random(5000, 10001); // 5-10 seconds
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
