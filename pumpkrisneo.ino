/* 

  Tetris for Arduino Metro using NeoPixels

  2022-09-21: Jonathan Bettger
    Took a lot of ideas and code from Nathan Pryor's Pumpktris
    Some ideas from John Bradnam Wii Chuck Neopixel Tetris https://www.hackster.io/john-bradnam/wii-chuck-neopixel-tetris-game-047fdc
    Fernado Jerez via the above.
*/

#include <FastLED.h>

// #define DEBUG   //If you comment this line, the DPRINT & DPRINTLN lines are defined as blank.
#ifdef DEBUG                                       //Macros are usually in all capital letters.
#define DPRINT(...) Serial.print(__VA_ARGS__)      //DPRINT is a macro, debug print
#define DPRINTLN(...) Serial.println(__VA_ARGS__)  //DPRINTLN is a macro, debug print with new line
#define SERIAL_BEGIN Serial.begin(9600)
#else
#define DPRINT(...)  //now defines a blank line
#define DPRINTLN(...)
#define SERIAL_BEGIN
#endif

/**************
   PINOUT
 **************/

#define MATRIX_PIN 2
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define MATRIX_PIXELS (BOARD_WIDTH * BOARD_HEIGHT)
#define MATRIX_BRIGHTNESS 8

CRGB leds[MATRIX_PIXELS];

int score;            // the total score
int totalLines;       // number of lines cleared
int level;            // the current level, should be totalLines/10
int stepCounter = 0;  // increments with every program loop. resets after the value of gravityTrigger is reached
int gravityTrigger;   // the active piece will drop one step after this many steps of stepCounter;
bool gameOver = true;
bool inverted = true;

byte board[BOARD_HEIGHT][BOARD_WIDTH];

int activeShape;          // the index of the currently active shape
int currentRotation = 0;  // the index of the current piece's rotation

int yOffset = 0;  // how far from the top the active shape is
int xOffset = 0;  // how far from the left the active shape is

// PIECES

#define EMPTY 7

typedef struct {
  int rotations[4][4][2];  // picture
  CRGB color;              // color
  char pieceName;
} Piece;

Piece pieces[7] = {
  { {
      { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 1, 2 } }, /* angle 0   */
      { { 1, 0 }, { 1, 1 }, { 2, 1 }, { 1, 2 } }, /* angle 90  */
      { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } }, /* angle 180 */
      { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 } }  /* angle 270 */
    },
    CRGB(127, 0, 127),
    'T' },
  { {
      { { 1, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 } }, /* angle 0   */
      { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 } }, /* angle 90  */
      { { 1, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 } }, /* angle 180 */
      { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 } }  /* angle 270 */
    },
    CRGB(0, 255, 0),
    'S' },
  { {
      { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 0, 2 } }, /* angle 0   */
      { { 1, 0 }, { 1, 1 }, { 1, 2 }, { 2, 2 } }, /* angle 90  */
      { { 2, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } }, /* angle 180 */
      { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 1, 2 } }  /* angle 270 */
    },
    CRGB(0, 0, 255),
    'L' },
  { {
      { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 } }, /* angle 0   */
      { { 1, 0 }, { 1, 1 }, { 1, 2 }, { 1, 3 } }, /* angle 90  */
      { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 } }, /* angle 180 */
      { { 1, 0 }, { 1, 1 }, { 1, 2 }, { 1, 3 } }  /* angle 270 */
    },
    CRGB(0, 255, 255),
    'I' },
  { {
      { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 2, 2 } }, /* angle 0   */
      { { 1, 0 }, { 2, 0 }, { 1, 1 }, { 1, 2 } }, /* angle 90  */
      { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } }, /* angle 180 */
      { { 1, 0 }, { 1, 1 }, { 1, 2 }, { 0, 2 } }  /* angle 270 */
    },
    CRGB(255, 127, 0),
    'J' },
  { {
      { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 2, 1 } }, /* angle 0   */
      { { 2, 0 }, { 1, 1 }, { 2, 1 }, { 1, 2 } }, /* angle 90  */
      { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 2, 1 } }, /* angle 180 */
      { { 2, 0 }, { 1, 1 }, { 2, 1 }, { 1, 2 } }  /* angle 270 */
    },
    CRGB(127, 0, 0),
    'Z' },
  { {
      { { 1, 0 }, { 2, 0 }, { 1, 1 }, { 2, 1 } }, /* angle 0   */
      { { 1, 0 }, { 2, 0 }, { 1, 1 }, { 2, 1 } }, /* angle 90  */
      { { 1, 0 }, { 2, 0 }, { 1, 1 }, { 2, 1 } }, /* angle 180 */
      { { 1, 0 }, { 2, 0 }, { 1, 1 }, { 2, 1 } }  /* angle 270 */
    },
    CRGB(255, 255, 0),
    'O' },
};

