#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

constexpr uint8_t PIN_UP = 1;
constexpr uint8_t PIN_DOWN = 2;
constexpr uint8_t PIN_LEFT = 3;
constexpr uint8_t PIN_RIGHT = 4;
constexpr uint8_t PIN_A = 5;
constexpr uint8_t PIN_B = 6;
constexpr uint8_t PIN_CS = 17;
constexpr uint8_t PIN_SCK = 18;
constexpr uint8_t PIN_MOSI = 19;
constexpr uint8_t PIN_DC = 20;
constexpr uint8_t PIN_RST = 21;
constexpr uint8_t PIN_BL = 22;

Adafruit_ILI9341 tft(&SPI, PIN_DC, PIN_CS, PIN_RST);

enum Button : uint8_t { UP, DOWN, LEFT, RIGHT, A, B, BUTTON_COUNT };
const uint8_t buttonPins[BUTTON_COUNT] = {
  PIN_UP, PIN_DOWN, PIN_LEFT, PIN_RIGHT, PIN_A, PIN_B
};

bool buttonRaw[BUTTON_COUNT] = {};
bool buttonHeld[BUTTON_COUNT] = {};
bool buttonPressed[BUTTON_COUNT] = {};
uint32_t buttonChangedAt[BUTTON_COUNT] = {};

enum Screen : uint8_t { SPLASH, MENU, SNAKE, SNAKE_OVER, BRICKS, BRICKS_OVER };
Screen screen = SPLASH;
uint8_t menuChoice = 0;
uint32_t splashEndsAt = 0;

constexpr uint16_t BACKGROUND = ILI9341_BLACK;
constexpr uint16_t TEXT = ILI9341_WHITE;
constexpr uint16_t ACCENT = ILI9341_CYAN;
constexpr uint16_t DIM = ILI9341_DARKGREY;

void scanButtons() {
  const uint32_t now = millis();
  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    buttonPressed[i] = false;
    const bool raw = digitalRead(buttonPins[i]) == LOW;
    if (raw != buttonRaw[i]) {
      buttonRaw[i] = raw;
      buttonChangedAt[i] = now;
    }
    if (raw != buttonHeld[i] && now - buttonChangedAt[i] >= 25) {
      buttonHeld[i] = raw;
      buttonPressed[i] = raw;
    }
  }
}

void centeredText(const char *text, int16_t y, uint8_t size, uint16_t color) {
  tft.setTextSize(size);
  tft.setTextColor(color);
  const int16_t width = strlen(text) * 6 * size;
  tft.setCursor((320 - width) / 2, y);
  tft.print(text);
}

void drawHeader(const char *title, uint16_t score, const char *scoreLabel) {
  tft.fillRect(0, 0, 320, 28, ILI9341_NAVY);
  tft.setTextSize(2);
  tft.setTextColor(TEXT);
  tft.setCursor(8, 6);
  tft.print(title);
  tft.setTextSize(1);
  tft.setCursor(222, 10);
  tft.print(scoreLabel);
  tft.print(score);
}

void drawOverlay(const char *heading, const char *detail) {
  tft.fillRoundRect(37, 76, 246, 91, 9, ILI9341_NAVY);
  tft.drawRoundRect(37, 76, 246, 91, 9, ACCENT);
  centeredText(heading, 93, 2, TEXT);
  centeredText(detail, 127, 1, ILI9341_YELLOW);
  centeredText("A RETRY  /  B HOME", 147, 1, TEXT);
}

void drawSplash() {
  tft.fillScreen(BACKGROUND);
  tft.drawRoundRect(20, 24, 280, 192, 18, ACCENT);
  centeredText("POCKET", 62, 4, ILI9341_YELLOW);
  centeredText("GAMEBOX", 112, 3, ACCENT);
  centeredText("TWO GAMES. SIX BUTTONS.", 178, 1, TEXT);
}

