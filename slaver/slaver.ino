//********************************************
// COBRA NONATO (Módulo slaver - Nó 1)
//********************************************

#define TAG "SLAVER"

#include "Ultrasonic.h"
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
Ultrasonic gUltrasonic(PIN_TRIGGER, PIN_ECHO);

// Sensor de Umidade e Temperatura DHT22
DHT_Unified gDht(PIN_DHT, DHTTYPE);

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
  ESP_LOGI(TAG, "Configurando módulo LoRa ...");
  while (!lora_setup_completed) {
    lora_setup_completed = config_lora();
  }
  ESP_LOGI(TAG, "done.");


  // Configurando os sensores
  begin_rain_sensor();
  //begin_dht_sensor();
}

void loop() {
  uint16_t id;
  uint8_t command;
  uint8_t payload[10];
  uint8_t payloadSize;

  // Verifica se há algum comando do master
  ESP_LOGI(TAG, "1");
  if (gLora.ReceivePacketCommand(&id, &command, payload, &payloadSize, 1000)) {
      ESP_LOGI(TAG, "2");
    if (command == DATA_REQUEST_CMD) {
      ESP_LOGI(TAG, "Lendos os dados dos sensores.");
      // Lendo os dados do sensor DHT22
      sensors_event_t event;
      float temperature, humidity;
      // gDht.temperature().getEvent(&event);
      // temperature = isnan(event.temperature) ? -1 : event.temperature;
      // gDht.humidity().getEvent(&event);
      // humidity = isnan(event.relative_humidity) ? -1 : event.relative_humidity;
      temperature = -1.1;
      humidity = -1.1;

      // Lendo os dados do sensor Ultrassonico
      // long microsec = gUltrasonic.timing();
      // float distance = gUltrasonic.convert(microsec, Ultrasonic::CM);
      float distance = -1.1;

      // Lendo os dados do sensor de Chuva
      int rain_analog = -1; //gLora.read_gpio(ID, PIN_RAIN_ANALOG); // analogRead(PIN_RAIN_ANALOG);
      uint8_t rain_dig = 1; //gLora.read_gpio(ID, PIN_RAIN_DIG); // digitalRead(PIN_RAIN_DIG);

      // Prepara os dados para envio
      ESP_LOGI(TAG, "Preparando dados para serem enviados.");
      uint8_t sensorData[8];
      uint16_t humidityInt = (uint16_t)(humidity * 100); // duas casas decimais
      uint16_t temperatureInt = (uint16_t)(temperature * 100); // duas casas decimais
      uint16_t distanceInt = (uint16_t)(distance * 100); // distância em cm com duas casas decimais

      // Dividindo os valores em bytes
      sensorData[0] = (humidityInt >> 8) & 0xFF;
      sensorData[1] = humidityInt & 0xFF;
      sensorData[2] = (temperatureInt >> 8) & 0xFF;
      sensorData[3] = temperatureInt & 0xFF;
      sensorData[4] = rain_dig;  // 1 byte para chuva digital (se está chovendo ou não)
      sensorData[5] = rain_analog & 0xFF;  // 1 byte para valor analógico do sensor de chuva
      sensorData[6] = (distanceInt >> 8) & 0xFF;
      sensorData[7] = distanceInt & 0xFF;

      // Envia os dados de volta para o master
      ESP_LOGI(TAG, "Enviando dados para o MASTER.");
      gLora.PrepareFrameCommand(id, RESPONSE_CMD, sensorData, 8);
      gLora.SendPacket();
    }
  }
}

//*****************************
// AUXILIARY FUNCTIONS
//*****************************
bool config_lora() {
  if (gLora.localId != ID) {
    if (!gLora.setnetworkId(ID)) {
      ESP_LOGE(TAG, "Erro ao definir o novo ID");
      return false;
    }
    ESP_LOGD(TAG, "ID configurado com sucesso!");

    if (!gLora.config_bps(BW500, SF_LoRa_7, CR4_5)) {
      ESP_LOGE(TAG, "Erro ao configurar bps");
      return false;
    }
    ESP_LOGD(TAG, "Parametros LoRa configurados com sucesso!");

    if (!gLora.config_class(LoRa_CLASS_C, LoRa_WINDOW_15s)) {
      ESP_LOGE(TAG, "Erro ao configurar a classe");
      return false;
    }
    ESP_LOGD(TAG, "Modo de operacao configurado com sucesso!");

    if (!gLora.setpassword(PASSWORD)) {
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

void begin_rain_sensor() {
  gLora.config_analog_gpio(PIN_RAIN_ANALOG);
  gLora.config_digital_gpio(PIN_RAIN_DIG, LoRa_NOT_PULL, LoRa_INOUT_DIGITAL_INPUT, LoRa_LOGICAL_LEVEL_LOW);
  // pinMode(PIN_RAIN_ANALOG, INPUT);
  // pinMode(PIN_RAIN_DIG, INPUT);
  ESP_LOGI(TAG, "Pinos do sensor de chuva configurados.");
}

void begin_dht_sensor() {
  sensor_t sensor_dht;
  gLora.config_analog_gpio(PIN_DHT);

  gDht.begin();                               // inicializa a função
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

  uint32_t gDelayBetweenReadings = sensor_dht.min_delay / 1000;  // define o atraso entre as leituras
}