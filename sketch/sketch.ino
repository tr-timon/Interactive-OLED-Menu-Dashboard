#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Pin Definitions
const int BTN_JUMP    = A0;
const int BTN_RESTART = A3;
const int LED_SCORE   = 12; // Lights up ONLY when a NEW HIGH SCORE is broken
const int LED_CRASH   = 11; // Lights up on GAME OVER
const int HIGH_SCORE_ADDR = 0;

// Pixel Art Bitmaps (8x8)
const unsigned char PROGMEM dinoSprite[] = {
  0b00001110,
  0b00001101,
  0b00001111,
  0b00100110,
  0b00111110,
  0b00011100,
  0b00010100,
  0b00010100
};

const unsigned char PROGMEM birdSprite[] = {
  0b00100000,
  0b01110000,
  0b11111100,
  0b00111110,
  0b00011100,
  0b00001000,
  0b00000000,
  0b00000000
};

// Game Variables
int dinoY = 48;
bool isJumping = false;
float jumpSpeed = 0;

float obstacleX = 128;
int obstacleY = 48;
int obstacleWidth = 6;
int obstacleHeight = 8;
bool isBird = false;

int score = 0;
int highScore = 0;
bool newHighScoreBeaten = false;

bool isGameOver = false;
bool isWaitingToStart = true;

int obstacleSpeed = 5;
int gameDelay = 35;

void setup() {
  pinMode(BTN_JUMP, INPUT_PULLUP);
  pinMode(BTN_RESTART, INPUT_PULLUP);
  pinMode(LED_SCORE, OUTPUT);
  pinMode(LED_CRASH, OUTPUT);

  digitalWrite(LED_SCORE, LOW);
  digitalWrite(LED_CRASH, LOW);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Load EEPROM High Score
  EEPROM.get(HIGH_SCORE_ADDR, highScore);
  if (highScore < 0 || highScore > 30000) {
    highScore = 0;
    EEPROM.put(HIGH_SCORE_ADDR, highScore);
  }

  showStartScreen();
}

void loop() {
  // --- STATE 1: Waiting to Start ---
  if (isWaitingToStart) {
    // EEPROM Reset Hotkey: Hold JUMP + RESTART together
    if (digitalRead(BTN_JUMP) == LOW && digitalRead(BTN_RESTART) == LOW) {
      highScore = 0;
      EEPROM.put(HIGH_SCORE_ADDR, 0);
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(15, 28);
      display.println("HIGH SCORE RESET!");
      display.display();
      delay(1200);
      showStartScreen();
    }
    
    if (digitalRead(BTN_RESTART) == LOW) {
      delay(200);
      runCountdown();
      resetGame();
    }
    return;
  }

  // --- STATE 2: Game Over ---
  if (isGameOver) {
    if (digitalRead(BTN_RESTART) == LOW) {
      delay(200);
      digitalWrite(LED_CRASH, LOW);
      digitalWrite(LED_SCORE, LOW);
      runCountdown();
      resetGame();
    }
    return;
  }

  // --- STATE 3: Active Gameplay ---
  if (digitalRead(BTN_JUMP) == LOW && !isJumping) {
    isJumping = true;
    jumpSpeed = -10;
  }

  // Jump Physics
  if (isJumping) {
    dinoY += jumpSpeed;
    jumpSpeed += 1.5;
    if (dinoY >= 48) {
      dinoY = 48;
      isJumping = false;
    }
  }

  // Dynamic Difficulty
  obstacleSpeed = 5 + (score / 200);
  if (obstacleSpeed > 12) obstacleSpeed = 12;

  gameDelay = 35 - (score / 150);
  if (gameDelay < 10) gameDelay = 10;

  // Obstacle Motion & Spawning
  obstacleX -= obstacleSpeed;
  if (obstacleX < 0) {
    obstacleX = 128;
    score += 50;

    // Check if high score has been surpassed
    if (highScore > 0 && score > highScore) {
      newHighScoreBeaten = true;
      digitalWrite(LED_SCORE, HIGH); // Turn ON High Score LED!
    }

    // 30% chance for flying bird once score > 200
    if (random(0, 100) < 30 && score > 200) {
      isBird = true;
      obstacleY = 34;
      obstacleWidth = 8;
      obstacleHeight = 8;
    } else {
      isBird = false;
      obstacleY = 48;
      obstacleWidth = 6;
      obstacleHeight = 8;
    }
  }

  // Collision Detection
  if (obstacleX >= 10 && obstacleX <= 22) {
    if (abs(dinoY - obstacleY) < 7) {
      isGameOver = true;
      digitalWrite(LED_CRASH, HIGH);

      if (score > highScore) {
        highScore = score;
        EEPROM.put(HIGH_SCORE_ADDR, highScore);
      }

      showGameOverScreen();
      return;
    }
  }

  // Frame Rendering
  display.clearDisplay();
  display.drawLine(0, 56, 128, 56, SSD1306_WHITE); // Ground

  // Draw Dino Bitmap
  display.drawBitmap(16, dinoY, dinoSprite, 8, 8, SSD1306_WHITE);

  // Draw Obstacle
  if (isBird) {
    display.drawBitmap((int)obstacleX, obstacleY, birdSprite, 8, 8, SSD1306_WHITE);
  } else {
    display.fillRect((int)obstacleX, obstacleY, obstacleWidth, obstacleHeight, SSD1306_WHITE);
  }

  // HUD Display
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("HI:"); display.print(highScore);
  display.setCursor(72, 0);
  display.print("SC:"); display.print(score);

  display.display();
  delay(gameDelay);
}

// Initial Screen Shown on Boot
void showStartScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 5);
  display.println("DINO GAME");

  display.setTextSize(1);
  display.setCursor(25, 26);
  display.print("Best: "); display.print(highScore);

  display.setTextSize(2);
  display.setCursor(34, 45);
  display.println("START"); // Displays "START" on boot
  display.display();
}

// Countdown Routine (Keeps GO!)
void runCountdown() {
  for (int i = 3; i > 0; i--) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(32, 10);
    display.println("GET READY!");

    display.setTextSize(3);
    display.setCursor(56, 30);
    display.print(i);
    display.display();
    delay(800);
  }

  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(40, 25);
  display.println("GO!"); // Retained "GO!"
  display.display();
  delay(500);
}

void resetGame() {
  dinoY = 48;
  isJumping = false;
  jumpSpeed = 0;
  obstacleX = 128;
  obstacleY = 48;
  isBird = false;
  score = 0;
  newHighScoreBeaten = false;
  obstacleSpeed = 5;
  gameDelay = 35;
  isGameOver = false;
  isWaitingToStart = false;
  
  digitalWrite(LED_SCORE, LOW);
  digitalWrite(LED_CRASH, LOW);
}

void showGameOverScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(8, 2);
  display.println("GAME OVER!");

  display.setTextSize(1);
  display.setCursor(20, 24);
  display.print("Score: "); display.print(score);

  display.setCursor(20, 36);
  display.print("Best:  "); display.print(highScore);

  display.setCursor(2, 52);
  display.println("Press RESTART button");
  display.display();
}