void drawMenu() {
  screen = MENU;
  tft.fillScreen(BACKGROUND);
  centeredText("POCKET GAMEBOX", 17, 2, ACCENT);
  centeredText("CHOOSE A GAME", 50, 1, DIM);

  const char *games[2] = { "SNAKE", "BRICK BREAKER" };
  for (uint8_t i = 0; i < 2; ++i) {
    const int16_t y = 77 + 57 * i;
    const uint16_t fill = i == menuChoice ? ILI9341_NAVY : ILI9341_DARKGREY;
    tft.fillRoundRect(26, y, 268, 45, 7, fill);
    tft.drawRoundRect(26, y, 268, 45, 7,
                      i == menuChoice ? ILI9341_YELLOW : DIM);
    centeredText(games[i], y + 14, 2, TEXT);
  }
  centeredText("UP/DOWN: CHOOSE   A: PLAY", 208, 1, TEXT);
  centeredText("B: HOME FROM ANY GAME", 223, 1, DIM);
}

constexpr uint8_t SNAKE_COLS = 16;
constexpr uint8_t SNAKE_ROWS = 12;
constexpr uint8_t SNAKE_CELL = 16;
constexpr int16_t SNAKE_X = 32;
constexpr int16_t SNAKE_Y = 35;
int8_t snakeX[SNAKE_COLS * SNAKE_ROWS];
int8_t snakeY[SNAKE_COLS * SNAKE_ROWS];
uint16_t snakeLength = 0;
uint16_t snakeScore = 0;
int8_t foodX = 0;
int8_t foodY = 0;
uint8_t snakeDirection = 1;
uint8_t nextSnakeDirection = 1;
uint32_t snakeLastStep = 0;

bool snakeOccupies(int8_t x, int8_t y) {
  for (uint16_t i = 0; i < snakeLength; ++i) {
    if (snakeX[i] == x && snakeY[i] == y) return true;
  }
  return false;
}

void placeFood() {
  if (snakeLength == SNAKE_COLS * SNAKE_ROWS) return;
  do {
    foodX = random(SNAKE_COLS);
    foodY = random(SNAKE_ROWS);
  } while (snakeOccupies(foodX, foodY));
}

void drawSnake() {
  tft.fillRect(SNAKE_X, SNAKE_Y,
               SNAKE_COLS * SNAKE_CELL, SNAKE_ROWS * SNAKE_CELL,
               ILI9341_NAVY);
  tft.drawRect(SNAKE_X - 1, SNAKE_Y - 1,
               SNAKE_COLS * SNAKE_CELL + 2, SNAKE_ROWS * SNAKE_CELL + 2,
               DIM);
  tft.fillCircle(SNAKE_X + foodX * SNAKE_CELL + SNAKE_CELL / 2,
                 SNAKE_Y + foodY * SNAKE_CELL + SNAKE_CELL / 2,
                 6, ILI9341_RED);
  for (int16_t i = snakeLength - 1; i >= 0; --i) {
    tft.fillRoundRect(SNAKE_X + snakeX[i] * SNAKE_CELL + 1,
                      SNAKE_Y + snakeY[i] * SNAKE_CELL + 1,
                      SNAKE_CELL - 2, SNAKE_CELL - 2, 3,
                      i == 0 ? ILI9341_YELLOW : ILI9341_GREEN);
  }
}

void startSnake() {
  screen = SNAKE;
  snakeLength = 3;
  snakeScore = 0;
  snakeX[0] = 8; snakeY[0] = 6;
  snakeX[1] = 7; snakeY[1] = 6;
  snakeX[2] = 6; snakeY[2] = 6;
  snakeDirection = 1;
  nextSnakeDirection = 1;
  snakeLastStep = millis();
  placeFood();
  tft.fillScreen(BACKGROUND);
  drawHeader("SNAKE", snakeScore, "SCORE ");
  drawSnake();
}

void requestSnakeTurn(uint8_t direction) {
  if ((direction + 2) % 4 != snakeDirection) nextSnakeDirection = direction;
}

