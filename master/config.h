#include <HardwareSerial.h>
#include "LoRaMESH.h"

#define USE_LORA_PINS_ON_SENSORS  // Comment out this line to connect the sensor pins directly to the ESP32

// Configuração da USART do ESP32 com o LoRa Master o qual está conectado
#define RX_Pin GPIO_NUM_16
#define TX_Pin GPIO_NUM_17
#define BAUDRATE 9600
#define SER_BUF_SIZE 1024

// Módulo LoRa
uint8_t ID = 0; // MASTER
#define PASSWORD 123
HardwareSerial Serial_ESP_LORA(2);

// Sensor Ultrassonico HC-SR04
#ifdef USE_LORA_PINS_ON_SENSORS
#define PIN_TRIGGER LoRa_GPIO0 
#define PIN_ECHO LoRa_GPIO1
#else
#define PIN_TRIGGER GPIO_NUM_12 
#define PIN_ECHO GPIO_NUM_14
#endif

// Sensor de Chuva YL-83
#ifdef USE_LORA_PINS_ON_SENSORS
#define PIN_RAIN_ANALOG LoRa_GPIO5 // // analogico
#define PIN_RAIN_DIG LoRa_GPIO4
#else
#define PIN_RAIN_ANALOG GPIO_NUM_35
#define PIN_RAIN_DIG GPIO_NUM_32
#endif

// Sensor de Umidade e Temperatura DHT22
#define DHTTYPE DHT22
#ifdef USE_LORA_PINS_ON_SENSORS
#define PIN_DHT LoRa_GPIO6 // analogico  
#else
#define PIN_DHT GPIO_NUM_33  
#endif
