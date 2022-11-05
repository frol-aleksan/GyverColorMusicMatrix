#define SOUND_PIN    A0     // пин звука
#define STRIP_PIN     2     // пин ленты
#define BUTTON_PIN    3     // пин кнопки
#define SHOW_MAX      1     // 1/0 - показывать/не показывать плавающие точки максимума
#define COLOR_MULT  -15     // шаг изменения цветовой палитры столбика
#define COLOR_STEP    3     // шаг движения палитры столбика (по времени)
#define COLON_SIZE   16     // высота матрицы
#define COLON_AMOUNT 16     // ширина матрицы
#define BRIGHT      128     // яркость ленты до 255
#define LEDS_AM     COLON_SIZE * COLON_AMOUNT  //вычисляемое количество светодиодов в матрице
#define P_SPEED       2     // скорость движения
#define START_HUE     0     // цвет огня (0.. 255). 0 красный, 150 синий, 200 розовый
#define COLOR_DEBTH   3

#include <FastLED.h>
#include <microLED.h>
#include "VolAnalyzer.h"

microLED < COLON_SIZE * COLON_AMOUNT, STRIP_PIN, -1, LED_WS2812, ORDER_GRB, CLI_AVER > strip;
VolAnalyzer sound(SOUND_PIN);

byte volume[COLON_AMOUNT];
int maxs[COLON_AMOUNT];
byte colorCount = 0;
int16_t noise;
byte curColor = 0; 
uint8_t counter = 1;
int currentValue, prevValue;
uint8_t COLORSTEP = 3;

void setup() {
  Serial.begin(9600);
  strip.setBrightness(BRIGHT);     // яркость ленты
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  currentValue = digitalRead(BUTTON_PIN);
  if (currentValue != prevValue) {
    // Что-то изменилось, здесь возможна зона неопределенности
    delay(1000);  // Делаем задержку, именно поэтому эффекты переключаются с секундным запозданием, зато плата не виснет
    // А вот теперь спокойно считываем значение, считая, что нестабильность исчезла
    currentValue = digitalRead(BUTTON_PIN);
    counter++;
    if (counter == 4)
         COLORSTEP = random(120,250); // шаг цвета, пусть генерируется произвольно
    if (counter > 4)
       counter = 1;
  }
  prevValue = currentValue;
  switch (counter) {
     case 1: spectrum();       break;
     case 2: analizerNoise();  break;
     case 3: waveForm();       break;
     case 4: runningLigts();   break;
  }
}

void spectrum()
{
  sound.setVolMax(100);
  sound.setVolK(30);
  if (sound.tick()) {   // если анализ звука завершён (~10мс)
    strip.clear();      // чистим ленту
    for (int i = 0; i < COLON_AMOUNT / 2; i++) {
      // домножаем шум на громкость
      // также я домножил на 2, чтобы шум был более амплитудным
      int val = inoise8(noise - i * 100) * 2 * sound.getVol() / 100;
      // ограничиваем и масштабируем до половины высоты столбика
      val = constrain(val, 0, 255);
      val = map(val, 0, 255, 0, COLON_SIZE);
      volume[i] = val;
    }
    noise += 90;    // двигаем шум (скорость бокового движения картинки)
    colorCount += COLOR_STEP;   // двигаем цвет
    // двигаем точки максимумов
    for (int i = 0; i < COLON_AMOUNT; i++) {
      if (maxs[i] < volume[i] * 10) maxs[i] = (volume[i] - 1) * 10;
      else maxs[i] -= 2;
      if (maxs[i] < 0) maxs[i] = 0;
    }
    // выводим
    for (int col = 0; col < COLON_AMOUNT; col++) {      // по столбикам
      for (int i = 0; i < volume[col]; i++) {           // для текущей громкости
        mData color = mWheel8(colorCount + i * COLOR_MULT);
        strip.leds[COLON_SIZE*2 * col + COLON_SIZE + i] = color;      // вверх от центра
        strip.leds[COLON_SIZE*2 * col + COLON_SIZE - 1 - i] = color;  // вниз от центра
      }
      // отображаем точки максимумов, если включены и больше 0
      if (SHOW_MAX && maxs[col] > 0) {
        strip.leds[COLON_SIZE*2 * col + COLON_SIZE + maxs[col] / 10] = mGreen;      // вверх от центра
        strip.leds[COLON_SIZE*2 * col + COLON_SIZE - 1 - maxs[col] / 10] = mGreen;  // вниз от центра
      }
    }
    strip.show();
  }
}

