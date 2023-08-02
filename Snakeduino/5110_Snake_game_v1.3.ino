#include <LCD5110_Graph.h>
#include <EEPROM.h>
#define SAG 1
#define SOL 2
#define ASAGI 3
#define YUKARI 4
#define BOS 5
#define MAX_SNAKE_SIZE 100
#define SNAKE_Y_BOUNDARY 40
#define MAX_MENU_ROWS 7
#define MAIN_MENU_MODE 0
#define GAME_MODE 1
#define SECENEKLER_MENU_MODE 2
#define SCOREBOARD_MODE 3
#define BACKLIGHT_ON 0
#define BACKLIGHT_OFF 1
#define BACKLIGHT_SMART 2
#define SOUND_OFF 0
#define SOUND_ON 1
#define SCOREBOARDSIZE 6
const int interruptPin = 3;
const int controllerAnalogPin = A0;
const int buzzerPin = 7;
const int backLightPin = 4;
const int ldrPin = A5;
int buzzerTimeHold = 0;
short buzzerDuration = 500;
bool buzzerState = 0;
unsigned long int moveTimeHold = 0;
byte moveDuration = 50;
bool soundOnOff = SOUND_OFF;
int scoreBoard [SCOREBOARDSIZE] = {0, 0, 0, 0, 0, 0};
LCD5110 myGLCD(8, 9, 10, 11, 12);
class snake {
  public:
    byte snakeCoordinatesX[MAX_SNAKE_SIZE];
    byte snakeCoordinatesY[MAX_SNAKE_SIZE];
    byte yon;
    byte size;
    byte yasakYon;
    int score;
    byte foodX;
    byte foodY;
    bool gameIsOn;

    snake(byte firstDirection, byte firstSize);
    void changeDirection(byte newDirection);
    void move();
    void display();
    void setYasakYon();
    void increaseSize();
    void calculateScore();
    void displayScoreHUD();
    void spawnFood();
    void checkIfGameIsOver();
    void gameOverAnimation();
    void resetSnake();
};
class controller {
  public:
    int rawAnalogInput;
    byte analogInputPin;
    byte controllerOutput;
    bool tusBasildi;

    void getAnalogInput();
    void decodeAnalogInput();
    controller(byte pin);
};
byte eskiInput;
class menu {
  public:
    byte menuItemCounter;
    byte totalRows;
    char menuStrings[MAX_MENU_ROWS][12];

    menu(byte rows, char row1string[], char row2string[], char row3string[], char row4string[], char row5string[], char row6string[]);
    menu(byte rows, char row1string[], char row2string[], char row3string[], char row4string[], char row5string[]);
    menu(byte rows, char row1string[], char row2string[], char row3string[], char row4string[]);
    menu(byte rows, char row1string[], char row2string[], char row3string[]);
    menu(byte rows, char row1string[], char row2string[]);

    void display();
    void increaseMenuItemCounter();
    void decreaseMenuItemCounter();
};

