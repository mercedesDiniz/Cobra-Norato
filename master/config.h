#include "WString.h"
#ifndef _CONFIG_H
#define _CONFIG_H
#include <HardwareSerial.h>

// Configuração da USART do ESP32 com o LoRa Master o qual está conectado
#define RX_Pin GPIO_NUM_16
#define TX_Pin GPIO_NUM_17
#define BAUDRATE 9600
#define SER_BUF_SIZE 1024

// Módulo LoRa
const uint8_t ID = 0; // MASTER
#define PASSWORD 123
HardwareSerial Serial_ESP_LORA(2);
#define DATA_REQUEST_CMD 0x03 // Comando de solicitação de dados
#define RESPONSE_CMD 0x02 // Comando de resposta

// Configurações da rede Wi-Fi
#define WIFI_SSID "Nome_da_Rede"
#define WIFI_PASS "Senha"

// Configurações do MQTT
const char* mqtt_server = "192.168.0.102";  // IP do host Docker na rede local
const int mqtt_port = 1883;  // Porta padrão MQTT
const char* mqtt_topic = "pot";
const char* mqtt_client_id = "com.frank123.esp32";

#define SEPARATOR ";"

// Estrutura de dados do sensor
struct SensorData{
  float humidity;
  float temperature;
  uint8_t rain_dig;
  int rain_analog;
  float distance;
  String message;
};

// Finite States Machine
enum State {
  SAMPLE,               
  CONNECT,              
  TRANSMIT,             
  REST,                
};

// Delays (300000ms = 5min | 120000ms = 2min | 60000ms = 1min | 30000 ms = 30s)
#define TIME_BETWEEN_SAMPLING_WINDOWS     30000 
#define WAITING_TIME_FOR_NEW_DATA_FROM_SENSOR_NODE      120000

#endif