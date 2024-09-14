#include <HardwareSerial.h>
#include "LoRaMESH.h"

// Configuração da USART do ESP32 com o LoRa Master o qual está conectado
#define RX_Pin GPIO_NUM_16
#define TX_Pin GPIO_NUM_17
#define BAUDRATE 9600
#define SER_BUF_SIZE 1024

// Módulo LoRa
uint8_t ID = 0; // MASTER
#define PASSWORD 123
HardwareSerial Serial_ESP_LORA(2);
#define DATA_REQUEST_CMD 0x01 // Comando de solicitação de dados
#define RESPONSE_CMD 0x02 // Comando de resposta