byte mode = MAIN_MENU_MODE;
byte backLightMode = BACKLIGHT_OFF ;
extern uint8_t TinyFont[];
extern uint8_t SmallFont[];
extern uint8_t snake2[];
snake yilan(SAG, 10);
controller tuslar(A0);
menu mainMenu (6, "Yeni oyun", "Secenekler", "Skorlar", "Zorluk", "Harita", "Hakkinda");
menu seceneklerMenu (2 , "Arka isik", "Ses");
void setup() {

  pinMode(backLightPin, OUTPUT);
  pinMode(interruptPin, INPUT);

  Serial.begin(9600);
  myGLCD.InitLCD();
  myGLCD.setFont(TinyFont);
  randomSeed(analogRead(3)); //Feedback from interrupt pin, less random since it is pulled down. Must change.
  attachInterrupt(digitalPinToInterrupt(interruptPin), ISR_Handle , RISING);
  digitalWrite(backLightPin , HIGH);
  myGLCD.drawBitmap(0, 0, snake2, 84, 48);
  myGLCD.update();
  readScoresFromEEPROM(scoreBoard, SCOREBOARDSIZE);
  delay(4000);

}
//*****************************************************************************************************************************************************************
void loop() {
  while (mode == MAIN_MENU_MODE) {
    mainMenu.display();
    backLightHandler();
    if (tuslar.tusBasildi) {
      tuslar.tusBasildi = 0;
      delay(100);

      switch (tuslar.controllerOutput) {
        case YUKARI :
          mainMenu.decreaseMenuItemCounter();
          //Serial.println(mainMenu.menuItemCounter);
          break;
        case ASAGI :
          mainMenu.increaseMenuItemCounter();
          //Serial.println(mainMenu.menuItemCounter);
          break;
        case SAG :
          switch (mainMenu.menuItemCounter) {
            case 0:
              mode = GAME_MODE;
              break;
            case 1:
              mode = SECENEKLER_MENU_MODE;
              break;
            case 2:
              mode = SCOREBOARD_MODE;
              break;
              // case 3:
              //    mode = ZORLUK_AYAR_MODE;
              //    break;
          }
          break;
      }
      tuslar.tusBasildi = 0;
    }
    delay(50);
  }
  /////////

  while (mode == SECENEKLER_MENU_MODE) {
    seceneklerMenu.display();
    backLightHandler();
    if (tuslar.tusBasildi) {
      tuslar.tusBasildi = 0;
      delay(100);

      switch (tuslar.controllerOutput) {
        case YUKARI :
          seceneklerMenu.decreaseMenuItemCounter();
          break;
        case ASAGI :
          seceneklerMenu.increaseMenuItemCounter();
          break;
        case SAG :
          switch (seceneklerMenu.menuItemCounter) {
            case 0:
              backLightMode++;
              if (backLightMode == 3)
                backLightMode = 0;
              break;
            case 1:
              if (soundOnOff == SOUND_OFF) {
                soundOnOff = SOUND_ON;
                tone(buzzerPin, 220, 100);
              }
              else
                soundOnOff = SOUND_OFF;
              break;
          }
          break;
        case SOL :
          mode = MAIN_MENU_MODE;
          break;
      }
      tuslar.tusBasildi = 0;
    }
    delay(50);
  }

  while (mode == GAME_MODE ) {
    myGLCD.setFont(TinyFont);
    tuslar.controllerOutput = SAG;
    yilan.resetSnake();
    yilan.yon = SAG;
    while (yilan.gameIsOn) {
      backLightHandler();
      yilan.display();
      if (millis() >= moveTimeHold + moveDuration) {
        //delay(50);
        yilan.move();
        yilan.checkIfGameIsOver();
        moveTimeHold = millis();
        if (!yilan.gameIsOn)
          break;
      }
    }
    while (!yilan.gameIsOn) {
      saveScore(yilan.score, scoreBoard, SCOREBOARDSIZE);
      saveScoresToEEPROM(scoreBoard, SCOREBOARDSIZE);
      tuslar.controllerOutput = BOS;
      yilan.gameOverAnimation();
      if (tuslar.controllerOutput == YUKARI) {
        yilan.gameIsOn = 1;
        break;
      }
      else if (tuslar.controllerOutput == ASAGI) {
        yilan.gameIsOn = 0;
        mode = MAIN_MENU_MODE;
        break;
      }
    }
  }
  while (mode == SCOREBOARD_MODE) {
    myGLCD.clrScr();
    readScoresFromEEPROM(scoreBoard, SCOREBOARDSIZE);
    myGLCD.setFont(SmallFont);
    myGLCD.print("HIGHSCORES", CENTER, 0);
    myGLCD.setFont(TinyFont);
    for (int i = 1; i <= SCOREBOARDSIZE; i++) {
      myGLCD.printNumI(i, LEFT, 6 * i + 4);
      myGLCD.print(":", LEFT + 5, 6 * i + 4);
      myGLCD.printNumI(scoreBoard[i - 1], LEFT + 10, 6 * i + 4);
    }
    myGLCD.update();
    if (tuslar.controllerOutput == SOL) {
      mode = MAIN_MENU_MODE;
      break;
    }
  }
}
//*********************************************************************************************************************************************************
snake::snake(byte firstDirection, byte firstSize) {
  this->gameIsOn = 1;
  this->yon = firstDirection;
  this->size = firstSize;
  for (int i = 0; i <= MAX_SNAKE_SIZE; i++) {
    this->snakeCoordinatesX[i] = 99;
    this->snakeCoordinatesY[i] = 99;
  }
  for (int i = this->size - 1 ; i >=  0 ; i--) {
    switch (this->yon) {
      case SAG:
        this->snakeCoordinatesX[i] = 24 - 2 * i;
        this->snakeCoordinatesY[i] = 12 ;
        break;
      case SOL:
        this->snakeCoordinatesX[i] = 24 + 2 * i;
        this->snakeCoordinatesY[i] = 12 ;
        break;
      case YUKARI:
        this->snakeCoordinatesX[i] = 24 ;
        this->snakeCoordinatesY[i] = 12 + 2 * i;
        break;
      case ASAGI:
        this->snakeCoordinatesX[i] = 24 ;
        this->snakeCoordinatesY[i] = 12 - 2 * i;
        break;
    }
  }
  this->spawnFood();
  this->setYasakYon();
}

