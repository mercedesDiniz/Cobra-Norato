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

// Módulo LoRa
LoRaMESH gLora(&Serial_ESP_LORA);

// Sensor Ultrassonico HC-SR04
Ultrasonic gUltrasonic(PIN_TRIGGER, PIN_ECHO);

// Sensor de Umidade e Temperatura DHT22
DHT_Unified gDht(PIN_DHT, DHTTYPE); 
sensors_event_t gEventDht;
uint32_t gDelayBetweenReadings; // em ms    

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
    lora_setup_completed = config_lora_master();
  }

  // Configurando os sensores
  begin_rain_sensor();
  begin_dht_sensor();

}

void loop() {
  // put your main code here, to run repeatedly:

}

//*****************************
// AUXILIARY FUNCTIONS
//*****************************
bool config_lora_master(){
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

  ESP_LOGI(TAG, "LocalID: " + String(gLora.localId));
  ESP_LOGI(TAG, "UniqueID: " + String(gLora.localUniqueId));
  ESP_LOGI(TAG, #include"Pass <= 65535: " + String(gLora.registered_password));
  return true;
}

void begin_rain_sensor(){
  pinMode(PIN_RAIN_ANALOG, INPUT);
  pinMode(PIN_RAIN_DIG, INPUT);
  ESP_LOGI(TAG, "Pinos do sensor de chuva configurados.");
}

void begin_dht_sensor(){
  sensor_t sensor_dht;
  gDht.begin();  // inicializa a função
  gDht.temperature().getSensor(&sensor_dht);  // imprime os detalhes do Sensor de Temperatura

  ESP_LOGI(TAG, "------------------------------------");
  ESP_LOGI(TAG, "Temperatura");
  ESP_LOGI(TAG, "Sensor: %s", sensor_dht.name);
  ESP_LOGI(TAG, "Valor max: %.2f *C", sensor_dht.max_value);
  ESP_LOGI(TAG, "Valor min: %.2f *C", sensor_dht.min_value);
  ESP_LOGI(TAG, "Resolucao: %.2f *C", sensor_dht.resolution);
  ESP_LOGI(TAG, "------------------------------------");

  gDht.humidity().getSensor(&sensor_dht);  // imprime os detalhes do Sensor de Umidade

  ESP_LOGI(TAG, "------------------------------------");
  ESP_LOGI(TAG, "Umidade");
  ESP_LOGI(TAG, "Sensor: %s", sensor_dht.name);
  ESP_LOGI(TAG, "Valor max: %.2f%%", sensor_dht.max_value);
  ESP_LOGI(TAG, "Valor min: %.2f%%", sensor_dht.min_value);
  ESP_LOGI(TAG, "Resolucao: %.2f%%", sensor_dht.resolution);
  ESP_LOGI(TAG, "------------------------------------");

  gDelayBetweenReadings = sensor_dht.min_delay / 1000;  // define o atraso entre as leituras
}