void analizerNoise() {
  sound.setVolMax(100);
  sound.setVolK(30);
  if (sound.tick()) {   // если анализ звука завершён (~10мс)
    strip.clear();      // чистим ленту
    for (int i = 0; i < COLON_AMOUNT; i++) {
      // домножаем шум на громкость
      // также я домножил на 2, чтобы шум был более амплитудным
      int val = inoise8(noise - i * 100) * 2 * sound.getVol() / 100; 
      // ограничиваем и масштабируем до половины высоты столбика
      val = constrain(val, 0, 255);
      val = map(val, 0, 255, 0, COLON_SIZE / 2);
      volume[i] = val;
    }
    noise += 90;    // двигаем шум (скорость бокового движения картинки)
    colorCount += COLOR_STEP;   // двигаем цвет
    // двигаем точки максимумов
    for (int i = 0; i < COLON_AMOUNT; i++) {
      if (maxs[i] < volume[i] * 10) maxs[i] = (volume[i] - 1) * 10;
      else maxs[i] -= 2;
      if (maxs[i] < 0) maxs[i] = 0;
    } 
    // выводим
    for (int col = 0; col < COLON_AMOUNT; col++) {      // по столбикам
      for (int i = 0; i < volume[col]; i++) {           // для текущей громкости
        mData color = mWheel8(colorCount + i * COLOR_MULT);
        strip.leds[COLON_SIZE * col + COLON_SIZE / 2 + i] = color;      // вверх от центра
        strip.leds[COLON_SIZE * col + COLON_SIZE / 2 - 1 - i] = color;  // вниз от центра
      }
      // отображаем точки максимумов, если включены и больше 0
      if (SHOW_MAX && maxs[col] > 0) {
        strip.leds[COLON_SIZE * col + COLON_SIZE / 2 + maxs[col] / 10] = mGreen;      // вверх от центра
        strip.leds[COLON_SIZE * col + COLON_SIZE / 2 - 1 - maxs[col] / 10] = mGreen;  // вниз от центра
      }
    }
    strip.show();
  }
}

void waveForm() {
  sound.setVolMax((COLON_SIZE / 2));
  if (sound.tick()) {   // если анализ звука завершён (~10мс)
    strip.clear();  // чистим ленту
    // перематываем массив громкости вправо
    for (int i = COLON_AMOUNT - 1; i > 0; i--) volume[i] = volume[i - 1];
    if (sound.getVol() > (COLON_SIZE / 2)) //при клипинге плата виснет, но почему-то только при включении эффекта. Если запустить эффект в тишине, то далее работает нормально.
         volume[0] = (COLON_SIZE / 2);     //Проблему зависания платы решил так: если звук слишком громкий, делаем принудительную компрессию по амплитуде.
    else                                   //Правда, при таком подходе остаются незадействованы верхний и нижний ряды, ну да ладно. Зато эффект работает без зависаний.
         volume[0] = sound.getVol();       //Если же уровень сигнала находится в допустимых рамках, пишем новое значение громкости на освободившееся место.
    colorCount += COLOR_STEP;   // двигаем цвет
    for (int col = 0; col < COLON_AMOUNT; col++) {      // по столбикам
      for (int i = 0; i < volume[col]; i++) {           // для текущей громкости
        mData color = mWheel8(colorCount + i * COLOR_MULT);             // цвет со смещением по радуге
        strip.leds[COLON_SIZE * col + COLON_SIZE / 2 + i] = color;      // вверх от центра
        strip.leds[COLON_SIZE * col + COLON_SIZE / 2 - 1 - i] = color;  // вниз от центра
      }
    }
    strip.show();
  }
}

void runningLigts() {
  sound.setVolK(15);        // снизим фильтрацию громкости (макс. 31)
  sound.setVolMax(255);     // выход громкости 0-255
  sound.setPulseMax(200);   // сигнал пульса
  sound.setPulseMin(150);   // перезагрузка пульса
  if (sound.tick()) {   // если анализ звука завершён (~10мс)
    // перематываем массив светодиодов на P_SPEED вправо
    for (int k = 0; k < P_SPEED; k++) {
      for (int i = LEDS_AM - 1; i > 0; i--) {
        strip.leds[i] = strip.leds[i - 1];
      }
    }
    // резкий звук - меняем цвет
    if (sound.pulse()) curColor += COLORSTEP;
    // берём текущий цвет с яркостью по громкости (0-255)
    mData color = mWheel8(curColor, sound.getVol());
    // красим P_SPEED первых светодиодов
    for (int i = 0; i < P_SPEED; i++) strip.set(i, color);
    strip.show();
  }
}
