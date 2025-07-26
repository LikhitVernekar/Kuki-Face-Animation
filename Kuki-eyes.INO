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

// For two random positions on the same side (declare these BEFORE the functions)
int look1_eye_x, look1_eye_y, look2_eye_x, look2_eye_y;

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

// --- NORMAL LOOK-AROUND FUNCTIONS ---
void setRandomLookPosition(bool left, int &eye_x, int &eye_y) {
  int offset_x = left ? -18 : 18;
  int random_y = center_eye_y + random(-10, 11);
  eye_x = left ? center_left_eye_x + offset_x : center_left_eye_x + offset_x;
  eye_y = random_y;
}

void setRandomLookPositions(bool left) {
  // First look position
  setRandomLookPosition(left, look1_eye_x, look1_eye_y);
  // Second look position, ensure it's different from the first
  do {
    setRandomLookPosition(left, look2_eye_x, look2_eye_y);
  } while (look2_eye_y == look1_eye_y);
}
// -------------------------------------

enum BlinkState {
  POWERON_HOLD, POWERON_OPENING, POWERON_WAIT_AFTER_OPEN,
  POWERON_LOOK_AROUND,
  WAITING, CLOSING, CLOSED, OPENING,
  LOOK_SIDE_1, LOOK_SIDE_1_BLINKING, LOOK_SIDE_1_BLINK_CLOSED, LOOK_SIDE_1_BLINK_OPENING,
  LOOK_SIDE_2, RETURN_CENTER
};
BlinkState blinkState = POWERON_HOLD;

unsigned long lastStateChange = 0;
unsigned long nextBlink = 0;
unsigned long nextLookAround = 0;
unsigned long lookSideStart = 0;
unsigned long lookSideBlinkTime = 0;
int lookSideBlinkCount = 0;
int lookSideBlinkTarget = 0;
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
const int look_side_min_time = 5000; // ms to hold look side (5 seconds total)

// For power-on look around
const int poweron_look_hold = 1000; // ms to hold at each position
const int poweron_wait_after_open = 1000; // ms to wait after opening eyes

// Sequence: left-top, left-down, right-top, right-down
const int poweron_look_steps = 4; // 4 steps: left-top, left-down, right-top, right-down
int poweron_look_index = 0;
unsigned long poweron_look_last = 0;

