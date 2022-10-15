/* 

  Tetris for Arduino Metro using NeoPixels

  2022-09-21: Jonathan Bettger
    Took a lot of ideas and code from Nathan Pryor's Pumpktris
    Some ideas from John Bradnam Wii Chuck Neopixel Tetris https://www.hackster.io/john-bradnam/wii-chuck-neopixel-tetris-game-047fdc
    Fernado Jerez via the above.
*/

#include <FastLED.h>

// #define DEBUG                                      //If you comment this line, the DPRINT & DPRINTLN lines are defined as blank.
#ifdef DEBUG                                       //Macros are usually in all capital letters.
#define DPRINT(...) Serial.print(__VA_ARGS__)      //DPRINT is a macro, debug print
#define DPRINTLN(...) Serial.println(__VA_ARGS__)  //DPRINTLN is a macro, debug print with new line
#define SERIAL_BEGIN Serial.begin(9600)
#else
#define DPRINT(...)  //now defines a blank line
#define DPRINTLN(...)
#define SERIAL_BEGIN
#endif

// *************
// BASIC DEFINES
// *************
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

#define MATRIX_PIN 2
#define MATRIX_PIXELS (BOARD_WIDTH * BOARD_HEIGHT)
#define MATRIX_BRIGHTNESS 30

CRGB leds[MATRIX_PIXELS];
byte board[BOARD_HEIGHT][BOARD_WIDTH];

int score;            // the total score
int totalLines;       // number of lines cleared
int level;            // the current level, should be totalLines/10
int stepCounter = 0;  // increments with every program loop. resets after the value of gravityTrigger is reached
int gravityTrigger;   // the active piece will drop one step after this many steps of stepCounter;
bool gameOver = true;
bool inverted = true;

// *************
// PIECES
// *************
int activeShape;
int currentRotation = 0;

int yOffset = 0;
int xOffset = 0;

#define EMPTY 7

int bagOfPieces[7];
int currentBagSelection = EMPTY;

typedef struct {
  int rotations[4][4][2];
  CRGB color;
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
bool listenForInput = false;

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
  if (!listenForInput) {
    return;
  }

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
  FastLED.setBrightness(8);

  clearBoard();

  int numOfPieces = (sizeof(pieces) / sizeof(Piece)) - 1;
  for (int piece = numOfPieces; piece >= 0; piece--) {
    paintAll(200, pieces[piece].color);
  }

  setup_buttons();

  drawDirectionArrows(200);

  listenForInput = true;
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
            if (checkNextMove(currentRotation, xOffset + (inverted ? +1 : -1), yOffset)) {
              if (inverted) {
                xOffset++;
              } else {
                xOffset--;
              }
            };
            break;
          case 2:                                         // accelerate the drop speed
            gravityTrigger = min(3, gravityTrigger / 2);  // increase the speed of the drop to the lowest of either 3 or 1/2 the current speed
                                                          // it's probably not the method used in the real game, but it was a quick solution
            break;
          case 3:  // move left
            if (checkNextMove(currentRotation, xOffset + (inverted ? -1 : +1), yOffset)) {
              if (inverted) {
                xOffset--;
              } else {
                xOffset++;
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
      drawFixedMinos(BOARD_HEIGHT);
      drawActivePiece();
      FastLED.show();
    }
  } else {
    // GAME OVER
    if (stepCounter % 5 == 0) {
      FastLED.setBrightness(30);
      drawPumpkinFace(stepCounter);
      FastLED.show();
    }

    for (byte i = 0; i < NUMBUTTONS; i++) {
      if (justpressed[i]) {
        justpressed[i] = 0;
        switch (i) {
          case 0:  // invert
            inverted = !inverted;
            drawDirectionArrows(500);
            break;
          default:
            startGame();
            break;
        }
      }
    }
  }
  delay(50);
}

