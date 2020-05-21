#include <FastLED.h>

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    5
#define CLK_PIN   7
#define LED_TYPE    WS2801
#define COLOR_ORDER RGB
#define NUM_LEDS    100
//#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define BRIGHTNESS         255
#define FRAMES_PER_SECOND  30
#define JOYSTICK_X 3
#define JOYSTICK_Y 2


CRGB leds[NUM_LEDS];
uint8_t globvals[4*NUM_LEDS];//speed, reset, hue
unsigned long actual_time;
unsigned long previous_time;
unsigned long interval=1000/FRAMES_PER_SECOND;
uint8_t snake_speed = 1;
const short initialSnakeLength = 3;
uint8_t game_cycle = 0;
bool game_running = false;

struct Point {
  int row = 0, col = 0;
  Point(int row = 0, int col = 0): row(row), col(col) {}
};

struct Coordinate {
  int x = 0, y = 0;
  Coordinate(int x = 0, int y = 0): x(x), y(y) {}
};



void setup() {
  //Serial.begin(9600);
  for (int i=0;i<NUM_LEDS;i++){
    for (int j=0;j<4;j++){
      globvals[i*4+j]=0;
    }
  }
  //initialize();         // initialize pins & LED matrix
  
// construct with default values in case the user turns off the calibration
  Coordinate joystickHome(500, 500);
  calibrateJoystick(); // calibrate the joystick home (do not touch it)
  
  delay(500); // 3 second delay for recovery
  FastLED.setDither(0);
  
  // tell FastLED about the LED strip configuration
  //FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalPixelString);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  previous_time=millis();
}


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
//SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };

SimplePatternList gPatterns = {smooth_confetti, ring_burst};
//SimplePatternList gPatterns = {smooth_confetti};

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t gHue2 = 0; // rotating "base color" used by many of the patterns

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

const uint8_t kMatrixWidth = 10;
const uint8_t kMatrixHeight = 10;
const bool    kMatrixSerpentineLayout = true;

uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;
  
  if( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }

  if( kMatrixSerpentineLayout == true) {
    if( y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }
  
  return i;
}
  