void snake::setYasakYon() {
  if (this->yon == SOL)
    this->yasakYon = SAG;
  if (this->yon == YUKARI)
    this->yasakYon = ASAGI;
  if (this->yon == SAG)
    this->yasakYon = SOL;
  if (this->yon == ASAGI)
    this->yasakYon = YUKARI;
}

void snake::increaseSize() {
  this->size++;
}

void snake::move() {
  //Serial.print(this->snakeCoordinatesX[0]);
  //Serial.print("---");
  // Serial.println(this->snakeCoordinatesY[0]);


  for (int i = this->size - 1; i >= 1; i--) {
    this->snakeCoordinatesX[i] = this->snakeCoordinatesX[i - 1];
    this->snakeCoordinatesY[i] = this->snakeCoordinatesY[i - 1];
  }
  if (this->yon == SAG ) {
    if (snakeCoordinatesX[0] == 82)
      this->snakeCoordinatesX[0] = 0;
    else
      this->snakeCoordinatesX[0] += 2;

  }
  else if (this->yon == SOL ) {
    if (this->snakeCoordinatesX[0] == 0)
      this->snakeCoordinatesX[0] = 82;
    else
      this->snakeCoordinatesX[0] -= 2;
  }
  else if (this->yon == YUKARI  ) {
    if (this->snakeCoordinatesY[0] == 0)
      this->snakeCoordinatesY[0] = SNAKE_Y_BOUNDARY;
    else
      this->snakeCoordinatesY[0] -= 2;
  }
  else if (this->yon == ASAGI ) {
    if (this->snakeCoordinatesY[0] == SNAKE_Y_BOUNDARY)
      this->snakeCoordinatesY[0] = 0;
    else
      this->snakeCoordinatesY[0] += 2;
  }

  if (this->snakeCoordinatesX[0] == this->foodX && this->snakeCoordinatesY[0] == this->foodY) {
    this->increaseSize();
    this->spawnFood();
    if (soundOnOff == SOUND_ON)
      tone(buzzerPin, 220, 100);
  }
  this->setYasakYon();
}

void snake::display() {
  myGLCD.clrScr();
  this->displayScoreHUD();
  for (int i = 0; i <= this->size - 1; i++) {
    if (this->snakeCoordinatesX[i] != 99 && snakeCoordinatesY[i] != 99)
      myGLCD.setPixel(this->snakeCoordinatesX[i], this->snakeCoordinatesY[i]);
    myGLCD.setPixel(this->snakeCoordinatesX[i] + 1, this->snakeCoordinatesY[i]);
    myGLCD.setPixel(this->snakeCoordinatesX[i], this->snakeCoordinatesY[i] + 1);
    myGLCD.setPixel(this->snakeCoordinatesX[i] + 1 , this->snakeCoordinatesY[i] + 1);
  }
  myGLCD.setPixel(this->foodX, this->foodY);
  myGLCD.setPixel(this->foodX + 1, this->foodY);
  myGLCD.setPixel(this->foodX, this->foodY + 1);
  myGLCD.setPixel(this->foodX + 1 , this->foodY + 1);
  myGLCD.update();
}

void snake::calculateScore() {
  this->score = (size - 3) * 25 ;
}

void snake::displayScoreHUD() {
  int score = this->score;
  this->calculateScore();
  myGLCD.drawLine(0, SNAKE_Y_BOUNDARY  + 2, 84, SNAKE_Y_BOUNDARY  + 2);
  myGLCD.printNumI(score, 0, SNAKE_Y_BOUNDARY  + 3);
}

void snake::changeDirection(byte newDirection) {
  if (newDirection != this->yasakYon) {
    this->yon = newDirection;
    this->setYasakYon();
  }
}

