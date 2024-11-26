#include <Arduino.h>
#include <ESP32Servo.h> 
#include <WiFi.h>
#include <TwoWayESP.h>
#include "esp_system.h"

// Настройки
#define BUFFER_SIZE 1000 // Размер буфера

// Серво-моторы
Servo RightFing;
Servo RightElb;
Servo RightBic;
Servo RightShoul;

// Пины серв
int rightFingers = 22; // кисть
int rightElbow = 25;   // локоть
int rightBiceps = 33;  // бицепс
int rightShoulder = 32; // плечо  

// Адрес для ESP-NOW
uint8_t broadcastAddress[] = {0xCC, 0xDB, 0xA7, 0x2F, 0x36, 0xE0};

// Структура данных
typedef struct ServoAngles {
    uint16_t id;               // Идентификатор
    uint16_t Serv1;            // Кисть
    uint16_t Serv2;            // Локоть
    uint16_t Serv3;            // Бицепс
    uint16_t Serv4;            // Плечо
    uint16_t buttRightHand;    // Правая клешня
    uint16_t Butt1;            // Запись
    uint16_t Butt2;            // Повтор
    uint16_t Butt3;            // Трансляция
} ServoAngles;

// Переменные
ServoAngles recv, buffer[BUFFER_SIZE];
int currentFing = 90, currentElb = 90, currentBic = 90, currentShoul = 90;

// Для работы с буфером
int bufferIndex = 0;
bool recording = false;
bool playing = false;

// Плавное движение
int smoothMove(int current, int target, int step) {
    if (abs(target - current) <= step) return target;
    return current + (target > current ? step : -step);
}

// Настройка
void setup() {
    Serial.begin(9600);
    Serial.println("rukainput");

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    RightFing.setPeriodHertz(50);
    RightElb.setPeriodHertz(50);
    RightBic.setPeriodHertz(50);
    RightShoul.setPeriodHertz(50);

    RightFing.attach(rightFingers, 500, 2400);
    RightElb.attach(rightElbow, 500, 2400);
    RightBic.attach(rightBiceps, 500, 2500);
    RightShoul.attach(rightShoulder, 500, 2400);

    // Установить начальные углы
    RightFing.write(currentFing);
    RightElb.write(currentElb);
    RightBic.write(currentBic);
    RightShoul.write(currentShoul);

    TwoWayESP::Begin(broadcastAddress);
}

// Основной цикл
void loop() {
    static unsigned long lastUpdate = 0;

    // Проверка наличия данных
    if (TwoWayESP::Available()) {
        TwoWayESP::GetBytes(&recv, sizeof(ServoAngles));

        // Ограничения на углы
        recv.Serv1 = constrain(recv.Serv1, 3, 120);
        recv.Serv2 = constrain(recv.Serv2, 3, 120);
        recv.Serv3 = constrain(recv.Serv3, 3, 120);
        recv.Serv4 = constrain(recv.Serv4, 3, 120);

        // Проверка кнопок
        if (recv.Butt1 == 1) { // Запуск/остановка записи
            recording = !recording;
            playing = false; // Остановить воспроизведение
            Serial.println("Запись началась");
        }
        if (recv.Butt2 == 1) { // Запуск/остановка воспроизведения
            playing = !playing;
            recording = false; // Остановить запись
            Serial.println("Воспроизведение началось");
        }
    }

    // Обновление каждые 30 мс
    if (millis() - lastUpdate >= 30) {
        lastUpdate = millis();

        if (recording) {
            // Сохранение данных в буфер
            buffer[bufferIndex] = recv;
            bufferIndex = (bufferIndex + 1) % BUFFER_SIZE; // Цикличный буфер
        } else if (playing) {
            // Воспроизведение из буфера
            recv = buffer[bufferIndex];
            bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
        }

        // Плавное движение
        currentFing = smoothMove(currentFing, recv.Serv1, 2);
        currentElb = smoothMove(currentElb, recv.Serv2, 2);
        currentBic = smoothMove(currentBic, recv.Serv3, 2);
        currentShoul = smoothMove(currentShoul, recv.Serv4, 2);

        RightFing.write(currentFing);
        RightElb.write(currentElb);
        RightBic.write(currentBic);
        RightShoul.write(currentShoul);

        // Вывод на последовательный монитор
        Serial.print("Fing: "); Serial.print(currentFing);
        Serial.print(" Elb: "); Serial.print(currentElb);
        Serial.print(" Bic: "); Serial.print(currentBic);
        Serial.print(" Shoul: "); Serial.println(currentShoul);
    }
}