void setPoweronLookPosition(int step) {
  // step: 0 = left-top, 1 = left-down, 2 = right-top, 3 = right-down
  int offset_x, y;
  if (step % 2 == 0) y = center_eye_y - 10; // top
  else y = center_eye_y + 10; // down

  if (step < 2) { // left
    offset_x = -look_offset_x;
    left_eye_x = center_left_eye_x + offset_x;
    right_eye_x = left_eye_x + (center_right_eye_x - center_left_eye_x);
  } else { // right
    offset_x = look_offset_x;
    left_eye_x = center_left_eye_x + offset_x;
    right_eye_x = left_eye_x + (center_right_eye_x - center_left_eye_x);
  }
  left_eye_y = y;
  right_eye_y = y;
}

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
          blinkState = POWERON_WAIT_AFTER_OPEN;
          lastStateChange = now;
        }
      }
      break;
    case POWERON_WAIT_AFTER_OPEN:
      if (now - lastStateChange >= poweron_wait_after_open) {
        // Start power-on look around sequence
        poweron_look_index = 0;
        poweron_look_last = now - poweron_look_hold; // So it triggers immediately
        blinkState = POWERON_LOOK_AROUND;
        lastStateChange = now;
      }
      break;
    case POWERON_LOOK_AROUND:
      if (poweron_look_index < poweron_look_steps) {
        if (now - poweron_look_last >= poweron_look_hold) {
          setPoweronLookPosition(poweron_look_index);
          draw_eyes(eye_height);
          poweron_look_index++;
          poweron_look_last = now;
        }
      } else {
        // Done with look around, return to center and normal state
        if (now - poweron_look_last >= poweron_look_hold) {
          left_eye_x = center_left_eye_x;
          right_eye_x = center_right_eye_x;
          left_eye_y = center_eye_y;
          right_eye_y = center_eye_y;
          draw_eyes(eye_height);
          blinkState = WAITING;
          nextBlink = now + random(3000, 8001);
          nextLookAround = now + random(15000, 30001);
          lastStateChange = now;
        }
      }
      break;
    case WAITING:
      if (now > nextLookAround) {
        // Look around animation (priority over normal blinking)
        lookLeft = random(0, 2) == 0;
        setRandomLookPositions(lookLeft);
        // Move eyes to first random position instantly
        left_eye_x = look1_eye_x;
        right_eye_x = lookLeft ? look1_eye_x + (center_right_eye_x - center_left_eye_x) : look1_eye_x + (center_right_eye_x - center_left_eye_x);
        left_eye_y = look1_eye_y;
        right_eye_y = look1_eye_y;
        draw_eyes(eye_height);
        lookSideBlinkTarget = random(1, 3); // 1 or 2 blinks
        lookSideBlinkCount = 0;
        blinkState = LOOK_SIDE_1;
        lookSideStart = now;
        lookSideBlinkTime = lookSideStart + random(1000, 2001); // Blink after 1-2 seconds
        lastStateChange = now;
      } else if (now > nextBlink) {
        // Normal blinking
        blinkState = CLOSING;
        lastStateChange = now;
      }
      break;
    case LOOK_SIDE_1:
      // Wait for blink time (1-2s) or total time (5s)
      if (now >= lookSideBlinkTime && lookSideBlinkCount == 0) {
        blinkState = LOOK_SIDE_1_BLINKING;
        lastStateChange = now;
      } else if (now - lookSideStart >= look_side_min_time) {
        // Time to move to second position
        left_eye_x = look2_eye_x;
        right_eye_x = lookLeft ? look2_eye_x + (center_right_eye_x - center_left_eye_x) : look2_eye_x + (center_right_eye_x - center_left_eye_x);
        left_eye_y = look2_eye_y;
        right_eye_y = look2_eye_y;
        draw_eyes(eye_height);
        blinkState = LOOK_SIDE_2;
        lookSideStart = now;
        lastStateChange = now;
      }
      break;
    case LOOK_SIDE_1_BLINKING:
      if (now - lastStateChange >= blink_frame_delay) {
        if (eye_height > blink_closed_height) {
          eye_height -= blink_speed;
          if (eye_height < blink_closed_height) eye_height = blink_closed_height;
          draw_eyes(eye_height);
          lastStateChange = now;
        } else {
          blinkState = LOOK_SIDE_1_BLINK_CLOSED;
          lastStateChange = now;
        }
      }
      break;
    case LOOK_SIDE_1_BLINK_CLOSED:
      if (now - lastStateChange >= blink_hold_closed) {
        blinkState = LOOK_SIDE_1_BLINK_OPENING;
        lastStateChange = now;
      }
      break;
    case LOOK_SIDE_1_BLINK_OPENING:
      if (now - lastStateChange >= blink_frame_delay) {
        if (eye_height < ref_eye_height) {
          eye_height += blink_speed;
          if (eye_height > ref_eye_height) eye_height = ref_eye_height;
          draw_eyes(eye_height);
          lastStateChange = now;
        } else {
          lookSideBlinkCount++;
          if (lookSideBlinkCount < lookSideBlinkTarget) {
            blinkState = LOOK_SIDE_1_BLINKING;
            lastStateChange = now;
          } else {
            blinkState = LOOK_SIDE_1;
            lastStateChange = now;
          }
        }
      }
      break;
    case LOOK_SIDE_2:
      // Wait for 5 seconds total, then return to center (no blinking)
      if (now - lookSideStart >= look_side_min_time) {
        blinkState = RETURN_CENTER;
        lastStateChange = now;
      }
      break;
    case RETURN_CENTER:
      // Instantly return to center (no blink)
      left_eye_x = center_left_eye_x;
      right_eye_x = center_right_eye_x;
      left_eye_y = center_eye_y;
      right_eye_y = center_eye_y;
      draw_eyes(eye_height);
      blinkState = WAITING;
      nextBlink = now + random(3000, 8001); // 3-8 seconds for normal blinking
      nextLookAround = now + random(15000, 30001); // 15-30 seconds for look around
      lastStateChange = now;
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
          nextBlink = now + random(3000, 8001); // 3-8 seconds for normal blinking
          // Don't reset nextLookAround here - only reset it after look around completes
        }
      }
      break;
  }
}