void snake::spawnFood() {
  this->foodX = 2 * random(0 , 41) ;
  this->foodY = 2 * random(0 , SNAKE_Y_BOUNDARY / 2) ;
  //Serial.print(this->foodX);
  //Serial.print("---");
  // Serial.println(this->foodY);
}

void snake::checkIfGameIsOver() {
  for (int i = 3; i <= this->size - 1; i++) {
    if (this->snakeCoordinatesX[0] ==  this->snakeCoordinatesX[i] && this->snakeCoordinatesY[0] ==  this->snakeCoordinatesY[i]) {
      this->gameIsOn = 0;
      //Serial.print(i);
      //  Serial.print(" BAS X :");
      //   Serial.print(snakeCoordinatesX[0]);
      //  Serial.print(" BAS Y :");
      /// Serial.print(snakeCoordinatesX[0]);
      //  Serial.print(" X :");
      //  Serial.print(snakeCoordinatesX[i]);
      //  Serial.print(" Y :");
      //  Serial.print(snakeCoordinatesX[i]);

      return;
    }
    this->gameIsOn = 1;
  }

}

void snake::resetSnake() {
  this->gameIsOn = 1;
  this->yon = SAG;
  this->size = 6;
  for (int i = 0; i <= MAX_SNAKE_SIZE; i++) {
    this->snakeCoordinatesX[i] = 99;
    this->snakeCoordinatesY[i] = 99;
  }
  for (int i = this->size - 1 ; i >=  0 ; i--) {
    switch (this->yon) {
      case SAG:
        this->snakeCoordinatesX[i] = 24 - 2 * i;
        this->snakeCoordinatesY[i] = 12 ;
        break;
      case SOL:
        this->snakeCoordinatesX[i] = 24 + 2 * i;
        this->snakeCoordinatesY[i] = 12 ;
        break;
      case YUKARI:
        this->snakeCoordinatesX[i] = 24 ;
        this->snakeCoordinatesY[i] = 12 + 2 * i;
        break;
      case ASAGI:
        this->snakeCoordinatesX[i] = 24 ;
        this->snakeCoordinatesY[i] = 12 - 2 * i;
        break;
    }
  }
  this->spawnFood();
  this->setYasakYon();
}

controller::controller(byte pin) {
  this->analogInputPin = pin;
}

void controller::getAnalogInput() {
  rawAnalogInput = analogRead(this->analogInputPin);
  Serial.println(rawAnalogInput);
}

void controller::decodeAnalogInput() {
  if (this->rawAnalogInput >= 850)
    this->controllerOutput = YUKARI;
  else if (this->rawAnalogInput < 850 && this->rawAnalogInput >= 650 )
    this->controllerOutput = SOL;
  if (this->rawAnalogInput < 650 && this->rawAnalogInput >= 450)
    this->controllerOutput = ASAGI;
  else if (this->rawAnalogInput < 450 && this->rawAnalogInput >= 200 )
    this->controllerOutput = SAG;
}

void ISR_Handle() {
  tuslar.getAnalogInput();
  tuslar.decodeAnalogInput();
  tuslar.tusBasildi = 1;
  yilan.changeDirection(tuslar.controllerOutput);
}

void snake::gameOverAnimation() {
  if (soundOnOff == SOUND_ON)
    uhoh();
  myGLCD.clrScr();
  myGLCD.update();
  delay(500);
  myGLCD.drawLine(0, 0, 83, 47);
  myGLCD.update();
  delay(400);
  myGLCD.drawLine(0, 47, 83, 0);
  myGLCD.update();
  delay(500);
  myGLCD.invert(true);
  myGLCD.update();
  delay(500);
  myGLCD.invert(false);
  myGLCD.update();
  delay(500);
  myGLCD.setFont(SmallFont);
  myGLCD.print("OYUN BITTI", CENTER, 0);
  myGLCD.print("PUANIN:", CENTER, 8);
  myGLCD.printNumI( this->score, CENTER, 16);
  myGLCD.print("(YENIDEN OYNA-", CENTER, 24);
  myGLCD.print("MAK ICIN ", CENTER, 32);
  myGLCD.print("KIRMIZIYA BAS)", CENTER, 40);
  myGLCD.update();
  delay(5000);
}

