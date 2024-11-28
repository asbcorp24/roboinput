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
Servo RightHandClaw; // Добавляем серво для клешни

// Пины серв
int rightFingers = 22; // кисть
int rightElbow = 25;   // локоть
int rightBiceps = 33;  // бицепс
int rightShoulder = 32; // плечо  
int rightHandClawPin = 26; // пин для клешни  

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
int currentFing = 0, currentElb = 3, currentBic = 90, currentShoul = 90;
int currentClaw = 0; // Начальный угол для клешни

// Для работы с буфером
int bufferIndex = 0;
int recordingIndex = 0;   // Счетчик текущей записи
unsigned long lastRecordingTime = 0; // Время последнего шага записи
bool recording = false;
bool playing = false;
bool armEnabled = false; // Переменная для отслеживания состояния кнопки Butt3

// Плавное движение
/*int smoothMove(int current, int target, int step) {
    if (abs(target - current) <= step) return target;
    return current + (target > current ? step : -step);
}*/
int smoothMove(int current, int target, int step) {
    int difference = abs(target - current);
    int dynamicStep = max(1, step * difference / 20); // Шаг зависит от разницы
    if (difference <= dynamicStep) return target;    // Если разница меньше шага, сразу устанавливаем цель
    return current + (target > current ? dynamicStep : -dynamicStep);
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
    RightHandClaw.setPeriodHertz(50); // Для клешни

    RightFing.attach(rightFingers, 500, 2400);
    RightElb.attach(rightElbow, 500, 2400);
    RightBic.attach(rightBiceps, 500, 2500);
    RightShoul.attach(rightShoulder, 500, 2400);
    RightHandClaw.attach(rightHandClawPin, 500, 2400); // Привязываем клешню к пину

    // Установить начальные углы
    RightFing.write(currentFing);
    RightElb.write(currentElb);
    RightBic.write(currentBic);
    RightShoul.write(currentShoul);
    RightHandClaw.write(currentClaw); // Установим начальный угол клешни

    TwoWayESP::Begin(broadcastAddress);
    
}

// Основной цикл
void loop() {
    static unsigned long lastUpdate = 0;

    // Проверка наличия данных
    if (TwoWayESP::Available()) {
        TwoWayESP::GetBytes(&recv, sizeof(ServoAngles));
  
        // Ограничения на углы
        recv.Serv1 = constrain(recv.Serv1, 3, 90);
        recv.Serv2 = constrain(recv.Serv2, 3, 90);
        recv.Serv3 = constrain(recv.Serv3, 20, 150);
        recv.Serv4 = constrain(recv.Serv4, 3, 90);
       
        recv.Serv1 = 90 - recv.Serv1;
        recv.Serv4 = 90 - recv.Serv4;
        recv.Serv3 = 150 - recv.Serv3;

 

        // Проверка кнопок
        if ((recv.Butt1 == 1)&&(recording != true)) { // Запуск/остановка записи
            recording = true;
             recordingIndex = 0; // Сброс счетчика записи
             bufferIndex=0;
              lastRecordingTime = millis(); // Сохранить время начала записи
                              playing = false; // Остановить воспроизведение при записи
                           
                Serial.println("Запись началась");
            }
     
               if ((recv.Butt1 == 0)&&(recording == true)) { // Запуск/остановка записи
             
              recording = false; // Останавливаем запись
    recordingIndex = bufferIndex; // Фиксируем длину записи
    Serial.println("Запись завершена.");                         
                Serial.println("Запись закончилась");
            }
      
 
        if ((recv.Butt2 == 1)&& (playing !=true)) { // Запуск/остановка воспроизведения
        
            playing = true;

            if (playing) {
                recording = false; // Остановить запись при воспроизведении
                Serial.println("Воспроизведение началось");
            }
        }
        if ((recv.Butt2 == 0)&& (playing ==true)) { // Запуск/остановка воспроизведения
        bufferIndex=0;
            playing = !true;
            
        }

        if (recv.Butt3 == 1) { // Переключение состояния "руки"
            armEnabled = true;
            if (armEnabled) {
                Serial.println("Рука включена");
            } else {
                Serial.println("Рука отключена");
            }
        } else {
             armEnabled = !true;
 if (armEnabled) {
                Serial.println("Рука включена");
            } else {
          //      Serial.println("Рука отключена");
            }

        }

        // Управление клешней
        if (recv.buttRightHand == 1) {
            currentClaw = 60; // Поворачиваем на 30 градусов, если кнопка нажата
        } else {
            currentClaw = 30; // Поворачиваем на 90 градусов, если кнопка не нажата
        }
    }

    // Обновление каждые 30 мс
    if (millis() - lastUpdate >= 20) {
        lastUpdate = millis();

        if (recording) {
            // Сохранение данных в буфер
            buffer[bufferIndex] = recv;
            bufferIndex = (bufferIndex + 1) % BUFFER_SIZE; // Цикличный буфер
            recordingIndex = (recordingIndex + 1) % BUFFER_SIZE; // Обновление счетчика записи
            lastRecordingTime = millis(); // Обновить время последнего шага записи
        } else if (playing) {
            if (recordingIndex > 0) {
        recv = buffer[bufferIndex]; // Читаем текущий кадр из буфера
        bufferIndex++; // Переходим к следующему кадру

        // Цикличность: если достигли конца записи, начинаем с начала
        if (bufferIndex >= recordingIndex) {
            bufferIndex = 0; // Возвращаемся к началу записи
            Serial.println("Цикл воспроизведения завершен, начинаем сначала.");
        }
    } else {
        // Если буфер пуст, останавливаем воспроизведение
        playing = false;
        Serial.println("Буфер пуст, воспроизведение остановлено.");
    }
        }

        if (armEnabled) {
            // Плавное движение
            currentFing = smoothMove(currentFing, recv.Serv1, 5);
            currentElb = smoothMove(currentElb, recv.Serv2, 5);
            currentBic = smoothMove(currentBic, recv.Serv3, 5);
            currentShoul = smoothMove(currentShoul, recv.Serv4, 5);

            RightFing.write(currentFing);
            RightElb.write(currentElb);
            RightBic.write(currentBic);
            RightShoul.write(currentShoul);
            RightHandClaw.write(currentClaw); // Управление клешней
        }

        // Вывод на последовательный монитор
  
   Serial.print(" "); Serial.print(bufferIndex);
   Serial.print(":"); Serial.print(recordingIndex);
        Serial.print("-"); Serial.print(currentFing);
        Serial.print(" "); Serial.print(currentElb);
        Serial.print(" "); Serial.print(currentBic);
        Serial.print(" "); Serial.print(currentShoul);
        Serial.print(" "); Serial.println(currentClaw); // Выводим угол клешни
    }
}