#define DEBOUNCE 10  // button debouncer, how many ms to debounce, 5+ ms is usually plenty

// here is where we define the buttons that we'll use. button "1" is the first, button "6" is the 6th, etc
byte buttons[] = { 14, 15, 16, 17 };  // the analog 0-5 pins are also known as 14-19
// analog 0 = rotate (bottom switch on joystick)
// analog 1 = move right (left switch on joystick)
// analog 2 = drop (top switch on joystick)
// analog 3 = move left (right switch on joystick)

// This handy macro lets us determine how big the array up above is, by checking the size
#define NUMBUTTONS sizeof(buttons)
// we will track if a button is just pressed, just released, or 'currently pressed'
volatile byte pressed[NUMBUTTONS], justpressed[NUMBUTTONS], justreleased[NUMBUTTONS];

void setup_buttons() {
  // Make input & enable pull-up resistors on switch pins
  byte i;
  for (i = 0; i < NUMBUTTONS; i++) {
    pinMode(buttons[i], INPUT);
    digitalWrite(buttons[i], HIGH);
  }

  // Run timer2 interrupt every 15 ms
  TCCR2A = 0;
  TCCR2B = 1 << CS22 | 1 << CS21 | 1 << CS20;

  //Timer2 Overflow Interrupt Enable
  TIMSK2 |= 1 << TOIE2;
}

SIGNAL(TIMER2_OVF_vect) {
  check_switches();
}

void check_switches() {
  static byte previousstate[NUMBUTTONS];
  static byte currentstate[NUMBUTTONS];
  static long lasttime;
  byte index;

  if (millis() < lasttime) {
    // we wrapped around, lets just try again
    lasttime = millis();
  }

  if ((lasttime + DEBOUNCE) > millis()) {
    // not enough time has passed to debounce
    return;
  }
  // ok we have waited DEBOUNCE milliseconds, lets reset the timer
  lasttime = millis();

  for (index = 0; index < NUMBUTTONS; index++) {

    currentstate[index] = digitalRead(buttons[index]);  // read the button
    if (currentstate[index] == previousstate[index]) {
      if ((pressed[index] == LOW) && (currentstate[index] == LOW)) {
        // just pressed
        justpressed[index] = 1;
      } else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH)) {
        // just released
        justreleased[index] = 1;
      }
      pressed[index] = !currentstate[index];  // remember, digital HIGH means NOT pressed
    }
    previousstate[index] = currentstate[index];  // keep a running tally of the buttons
  }
}

void setup() {
  SERIAL_BEGIN;
  delay(1500);

  FastLED.addLeds<NEOPIXEL, MATRIX_PIN>(leds, MATRIX_PIXELS);
  FastLED.setBrightness(MATRIX_BRIGHTNESS);

  clearBoard();

  paintAll(200);

  setup_buttons();
}

