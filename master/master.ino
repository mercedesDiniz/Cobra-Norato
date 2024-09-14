//********************************************
// COBRA NONATO (Módulo master - Nó 0)
//********************************************

#define TAG "MASTER"

#include <Ultrasonic.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "config.h"

//********************************************
// GLOBAL VARIABLES AND FUNCTION PROTOTYPES
//********************************************

bool config_lora();

// Módulo LoRa
LoRaMESH gLora(&Serial_ESP_LORA);

// Sensor Ultrassonico HC-SR04
float distance;

// Sensor de Umidade e Temperatura DHT22
float temperature, humidity; 

// Sensor de Chuva YL-83
int rain_analog;
uint8_t rain_dig;

//************************
// MAIN CODE
//************************
void setup() {
  // Monitor Serial
  Serial.begin(115200);
  delay(2000);

  // Comunicação da USART do ESP32 com o LoRa o qual está conectado
  Serial_ESP_LORA.setRxBufferSize(SER_BUF_SIZE);
  Serial_ESP_LORA.begin(BAUDRATE, SERIAL_8N1, RX_Pin, TX_Pin);
  gLora.begin(true);

  // Configurando o módulo LoRa conectado ao ESP para ser o MASTER
  bool lora_setup_completed = false;
  while (!lora_setup_completed) {
    lora_setup_completed = config_lora();
  }


}

void loop() {
  uint16_t slaveId = 1;  // Nó sensor 01
  uint8_t payload_tmp[1];

  // Solicita dados dos sensores ao nó slave
  ESP_LOGI(TAG, "Solicitando dados ...");
  for(uint8_t i=0; i<5; i++){
    gLora.PrepareFrameCommand(slaveId, DATA_REQUEST_CMD, payload_tmp, 1);
    gLora.SendPacket();
  }

  // Aguardar a resposta do slave
  uint16_t id;
  uint8_t command;
  uint8_t payload[10];
  uint8_t payloadSize;

  if (gLora.ReceivePacketCommand(&id, &command, payload, &payloadSize, 3000)) {
        if (command == RESPONSE_CMD && payloadSize == 8) { // Verifica se é uma resposta válida e o tamanho certo
            ESP_LOGI(TAG, "done.");
            // Processar os dados recebidos
            float humidity = ((payload[0] << 8) | payload[1]) / 100.0; // Humidade com duas casas decimais
            float temperature = ((payload[2] << 8) | payload[3]) / 100.0; // Temperatura com duas casas decimais
            uint8_t rain_dig = payload[4];  // Chuva digital (1 ou 0)
            int rain_analog = payload[5];  // Valor analógico do sensor de chuva
            float distance = ((payload[6] << 8) | payload[7]) / 100.0; // Distância em cm com duas casas decimais

            // Exibir os dados
            Serial.print("Umidade: ");
            Serial.print(humidity);
            Serial.print(" %, Temperatura: ");
            Serial.print(temperature);
            Serial.print(" ºC, Chuva (digital): ");
            Serial.print(rain_dig ? "Sim" : "Não");
            Serial.print(", Chuva (analógico): ");
            Serial.print(rain_analog);
            Serial.print(", Distância: ");
            Serial.print(distance);
            Serial.println(" cm");
        }
  }

  delay(10000);  // Repete a cada 10 segundos
}

//*****************************
// AUXILIARY FUNCTIONS
//*****************************
bool config_lora(){
  if(gLora.localId != ID)
  {
    if(!gLora.setnetworkId(ID)){
      ESP_LOGE(TAG, "Erro ao definir o novo ID");
      return false;
    }
    ESP_LOGD(TAG, "ID configurado com sucesso!");

    if(!gLora.config_bps(BW500, SF_LoRa_7, CR4_5)){
      ESP_LOGE(TAG, "Erro ao configurar bps");
      return false;
    }
    ESP_LOGD(TAG, "Parametros LoRa configurados com sucesso!");
    
    if(!gLora.config_class(LoRa_CLASS_C, LoRa_WINDOW_15s)){
      ESP_LOGE(TAG, "Erro ao configurar a classe");
      return false;
    }
    ESP_LOGD(TAG, "Modo de operacao configurado com sucesso!");

    if(!gLora.setpassword(PASSWORD)){
      ESP_LOGE(TAG, "Erro ao gravar a senha ou a senha gravada não condiz com a senha definida");
      return false;
    }
    ESP_LOGD(TAG, "Senha configurada com sucesso!");
  }

  ESP_LOGI(TAG, "LocalID: %s", String(gLora.localId));
  ESP_LOGI(TAG, "UniqueID: %s", String(gLora.localUniqueId));
  ESP_LOGI(TAG, "Pass <= 65535: %s", String(gLora.registered_password));
  return true;
}
