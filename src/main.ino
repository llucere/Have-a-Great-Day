#include "libraries/FastShiftOut.h"

// created entirely by Bryan Kearney @ https://github.com/llucere
// Sep 18th, 2022
// Modified & Documented Sep 23rd, 2022

#define CLOCK595 9
#define LATCH595 7
#define DATA595  8

#define DISPLAY1 0b1110
#define DISPLAY2 0b1101
#define DISPLAY3 0b1011
#define DISPLAY4 0b0111

FastShiftOut fast_shift_out(DATA595, CLOCK595, MSBFIRST);

// the values in the arrays 'letters' and 'numbers' are the led outputs
// the '<< 4' is to make space for which display to be used, which is a nibble of data where 0 is the display to use and 1 is the display to not use

// the shift left by 4 operations recieve constant folding internally, so its not really that inefficient
// accessed by letters[k], where toupper(char(k)) - 65 ∈ {'A' - 65, ..., 'Z' - 65}

// A-Z
int letters[] = {
  [0]   = 0b01110111 << 4,
  [1]   = 0b01111100 << 4,
  [2]   = 0b00111001 << 4,
  [3]   = 0b01011110 << 4,
  [4]   = 0b01111001 << 4,
  [5]   = 0b01110001 << 4,
  [6]   = 0b00111101 << 4,
  [7]   = 0b01110110 << 4,
  [8]   = 0b00010001 << 4,
  [9]   = 0b00001101 << 4,
  [10]  = 0b01110101 << 4,
  [11]  = 0b00111000 << 4,
  [12]  = 0b01010101 << 4,
  [13]  = 0b00010101 << 4,
  [14]  = 0b01011100 << 4,
  [15]  = 0b01110011 << 4,
  [16]  = 0b01100111 << 4,
  [17]  = 0b01010000 << 4,
  [18]  = 0b00101101 << 4,
  [19]  = 0b01111000 << 4,
  [20]  = 0b00011100 << 4,
  [21]  = 0b00101010 << 4,
  [22]  = 0b01101010 << 4,
  [23]  = 0b00010100 << 4,
  [24]  = 0b01101110 << 4,
  [25]  = 0b00011011 << 4,
};

// 0-9
int numbers[] = {
  [0] = 0b00111111 << 4,
  [1] = 0b00000110 << 4,
  [2] = 0b01011011 << 4,
  [3] = 0b01001111 << 4,
  [4] = 0b01100110 << 4,
  [5] = 0b01101101 << 4,
  [6] = 0b01111101 << 4,
  [7] = 0b00000111 << 4,
  [8] = 0b01111111 << 4,
  [9] = 0b01101111 << 4,
};


void setup(void);
void loop(void);
// -----------------------

void cld(void);                   // this function clears the display and display array
void sleep(int n,
           int t = 2);            // sleeps for n amount of time, checks if n time has passed every t milliseconds.
                                  // this sleep function supports rendering of the 7-segment led display, as opposed to a regular loop.
void runString(const char* msg,
               int type_speed,
               int end_speed);    // this function displays the value of msg onto the display.
                                  // supported characters: {A, ..., Z}, {a, ..., z}, {., } (period and space)
void out(int disp,
         int n,
         int current_character,
         char* next_character,
         int* ptr,
         char* msg);              // this function is the logic for displaying a character

inline void render(void);         // this function updates the display to the data in the _display[] array
inline uint64_t dp(int value);    // sets the decimal point on an led
inline void writeDisplay(byte n); // writes to n display
inline void shiftRegInput(int v,
                          int i); // sets _display[i] to v, where _display is the current led displays text


void setup(void) {
  DDRB = DDRB | 0b11111111; // set pins 0x8-0xD to output
  DDRD = DDRD | 0b11111100; // set pins 0x2-0x7 to output
  Serial.begin(115200);     // begin serial output
}