void loop() {
  stepCounter++;

  if (!gameOver) {
    for (byte i = 0; i < NUMBUTTONS; i++) {
      //The left, right, and rotate switches are triggered once then need to be released and re-triggered.
      //They can't be held down for continuous movement or rotation
      if (justpressed[i]) {
        justpressed[i] = 0;
        switch (i) {
          case 0:  // rotate
            if (checkNextMove(currentRotation + 1, xOffset, yOffset)) {
              currentRotation++;
              if (currentRotation > 3) { currentRotation = 0; }
            };
            break;
          case 1:  // move right
            if (checkNextMove(currentRotation, xOffset + (inverted ? -1 : +1), yOffset)) {
              if (inverted) {
                xOffset--;
              } else {
                xOffset++;
              }
            };
            break;
          case 2:                                         // accelerate the drop speed
            gravityTrigger = min(3, gravityTrigger / 2);  // increase the speed of the drop to the lowest of either 3 or 1/2 the current speed
                                                          // it's probably not the method used in the real game, but it was a quick solution
            break;
          case 3:  // move left
            if (checkNextMove(currentRotation, xOffset + (inverted ? +1 : -1), yOffset)) {
              if (inverted) {
                xOffset++;
              } else {
                xOffset--;
              }
            }
            break;
        }
      }
    }

    if (justreleased[2]) {                  // when the down button is released
      gravityTrigger = max(1, 21 - level);  // reset the drop speed to its normal speed for that level
      justreleased[2] = 0;
    }

    //drop the piece one step every N ticks. N=gravityTrigger
    if (stepCounter > gravityTrigger) {  // if the program has looped more times than the gravityTrigger then
      stepCounter = 0;                   // reset the loop counter

      if (checkNextMove(currentRotation, xOffset, yOffset + 1)) {  // check to see if the piece can be dropped a level
        yOffset++;                                                 // if so, then do it
      } else {                                                     // if not,
        storeFinalPlacement();                                     // record the final location of that block
        dropFullRows();                                            // clear out any full rows and drop the ones above
        launchNewShape();                                          // drop a new shape from the top
      }
    }

    if (!gameOver) {
      clearLEDs();
      drawFixedMinos();
      drawActivePiece();
      FastLED.show();
    }
  } else {
    // GAME OVER
    if (stepCounter > 10) {
      stepCounter = 0;
      blinkRandom();
    }

    // Press anything to start
    for (byte i = 0; i < NUMBUTTONS; i++) {
      if (justpressed[i]) {
        gameOver = false;
        startGame();
        break;
      }
    }
  }
  delay(50);
}

void startGame() {
  stepCounter = 0;
  gravityTrigger = 20;  // reset the trigger point at which the active piece will drop another step
  level = 1;            // reset the level
  score = 0;            // reset the score
  totalLines = 0;       // reset the line count
  clearBoard();         // reset the fixed-piece grid
  launchNewShape();     // kick off the first shape
}

void launchNewShape() {
  activeShape = random(7);              // pick a random shape
  yOffset = 0;                          // reset yOffset so it comes from the top
  xOffset = BOARD_WIDTH / 2;            // reset xOffset so it comes from the middle
  currentRotation = 0;                  // pieces start from the default rotation
  gravityTrigger = max(1, 21 - level);  // set the gravity appropriately. this is so that a drop doesn't carry to the next piece

  //can the piece be placed in the starting location?
  //if not, the stack is full so end the game
  if (!checkNextMove(currentRotation, xOffset, yOffset)) {
    endGame();
  }
}