int pickNextPieceFromBag() {
  if (currentBagSelection == EMPTY) {
    currentBagSelection = 0;
    for (int i = 0; i < EMPTY; i++) {
      bagOfPieces[i] = i;
    }

    // Fisher-Yates Shuffle
    for (int i = EMPTY - 1; i >= 1; i--) {
      int j = random(0, i);
      // In-place shuffle
      bagOfPieces[i] ^= bagOfPieces[j];
      bagOfPieces[j] ^= bagOfPieces[i];
      bagOfPieces[i] ^= bagOfPieces[j];
    }

    printBag();
  }

  return bagOfPieces[currentBagSelection++];  // post increase selection;
}

void launchNewShape() {
  activeShape = pickNextPieceFromBag();  // pick a shape
  yOffset = -1;                          // reset yOffset so it comes from the top
  xOffset = (BOARD_WIDTH - 1) / 2;       // reset xOffset so it comes from the middle
  currentRotation = 0;                   // pieces start from the default rotation
  gravityTrigger = max(1, 21 - level);   // set the gravity appropriately. this is so that a drop doesn't carry to the next piece

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
    int col = pieces[activeShape].rotations[nextRot][thisPixel][0] + nextXOffset;
    int row = pieces[activeShape].rotations[nextRot][thisPixel][1] + nextYOffset;

    if (row < 0 || col < 0) {
      continue;
    }

    if (board[row][col] != EMPTY) {
      isOK = false;
      break;  //no need to check further
    }
  }
  return isOK;
}

void storeFinalPlacement() {
  for (int thisPixel = 0; thisPixel < 4; thisPixel++) {
    int col = pieces[activeShape].rotations[currentRotation][thisPixel][0] + xOffset;
    int row = pieces[activeShape].rotations[currentRotation][thisPixel][1] + yOffset;

    if (row < 0 || col < 0) {
      continue;
    }

    board[row][col] = activeShape;
  }
}

void drawFixedMinos(int height) {
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < BOARD_WIDTH; j++) {
      if (board[i][j] != EMPTY) {
        setLED(i, j, pieces[board[i][j]].color);
      } else {
        setLED(i, j, CRGB::Black);
      }
    }
  }
}