void advanceSnake() {
  snakeDirection = nextSnakeDirection;
  int8_t x = snakeX[0];
  int8_t y = snakeY[0];
  if (snakeDirection == 0) --y;
  if (snakeDirection == 1) ++x;
  if (snakeDirection == 2) ++y;
  if (snakeDirection == 3) --x;

  const bool grows = x == foodX && y == foodY;
  bool collision = x < 0 || x >= SNAKE_COLS || y < 0 || y >= SNAKE_ROWS;
  const uint16_t checked = snakeLength - (grows ? 0 : 1);
  for (uint16_t i = 0; i < checked; ++i) {
    if (snakeX[i] == x && snakeY[i] == y) collision = true;
  }
  if (collision) {
    screen = SNAKE_OVER;
    drawOverlay("GAME OVER", "THE SNAKE CRASHED");
    return;
  }

  if (grows) ++snakeLength;
  for (int16_t i = snakeLength - 1; i > 0; --i) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }
  snakeX[0] = x;
  snakeY[0] = y;
  if (grows) {
    ++snakeScore;
    drawHeader("SNAKE", snakeScore, "SCORE ");
    if (snakeLength == SNAKE_COLS * SNAKE_ROWS) {
      screen = SNAKE_OVER;
      drawSnake();
      drawOverlay("YOU WIN!", "THE BOARD IS FULL");
      return;
    }
    placeFood();
  }
  drawSnake();
}

constexpr int16_t PADDLE_Y = 218;
constexpr int16_t PADDLE_W = 50;
constexpr int16_t PADDLE_H = 7;
constexpr int16_t BALL_R = 4;
constexpr uint8_t BRICK_ROWS = 4;
constexpr uint8_t BRICK_COLS = 8;
bool bricks[BRICK_ROWS][BRICK_COLS];
int16_t paddleX = 135;
float ballX = 160;
float ballY = 207;
float ballVX = 2;
float ballVY = -2.8f;
bool ballAttached = true;
uint8_t lives = 3;
uint8_t bricksRemaining = 0;
uint16_t brickScore = 0;
uint32_t brickLastFrame = 0;

int16_t brickX(uint8_t col) { return 16 + col * 36; }
int16_t brickY(uint8_t row) { return 40 + row * 18; }

void drawBrickHeader() {
  drawHeader("BRICKS", brickScore, "SCORE ");
  tft.fillRect(0, 28, 320, 10, BACKGROUND);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(264, 30);
  tft.print("LIVES ");
  tft.print(lives);
}

void drawPaddleAndBall() {
  tft.fillRoundRect(paddleX, PADDLE_Y, PADDLE_W, PADDLE_H, 3, ACCENT);
  tft.fillCircle((int16_t)ballX, (int16_t)ballY, BALL_R, ILI9341_YELLOW);
}

void attachBall() {
  ballAttached = true;
  ballX = paddleX + PADDLE_W / 2;
  ballY = PADDLE_Y - BALL_R - 3;
  ballVX = 2;
  ballVY = -2.8f;
}

void startBricks() {
  screen = BRICKS;
  lives = 3;
  brickScore = 0;
  bricksRemaining = BRICK_ROWS * BRICK_COLS;
  paddleX = 135;
  attachBall();
  brickLastFrame = millis();
  tft.fillScreen(BACKGROUND);
  drawBrickHeader();
  for (uint8_t row = 0; row < BRICK_ROWS; ++row) {
    for (uint8_t col = 0; col < BRICK_COLS; ++col) {
      bricks[row][col] = true;
      const uint16_t color = row < 2 ? ILI9341_ORANGE : ILI9341_GREEN;
      tft.fillRoundRect(brickX(col), brickY(row), 32, 13, 3, color);
    }
  }
  drawPaddleAndBall();
  centeredText("A: LAUNCH   B: HOME", 184, 1, DIM);
}