menu::menu(byte rows, char row1string[], char row2string[], char row3string[], char row4string[], char row5string[], char row6string[]) {
  this->totalRows = rows;
  strcpy(this->menuStrings [0], row1string);
  strcpy(this->menuStrings [1], row2string);
  strcpy(this->menuStrings [2], row3string);
  strcpy(this->menuStrings [3], row4string);
  strcpy(this->menuStrings [4], row5string);
  strcpy(this->menuStrings [5], row6string);
}
menu::menu(byte rows, char row1string[], char row2string[], char row3string[], char row4string[], char row5string[]) {
  this->totalRows = rows;
  strcpy(this->menuStrings [0], row1string);
  strcpy(this->menuStrings [1], row2string);
  strcpy(this->menuStrings [2], row3string);
  strcpy(this->menuStrings [3], row4string);
  strcpy(this->menuStrings [4], row5string);
}
menu::menu(byte rows, char row1string[], char row2string[], char row3string[], char row4string[]) {
  this->totalRows = rows;
  strcpy(this->menuStrings [0], row1string);
  strcpy(this->menuStrings [1], row2string);
  strcpy(this->menuStrings [2], row3string);
  strcpy(this->menuStrings [3], row4string);
}
menu::menu(byte rows, char row1string[], char row2string[], char row3string[]) {
  this->totalRows = rows;
  strcpy(this->menuStrings [0], row1string);
  strcpy(this->menuStrings [1], row2string);
  strcpy(this->menuStrings [2], row3string);
}
menu::menu(byte rows, char row1string[], char row2string[]) {
  this->totalRows = rows;
  strcpy(this->menuStrings [0], row1string);
  strcpy(this->menuStrings [1], row2string);;
}

void menu::display() {
  myGLCD.clrScr();
  myGLCD.setFont(SmallFont);
  for (int i = 0 ; i <= this->totalRows ; i++) {
    myGLCD.print(this->menuStrings[i], 0, i * 8);
  }
  myGLCD.print("*", 78, this->menuItemCounter * 8);
  myGLCD.update();
}

void menu::increaseMenuItemCounter() {
  if (this->menuItemCounter < this->totalRows - 1 )
    this->menuItemCounter++;
}

void menu::decreaseMenuItemCounter() {
  if (this->menuItemCounter != 0)
    this->menuItemCounter--;
}


void uhoh() {
  for (int i = 220; i < 550; i = i * 1.01) {
    tone(buzzerPin, i, 100);
  }
  delay(200);
  for (int i = 220; i < 550; i = i * 1.01) {
    tone(buzzerPin, i, 100);
  }
  delay(200);
  for (int i = 220; i < 550; i = i * 1.01) {
    tone(buzzerPin, i, 100);
  }
  delay(200);
  for (int i = 550; i > 220; i = i * .999) {
    tone(buzzerPin, i, 100);
  }
}

void backLightLdrHandler(void) {
  if (analogRead(ldrPin) >= 500)
    digitalWrite(backLightPin , LOW);
  else
    digitalWrite(backLightPin , HIGH);
}

void backLightHandler(void) {
  if (backLightMode == BACKLIGHT_ON)
    digitalWrite(backLightPin, LOW);
  else if (backLightMode == BACKLIGHT_OFF)
    digitalWrite(backLightPin, HIGH);
  else
    backLightLdrHandler();
}

void readAllEEPROMData() {
  for (int i = 0; i < EEPROM.length(); i++) {
    Serial.print(i);
    Serial.print(" : ");
    Serial.println(EEPROM.read(i));
  }
}

void saveScore(int score, int* scoreBoardArray, int scoreBoardSize) {
  int i = 0;
  int temp = 0;
  for (i = 0; i < scoreBoardSize; i++) {
    if (score == scoreBoardArray[i])
      break;
    if (score > scoreBoardArray[i]) {
      temp = scoreBoardArray[i];
      scoreBoardArray[i] = score;
      saveScore(temp, scoreBoardArray, scoreBoardSize);
      break;
    }
  }
}

void readScoresFromEEPROM(int* scoreBoardArray, int scoreBoardSize) {
  int i;
  for (i = 0; i < scoreBoardSize; i++) {
    scoreBoardArray[i] = EEPROM.read(i * 2) * 100 + EEPROM.read(i * 2 + 1);
  }
}

void saveScoresToEEPROM(int* scoreBoardArray, int scoreBoardSize) {
  int i;
  for (i = 0; i < scoreBoardSize; i++) {
    EEPROM.update(i * 2 , scoreBoardArray[i] / 100) ;
    EEPROM.update(i * 2 + 1 , scoreBoardArray[i] % 100) ;
  }
}