boolean checkNextMove(int nextRot, int nextXOffset, int nextYOffset) {
  boolean isOK = true;  // assume the move is valid. we'll change isOK if it's not

  if (nextRot > 3) { nextRot = 0; }  //rotate back to the original 0-index position if needed

  for (int thisPixel = 0; thisPixel < 4; thisPixel++) {  // Check all 4 pixels of the shape in play
    // have we gone out of bounds left or right?
    if (pieces[activeShape].rotations[nextRot][thisPixel][0] + nextXOffset > (BOARD_WIDTH - 1)
        || pieces[activeShape].rotations[nextRot][thisPixel][0] + nextXOffset < 0) {
      isOK = false;
      break;  //no need to check further
    }

    //have we hit the bottom of the grid?
    if (pieces[activeShape].rotations[nextRot][thisPixel][1] + nextYOffset > (BOARD_HEIGHT - 1)) {
      isOK = false;
      break;  //no need to check further
    }

    //have we collided with any other shapes?
    int inverseCol = (BOARD_WIDTH - 1) - (pieces[activeShape].rotations[nextRot][thisPixel][0] + nextXOffset);
    int theRow = (pieces[activeShape].rotations[nextRot][thisPixel][1]) + nextYOffset;
    if (board[theRow][inverseCol] != EMPTY) {
      isOK = false;
      break;  //no need to check further
    }
  }
  return isOK;
}

void storeFinalPlacement() {
  for (int thisPixel = 0; thisPixel < 4; thisPixel++) {
    int pixelX = pieces[activeShape].rotations[currentRotation][thisPixel][0] + xOffset;  //calculate its final, offset X position
    int pixelY = pieces[activeShape].rotations[currentRotation][thisPixel][1] + yOffset;  //calculate its final, offset Y position
    board[pixelY][(BOARD_WIDTH - 1) - pixelX] = activeShape;
  }
}

void drawFixedMinos() {
  for (int i = 0; i < BOARD_HEIGHT; i++) {
    for (int j = 0; j < BOARD_WIDTH; j++) {
      if (board[i][j] != EMPTY) {
        if (i % 2 == 0) {  // Zig Row
          setLED(i * BOARD_WIDTH + j, pieces[board[i][j]].color);
        } else {  // Zag Row
          setLED(i * BOARD_WIDTH + (BOARD_WIDTH - 1 - j), pieces[board[i][j]].color);
        }
      }
    }
  }
}

void drawActivePiece() {
  for (int thisPixel = 0; thisPixel < 4; thisPixel++) {
    int pixelX = pieces[activeShape].rotations[currentRotation][thisPixel][0] + xOffset;
    int pixelY = pieces[activeShape].rotations[currentRotation][thisPixel][1] + yOffset;

    int adjustedX;
    if (pixelY % 2 != 0) {
      adjustedX = pixelX;
    } else {
      adjustedX = (BOARD_WIDTH - 1 - pixelX);
    }


    setLED(pixelY * BOARD_WIDTH + adjustedX, pieces[activeShape].color);
  }
}

void dropFullRows() {
  int fullRows[4];  // to store the Y location of full rows. There can never be more than four at a time
  int fullRowCount = 0;
  for (int row = 0; row < BOARD_HEIGHT; row++) {  // step through each row of the played grid
    bool allFilled = true;
    for (int col = 0; col < BOARD_WIDTH; col++) {
      if (board[row][col] == EMPTY) {
        allFilled = false;
        break;
      }
    }
    if (allFilled) {                 // if the row is full then
      fullRows[fullRowCount] = row;  // record the position of the filled row
      fullRowCount++;                // increment the count of full rows
    }
  }


  if (fullRowCount > 0) {  // if there is a filled row, blink all filled rows
    for (int blinkCount = 0; blinkCount < 3; blinkCount++) {
      for (int i = 0; i < fullRowCount; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
          int adjustedJ;
          if (i % 2 != 0) {
            adjustedJ = j;
          } else {
            adjustedJ = (BOARD_WIDTH - 1 - j);
          }
          setLED(fullRows[i] * BOARD_WIDTH + adjustedJ, CRGB::Black);
          // leds[fullRows[i] * BOARD_WIDTH + adjustedJ] = CRGB::Black;
        }
      }
      FastLED.show();
      delay(200);
      drawFixedMinos();
      FastLED.show();
      delay(200);
    }

    //  remove the filled rows and drop them
    for (int i = 0; i < fullRowCount; i++) {                     // we only need to do this for as many filled rows as there are
      for (int copyRow = fullRows[i]; copyRow > 0; copyRow--) {  // for every row above the filled row
        for (int j = 0; j < BOARD_WIDTH; j++) {
          board[copyRow][j] = board[copyRow - 1][j];
        }
      }
      for (int j = 0; j < BOARD_WIDTH; j++) {
        board[0][j] = EMPTY;
      }
    }

    totalLines = totalLines + fullRowCount;  //  add the filled rows to the total line count

    //  add points to the score
    int thePoints;
    switch (fullRowCount) {
      case 1:  // Single = 100 x Level
        thePoints = 100 * level;
        break;
      case 2:  // Double = 300 x Level
        thePoints = 300 * level;
        break;
      case 3:  // Triple = 500 x Level
        thePoints = 500 * level;
        break;
      case 4:  // Tetris = 800 x Level
        thePoints = 800 * level;
        break;
    }

    score = score + thePoints;  // add the points to the total score

    //increment the level every 10 lines
    if (totalLines % 10 == 0) {
      level++;
      gravityTrigger--;  // and with every level, decrease the trigger value at which stepCounter will drop the piece. This makes it faster with every level
    }
  }
}