void loop(){ 
  //print_debug();
  if (game_running == false){
    if (analogRead(JOYSTICK_X) >900){
      initialize_snake();
      game_running = true;
    }
  }
  actual_time=millis();
  if (actual_time - previous_time > interval){
    // Call the current pattern function once, updating the 'leds' array
    if (game_running == true){
      snake_game_loop();
    }
    else{
      gPatterns[gCurrentPatternNumber]();
    }
    // send the 'leds' array out to the actual LED strip
    FastLED.show();
    previous_time=actual_time;
  }
  // do some periodic updates
  EVERY_N_MILLISECONDS( 150 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_MILLISECONDS( 20 ) { gHue2++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 120 ) { nextPattern(); } // change patterns periodically
  EVERY_N_SECONDS( 5 ) {increase_snake_speed();}
}

void print_debug(){
  Serial.print("Debug X ");
  Serial.print(analogRead(JOYSTICK_X));
  delay(400);
  Serial.print("Debug Y ");
  Serial.print(analogRead(JOYSTICK_Y));
  delay(400);
  Serial.println();
  Serial.println();
}

void smooth_confetti()
{
  for (int i=0;i<NUM_LEDS;i++){
    uint8_t l_speed = globvals[i*4];
    uint8_t l_reset = globvals[i*4+1];
    uint8_t l_hue = globvals[i*4+2];
    uint8_t l_position = globvals[i*4+3];
    if (l_reset==0){
      //globvals[i*4]=1;
      globvals[i*4]=random8(1,5);//set new random speed
      globvals[i*4+1]=1;//unset reset bit
      globvals[i*4+2]=gHue;
      globvals[i*4+3]=0;//reset position to 0
    }
    else{
      if ((uint8_t)(l_position+l_speed) < l_position){//overflow at the end
        globvals[i*4+1]=0; //set reset bit
      }
      else{
        //leds[i]=CHSV(l_hue,200,cubicwave8((uint8_t)l_position/60));
        leds[i]=CHSV(l_hue,200,cubicwave8(l_position));
        globvals[i*4+3]=l_position+l_speed;
      }
    }
  }
}

void ring_burst()
{
  leds[44]=leds[45]=leds[54]=leds[55]=CHSV(gHue2,255,255);//Center

  //first ring
  for (int i=4; i<6; i++){
    leds[i*10+3]=leds[i*10+6]=CHSV(gHue2-20,255,255);
  }
  for (int i=3; i<7; i++){
    leds[30+i]=leds[60+i]=CHSV(gHue2-20,255,255);
  }

  //second ring
  for (int i=3; i<7; i++){
    leds[i*10+2]=leds[i*10+7]=CHSV(gHue2-40,255,255);
  }
  for (int i=2; i<8; i++){
    leds[20+i]=leds[70+i]=CHSV(gHue2-40,255,255);
  }

  //third ring
  for (int i=2; i<8; i++){
    leds[i*10+1]=leds[i*10+8]=CHSV(gHue2-60,255,255);
  }
  for (int i=1; i<9; i++){
    leds[10+i]=leds[80+i]=CHSV(gHue2-60,255,255);

  }

  
  //fourth ring
  for (int i=1; i<9; i++){
    leds[i*10]=leds[i*10+9]=CHSV(gHue2-80,255,255);
  }
  for (int i=0; i<10; i++){
    leds[i]=leds[90+i]=CHSV(gHue2-80,255,255);

  }
}

void snake_game_loop() {
  generateFood();    // if there is no food, generate one
  scanJoystick();    // watches joystick movements & blinks with food
  game_cycle++;
  if (game_cycle > (FRAMES_PER_SECOND-snake_speed)){
    calculateSnake();  // calculates snake parameters
    handleGameStates();
    game_cycle = 0;
  }
}


bool win = false;
bool gameOver = false;

// primary snake head coordinates (snake head), it will be randomly generated
Point snake;

// food is not anywhere yet
Point food(-1, -1);

// construct with default values in case the user turns off the calibration
Coordinate joystickHome(500, 500);

// snake parameters
int snakeLength = initialSnakeLength; // choosed by the user in the config section
int snakeDirection = 0; // if it is 0, the snake does not move

// direction constants
const short up     = 1;
const short right  = 2;
const short down   = 3; // 'down - 2' must be 'up'
const short left   = 4; // 'left - 2' must be 'right'

// threshold where movement of the joystick will be accepted
const int joystickThreshold = 200;

// artificial logarithmity (steepness) of the potentiometer (-1 = linear, 1 = natural, bigger = steeper (recommended 0...1))
const float logarithmity = 0.4;

// snake body segments storage
int gameboard[10][10] = {};




// --------------------------------------------------------------- //
// -------------------------- functions -------------------------- //
// --------------------------------------------------------------- //


// if there is no food, generate one, also check for victory
void generateFood() {
  if (food.row == -1 || food.col == -1) {
    // self-explanatory
    if (snakeLength >= 100) {
      win = true;
      return; // prevent the food generator from running, in this case it would run forever, because it will not be able to find a pixel without a snake
    }

    // generate food until it is in the right position
    do {
      food.col = random(10);
      food.row = random(10);
    } while (gameboard[food.row][food.col] > 0);
    gameboard[food.row][food.col]=-1;
  }
}


// watches joystick movements
void scanJoystick() {
  int previousDirection = snakeDirection; // save the last direction

    // determine the direction of the snake
    analogRead(JOYSTICK_Y) < joystickHome.y - joystickThreshold ? snakeDirection = up    : 0;
    analogRead(JOYSTICK_Y) > joystickHome.y + joystickThreshold ? snakeDirection = down  : 0;
    analogRead(JOYSTICK_X) < joystickHome.x - joystickThreshold ? snakeDirection = left  : 0;
    analogRead(JOYSTICK_X) > joystickHome.x + joystickThreshold ? snakeDirection = right : 0;

    // ignore directional change by 180 degrees (no effect for non-moving snake)
    snakeDirection + 2 == previousDirection && previousDirection != 0 ? snakeDirection = previousDirection : 0;
    snakeDirection - 2 == previousDirection && previousDirection != 0 ? snakeDirection = previousDirection : 0;

    // display food
    leds[XY (food.row,food.col)].setRGB( 255, 0, 0);
}


// calculate snake movement data
void calculateSnake() {
  switch (snakeDirection) {
    case up:
      snake.row--;
      fixEdge();
      leds[ XY (snake.row, snake.col)].setRGB(0,255,0);
      break;

    case right:
      snake.col++;
      fixEdge();
      leds[ XY (snake.row, snake.col)].setRGB(0,255,0);
      break;

    case down:
      snake.row++;
      fixEdge();
      leds[ XY (snake.row, snake.col)].setRGB(0,255,0);
      break;

    case left:
      snake.col--;
      fixEdge();
      leds[ XY (snake.row, snake.col)].setRGB(0,255,0);
      break;

    default: // if the snake is not moving, exit
      return;
  }

  // if there is a snake body segment, this will cause the end of the game (snake must be moving)
  if (gameboard[snake.row][snake.col] > 1 && snakeDirection != 0) {
    gameOver = true;
    return;
  }

  // check if the food was eaten
  if (snake.row == food.row && snake.col == food.col) {
    food.row = -1; // reset food
    food.col = -1;

    // increment snake length
    snakeLength++;

    // increment all the snake body segments
    for (int row = 0; row < 10; row++) {
      for (int col = 0; col < 10; col++) {
        if (gameboard[row][col] > 0 ) {
          gameboard[row][col]++;
        }
      }
    }
  }

  // add new segment at the snake head location
  gameboard[snake.row][snake.col] = snakeLength + 1; // will be decremented in a moment

  // decrement all the snake body segments, if segment is 0, turn the corresponding led off
  for (int row = 0; row < 10; row++) {
    for (int col = 0; col < 10; col++) {
      // if there is a body segment, decrement it's value
      if (gameboard[row][col] > 0 ) {
        gameboard[row][col]--;
      }

      // display the current pixel
      if (gameboard[row][col] ==0){
        leds[XY (row,col)].setRGB(0,0,0);
        }
      else if (gameboard[row][col] == -1){
        leds[XY (row,col)].setRGB(255,0,0);
      }
      else{
        leds[XY (row,col)].setRGB(0,255,0);
        }
      //matrix.setLed(0, row, col, gameboard[row][col] == 0 ? 0 : 1);
    }
  }
}


// causes the snake to appear on the other side of the screen if it gets out of the edge
void fixEdge() {
  snake.col < 0 ? snake.col += 10 : 0;
  snake.col > 9 ? snake.col -= 10 : 0;
  snake.row < 0 ? snake.row += 10 : 0;
  snake.row > 9 ? snake.row -= 10 : 0;
}


void handleGameStates() {
  if (gameOver || win) {
    //unrollSnake();

    //showScoreMessage(snakeLength - initialSnakeLength);

    //if (gameOver) showGameOverMessage();
    if (win)
    {
      display_win();
    }

    // re-init the game
    win = false;
    gameOver = false;
    snake.row = random(10);
    snake.col = random(10);
    food.row = -1;
    food.col = -1;
    snakeLength = initialSnakeLength;
    snakeDirection = 0;
    memset(gameboard, 0, sizeof(gameboard[0][0]) * 10 * 10);
    for (int i=0; i<100; i++){
      leds[i].setRGB(0,0,0);
      }
    game_running = false;
    }
}


void display_win(){
   fill_solid( &(leds[1]), 1, CRGB( 255, 255, 255) );
   FastLED.show();
   delay(5000);
}

//void unrollSnake() {
  /*
  // switch off the food LED
  //matrix.setLed(0, food.row, food.col, 0);

  delay(800);

  // flash the screen 5 times
  for (int i = 0; i < 5; i++) {
    // invert the screen
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        matrix.setLed(0, row, col, gameboard[row][col] == 0 ? 1 : 0);
      }
    }

    delay(20);

    // invert it back
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        matrix.setLed(0, row, col, gameboard[row][col] == 0 ? 0 : 1);
      }
    }

    delay(50);

  }


  delay(600);

  for (int i = 1; i <= snakeLength; i++) {
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        if (gameboard[row][col] == i) {
          matrix.setLed(0, row, col, 0);
          delay(100);
        }
      }
    }
  }
  */
//}


// calibrate the joystick home for 10 times
void calibrateJoystick() {
  Coordinate values;

  for (int i = 0; i < 10; i++) {
    values.x += analogRead(JOYSTICK_X);
    values.y += analogRead(JOYSTICK_Y);
  }

  joystickHome.x = values.x / 10;
  joystickHome.y = values.y / 10;
}

void initialize_snake() {
  snake_speed = 1;
  randomSeed(analogRead(10));
  snake.row = random(10);
  snake.col = random(10);
}

void increase_snake_speed(){

  if (snake_speed<FRAMES_PER_SECOND){
    snake_speed++;
  }
}