void drawActivePiece() {
  for (int thisPixel = 0; thisPixel < 4; thisPixel++) {
    int col = pieces[activeShape].rotations[currentRotation][thisPixel][0] + xOffset;
    int row = pieces[activeShape].rotations[currentRotation][thisPixel][1] + yOffset;

    if (row < 0 || col < 0) {
      continue;
    }

    setLED(row, col, pieces[activeShape].color);
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


  if (fullRowCount > 0) {

    // Blink all the filled rows.
    for (int blinkCount = 0; blinkCount < 3; blinkCount++) {
      for (int i = 0; i < fullRowCount; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
          setLED(fullRows[i], j, CRGB::Black);
        }
      }
      FastLED.show();
      delay(175);
      drawFixedMinos(BOARD_HEIGHT);
      FastLED.show();
      delay(175);
    }

    // Sideway wipe of all the filled rows.
    for (int columnClear = 0; columnClear < BOARD_WIDTH; columnClear++) {
      for (int clearRow = 0; clearRow < fullRowCount; clearRow++) {
        setLED(fullRows[clearRow], columnClear, CRGB::Black);
      }
      FastLED.show();
      delay(50);
    }

    //  remove the filled rows and drop them
    for (int i = 0; i < fullRowCount; i++) {
      for (int copyRow = fullRows[i]; copyRow > 0; copyRow--) {
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

    score = score + thePoints;

    if (totalLines % 10 == 0) {
      level++;
      gravityTrigger--;
    }
  }
}


void startGame() {
  FastLED.setBrightness(MATRIX_BRIGHTNESS);
  gameOver = false;
  stepCounter = 0;
  gravityTrigger = 20;
  level = 1;
  score = 0;
  totalLines = 0;
  clearBoard();
  launchNewShape();
}


void endGame() {
  printBoard();
  listenForInput = false;
  gameOver = true;
  currentBagSelection = EMPTY;

  for (int blinkCount = 0; blinkCount < 6; blinkCount++) {
    drawFixedMinos(BOARD_HEIGHT);

    for (int thisPixel = 0; thisPixel < 4; thisPixel++) {
      int col = pieces[activeShape].rotations[currentRotation][thisPixel][0] + xOffset;
      int row = pieces[activeShape].rotations[currentRotation][thisPixel][1] + yOffset;

      if (row < 0 || col < 0) {
        continue;
      }

      if (blinkCount % 2 == 0) {
        setLED(row, col, pieces[activeShape].color);
      } else {
        setLED(row, col, CRGB::Black);
      }
    }
    FastLED.show();
    delay(175);
  }

  for (int i = BOARD_HEIGHT - 1; i >= 0; i--) {
    drawPumpkinFace(0);
    drawFixedMinos(i);
    FastLED.show();
    delay(200);
  }

  clearLEDs();
  clearBoard();
  listenForInput = true;
}

#ifdef DEBUG
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

void printBag() {
  DPRINT("Shuffled Bag [");
  for (int i = 0; i < EMPTY; i++) {
    DPRINT(bagOfPieces[i]);
    DPRINT(",");
  }
  DPRINTLN("]");
}
#else
void printBoard() {}
void printBag() {}
#endif

void setLED(int row, int column, CRGB color) {
  int calculatedColumn;
  if (row % 2 == 0) {  //zig rows
    calculatedColumn = column;
  } else {  // zag rows
    calculatedColumn = (BOARD_WIDTH - 1) - column;
  }

  int number = row * BOARD_WIDTH + calculatedColumn;
  if (inverted) {
    number = (MATRIX_PIXELS - 1) - number;
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

int face[][BOARD_WIDTH] = {
  { 0, 0, 1, 0, 0, 0, 0, 1, 0, 0 },
  { 0, 1, 1, 1, 0, 0, 1, 1, 1, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 0, 0, 1, 0, 0, 0, 1, 0 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
  { 0, 0, 1, 1, 1, 1, 1, 1, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};
int faceHeight = sizeof(face) / sizeof(int[BOARD_WIDTH]);

void drawPumpkinFace(int stepCounter) {
  for (int i = 0; i < MATRIX_PIXELS; i++) {
    CRGB color = CRGB(80, 35, 0);
    int r = random(80);
    color = color - CRGB(r, r / 2, r / 2);
    leds[i] = color;
  }

  for (int i = 0; i < faceHeight; i++) {
    for (int j = 0; j < BOARD_WIDTH; j++) {
      if (face[i][j] == 1) {
        CRGB color;
        if (stepCounter % 180 > 90) {
          color = CRGB(33, 0, 33);
        } else {
          color = CRGB::Green;
        }
        setLED(i + (BOARD_HEIGHT / 2) - (faceHeight / 2), j, color);
      }
    }
  }
}

void drawDirectionArrows(int drawLength) {
  FastLED.setBrightness(MATRIX_BRIGHTNESS);
  clearLEDs();
  FastLED.show();

  int arrowHeight = BOARD_WIDTH / 2;

  for (int i = 0; i < BOARD_HEIGHT; i++) {
    if (i + arrowHeight >= BOARD_HEIGHT) {
      break;
    }

    if (i % 4 != 0) {
      continue;
    }

    for (int j = 0; j < arrowHeight; j++) {
      setLED((i + j), j, CRGB::Red);
      setLED((i + j), (BOARD_WIDTH - 1) - j, CRGB::Red);
    }
  }

  FastLED.show();

  delay(drawLength);

  clearLEDs();
  FastLED.show();
}

void paintAll(int timing, CRGB color) {
  for (int i = 0; i < MATRIX_PIXELS; i++) {
    leds[i] = color;
  }
  FastLED.show();
  delay(timing);
}