void endGame() {
  printBoard();
  gameOver = true;
  clearLEDs();
  clearBoard();
  paintAll(500);
}

void printBoard() {
  char buffer[2];
  for (int i = 0; i < BOARD_HEIGHT; i++) {
    for (int j = 0; j < BOARD_WIDTH; j++) {
      if (board[i][j] != EMPTY) {
        sprintf(buffer, "%c", pieces[board[i][j]].pieceName);
        DPRINT(buffer);
      } else {
        sprintf(buffer, "%c", '*');
        DPRINT(buffer);
      }
    }
    DPRINTLN("");
  }
}

void setLED(int index, CRGB color) {
  int number = index;
  if (inverted) {
    number = MATRIX_PIXELS - 1 - index;
  }

  leds[number] = color;
}

void clearLEDs() {
  int numOfLEDs = (sizeof(pieces) / sizeof(Piece)) - 1;
  for (int led = numOfLEDs; led >= 0; led--) {
    leds[led] = CRGB::Black;
  }
  FastLED.clear();
}

// Fill the board with EMPTY
void clearBoard() {
  for (int i = 0; i < BOARD_HEIGHT; i++) {
    for (int j = 0; j < BOARD_WIDTH; j++) {
      board[i][j] = EMPTY;
    }
  }
}

enum BlinkState {
  BlinkOffFirst,
  BlinkOnFirst,
  BlinkOffSecond,
  BlinkOnSecond
};

BlinkState blinkState = BlinkOffFirst;
int blinkLED;
CRGB blinkColor;

void blinkRandom() {
  switch (blinkState) {
    case BlinkOffFirst:
      blinkLED = random(0, MATRIX_PIXELS);
      blinkColor = CRGB(random(0, 256), random(0, 256), random(0, 256));

      leds[blinkLED] = CRGB::Black;
      blinkState = BlinkOnFirst;
      break;
    case BlinkOnFirst:
      leds[blinkLED] = blinkColor;
      blinkState = BlinkOffSecond;
      break;
    case BlinkOffSecond:
      leds[blinkLED] = CRGB::Black;
      blinkState = BlinkOnSecond;
      break;
    case BlinkOnSecond:
      leds[blinkLED] = blinkColor;
      blinkState = BlinkOffFirst;
      break;
  }
  FastLED.show();
}

void paintAll(int timing) {
  int numOfLEDs = (sizeof(pieces) / sizeof(Piece)) - 1;
  for (int piece = numOfLEDs; piece >= 0; piece--) {
    for (int i = 0; i < MATRIX_PIXELS; i++) {
      leds[i] = pieces[piece].color;
    }
    FastLED.show();
    delay(timing);
  }

  FastLED.clear();
  FastLED.show();
}