// _display[4] is just what is currently being rendered on the display
// lower nibble = Display4, Display3, Display2, Display1 RESPECTIVELY (0 = display on) (Display1 is leftmost display)
// high byte = g,f,e,d,c,b,a RESPECTIVELY (1 = on). to see the led labeled, search up "3461as-1" and there's a lot of results
uint64_t _display[4];

inline void shiftRegInput(int value, int index) {
  _display[index] = value;
}

inline uint64_t dp(int value) {
  return value | 0b100000000000; // enable decimal point on input
}

inline void writeDisplay(byte n) {
  delayMicroseconds(3000);                              // delay for 3000 microseconds- a considerably reasonable amount of time for extremely fast processing as such
  PORTD &= ~(1 << 7);                                   // set falling edge of latch (latch is pin 7, thus is the 7th bit starting from 0th)
  fast_shift_out.writeMSBFIRST(highByte(_display[n]));  // write the upper 8 bits of the corresponding display
  fast_shift_out.writeMSBFIRST(lowByte(_display[n]));   // write the lower 8 bits of the corresponding display
  PORTD |= (1 << 7);                                    // set rising edge of latch
}

inline void render(void) {
  writeDisplay(0);
  writeDisplay(1);
  writeDisplay(2);
  writeDisplay(3);
}

void cld(void) {
  memset(_display, 0, sizeof(_display));  // clear the display array
  render();                               // update the display
}

void sleep(int n, int t = 2) {
  if (n == 0) return;
  register uint64_t st = millis() + n;
  while ((uint64_t)millis() < st) {
    render();
    delay(t);
  }
}

void out(int disp, int n, int current_character, char* next_character, int* ptr, char* msg) {
  // if the current character is a space then just set the corresponding display to active, otherwise render a character with the corresponding display
  // n is the display number (0 - 3) (4th display is 3, 1st display is 0), and disp is the display (disp is the macro 'DISPLAY(n+1)')
  if (current_character == ' ') {
    shiftRegInput(disp, n);
  } else {
    shiftRegInput(letters[current_character - 65] | disp, n);
  }

  // if the next character is a period then
  if (*next_character == '.') {
    // set the decimal point to active on the corresponding display
    shiftRegInput(dp(_display[n]), n);

    // ignore chain of periods
    while (*next_character == '.') {
      *ptr += 1;
      *next_character = msg[*ptr + 1];
    }
  }
}

void runString(const char* msg, int type_speed, int end_speed) {
  cld();  // clear display before execution

  int ptr = 0, cap_ptr = 0, end_ptr = strlen(msg);
  char current_character = 0, next_character = 0;

  while (ptr < end_ptr) {
    current_character = toupper(msg[ptr]);
    next_character = msg[ptr + 1];

    // if there are more than 4 characters being displayed
    if (cap_ptr == 4) {
      shiftRegInput(( (_display[1] >> 4) << 4 ) | DISPLAY1, 0);         // update display 0 to the value of display 1
      shiftRegInput(( (_display[2] >> 4) << 4 ) | DISPLAY2, 1);         // update display 1 to the value of display 2
      shiftRegInput(( (_display[3] >> 4) << 4 ) | DISPLAY3, 2);         // update display 2 to the value of display 3
                                                                        // this process shifts the text over to the left
      out(DISPLAY4, 3, current_character, &next_character, &ptr, msg);  // update display 3 to the next character that needs to be displayed
    } else {
      char selected_display = (~(1 << cap_ptr)) & 0b1111;               // get the display that should be set
      out(selected_display,
          cap_ptr,
          current_character,
          &next_character,
          &ptr,
          msg);                                                         // display necessary character
      cap_ptr++;                                                        // increment cap_ptr until it's 4
    }

    ptr++;              // next character
    sleep(type_speed);  // delay before showing next character
  }

  sleep(end_speed);     // delay before ending function
  cld();                // clear display
}

void loop(void) {
  runString("Have a great day.", 120, 375);
}