void advanceBricks() {
  tft.fillRect(paddleX - 1, PADDLE_Y - 1, PADDLE_W + 2, PADDLE_H + 2,
               BACKGROUND);
  tft.fillCircle((int16_t)ballX, (int16_t)ballY, BALL_R, BACKGROUND);

  if (buttonHeld[LEFT]) paddleX -= 5;
  if (buttonHeld[RIGHT]) paddleX += 5;
  if (paddleX < 4) paddleX = 4;
  if (paddleX > 320 - PADDLE_W - 4) paddleX = 320 - PADDLE_W - 4;

  if (ballAttached) {
    ballX = paddleX + PADDLE_W / 2;
    ballY = PADDLE_Y - BALL_R - 3;
    if (buttonPressed[A]) {
      ballAttached = false;
      tft.fillRect(0, 182, 320, 13, BACKGROUND);
    }
  } else {
    const float previousY = ballY;
    ballX += ballVX;
    ballY += ballVY;

    if (ballX <= BALL_R || ballX >= 320 - BALL_R) {
      ballVX = -ballVX;
      ballX = constrain(ballX, BALL_R, 320 - BALL_R);
    }
    if (ballY <= 39) {
      ballVY = -ballVY;
      ballY = 39;
    }

    if (ballVY > 0 && previousY + BALL_R < PADDLE_Y &&
        ballY + BALL_R >= PADDLE_Y && ballX >= paddleX - BALL_R &&
        ballX <= paddleX + PADDLE_W + BALL_R) {
      ballY = PADDLE_Y - BALL_R;
      ballVY = -abs(ballVY);
      ballVX = (ballX - (paddleX + PADDLE_W / 2)) * 0.11f;
      if (ballVX > -0.6f && ballVX < 0.6f) ballVX = 0.6f;
    }

    bool hit = false;
    for (uint8_t row = 0; row < BRICK_ROWS && !hit; ++row) {
      for (uint8_t col = 0; col < BRICK_COLS; ++col) {
        if (!bricks[row][col]) continue;
        if (ballX + BALL_R >= brickX(col) &&
            ballX - BALL_R <= brickX(col) + 32 &&
            ballY + BALL_R >= brickY(row) &&
            ballY - BALL_R <= brickY(row) + 13) {
          bricks[row][col] = false;
          --bricksRemaining;
          brickScore += 10;
          tft.fillRect(brickX(col), brickY(row), 32, 13, BACKGROUND);
          ballVY = -ballVY;
          drawBrickHeader();
          hit = true;
          break;
        }
      }
    }
    if (bricksRemaining == 0) {
      screen = BRICKS_OVER;
      drawOverlay("YOU WIN!", "ALL BRICKS CLEARED");
      return;
    }

    if (ballY - BALL_R > 240) {
      --lives;
      drawBrickHeader();
      if (lives == 0) {
        screen = BRICKS_OVER;
        drawOverlay("GAME OVER", "OUT OF LIVES");
        return;
      }
      attachBall();
      centeredText("A: LAUNCH   B: HOME", 184, 1, DIM);
    }
  }
  drawPaddleAndBall();
}

void setup() {
  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    buttonRaw[i] = digitalRead(buttonPins[i]) == LOW;
    buttonHeld[i] = buttonRaw[i];
    buttonChangedAt[i] = millis();
  }

  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, LOW);
  SPI.setSCK(PIN_SCK);
  SPI.setTX(PIN_MOSI);
  tft.begin(24000000);
  tft.setRotation(1);
  digitalWrite(PIN_BL, HIGH);
  randomSeed(micros());
  drawSplash();
  splashEndsAt = millis() + 1200;
}

void loop() {
  scanButtons();
  const uint32_t now = millis();

  if (screen == SPLASH) {
    if ((int32_t)(now - splashEndsAt) >= 0 || buttonPressed[A]) drawMenu();
    return;
  }

  if (screen == MENU) {
    if (buttonPressed[UP] || buttonPressed[DOWN] ||
        buttonPressed[LEFT] || buttonPressed[RIGHT]) {
      menuChoice ^= 1;
      drawMenu();
    }
    if (buttonPressed[A]) {
      if (menuChoice == 0) startSnake();
      else startBricks();
    }
    return;
  }

  if (buttonPressed[B]) {
    drawMenu();
    return;
  }

  if (screen == SNAKE_OVER) {
    if (buttonPressed[A]) startSnake();
    return;
  }
  if (screen == BRICKS_OVER) {
    if (buttonPressed[A]) startBricks();
    return;
  }

  if (screen == SNAKE) {
    if (buttonPressed[UP]) requestSnakeTurn(0);
    if (buttonPressed[RIGHT]) requestSnakeTurn(1);
    if (buttonPressed[DOWN]) requestSnakeTurn(2);
    if (buttonPressed[LEFT]) requestSnakeTurn(3);
    const uint16_t interval = max(85, 160 - (int)snakeScore * 3);
    if (now - snakeLastStep >= interval) {
      snakeLastStep = now;
      advanceSnake();
    }
    return;
  }

  if (screen == BRICKS && now - brickLastFrame >= 20) {
    brickLastFrame = now;
    advanceBricks();
  }
}
