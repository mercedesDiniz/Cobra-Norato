//********************************************
// COBRA NONATO (Módulo master - Nó 0)
//********************************************

#define TAG "MASTER"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "LoRaMESH.h"
#include "config.h"

//********************************************
// GLOBAL VARIABLES AND FUNCTION PROTOTYPES
//********************************************
static bool config_lora(void);
static bool SetupWiFi(void);
static void callback(char* topic, byte* payload, unsigned int length);
static void reconnect(void);
// String message_mqtt(SensorData data);
const char* message_mqtt(SensorData data);
static void SetupLEDs(void);

enum State state = REST; // Controls the states machine (switch case)

// Módulo LoRa
LoRaMESH gLora(&Serial_ESP_LORA);

// Cliente WiFi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);

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
  // Configurando leds
  SetupLEDs();

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

  // Configurando WiFi
  SetupWiFi();

  // Configurando o servidor MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// Infinite loop
void loop() {
  static double time_cycle_starts = millis();
  static SensorData data; 
  static uint8_t gDataSubmissionPending = 0; 
  
  // Finite-state machine (FSM)
  switch (state) {
    case SAMPLE:
    { 
      ESP_LOGI(TAG, "\nEntering SAMPLE state");
      digitalWrite(LEDR, LOW);
      analogWriteFrequency(10); 

      double t0 = millis();
      // Solicita dados dos sensores ao nó slave
      // ESP_LOGI(TAG, "Solicitando dados ...");
      // gLora.PrepareFrameCommand(1, DATA_REQUEST_CMD, nullptr, 0); // Nó sensor 01
      // gLora.SendPacket();
      // delay(500);
          
      // Aguardar a resposta do slave
      uint16_t id;
      uint8_t command;
      uint8_t payload[10];
      uint8_t payloadSize;

      while((millis()-t0) < WAITING_TIME_FOR_NEW_DATA_FROM_SENSOR_NODE){
        if (gLora.ReceivePacketCommand(&id, &command, payload, &payloadSize, 3000)) {
          ESP_LOGD(TAG, "ID: %s, Command: %s, Payload Size: %s", String(id), String(command), String(payloadSize));
          
          // Verifica se é uma resposta válida e o tamanho certo
          if (command == RESPONSE_CMD && payloadSize == 9) {
            data.humidity = ((payload[0] << 8) | payload[1]) / 100.0;     // Humidade com duas casas decimais
            data.temperature = ((payload[2] << 8) | payload[3]) / 100.0;  // Temperatura com duas casas decimais
            data.rain_dig = payload[4];                                   // Chuva digital (1 ou 0)
            data.rain_analog = ((payload[5] << 8) | payload[6]);          // Leitura analógica do sensor de chuva (0-4095)
            data.distance = ((payload[7] << 8) | payload[8]) / 100.0;     // Distância em cm com duas casas decimais
            data.message = message_mqtt(data);

            // Exibir os dados
            ESP_LOGI(TAG, "Umidade: %s %, Temperatura: %sºC", String(data.humidity), String(data.temperature));
            ESP_LOGI(TAG, "Chuva (dig): %s, Chuva (analog): %s", String(data.rain_dig), String(data.rain_analog), String(data.distance));
            ESP_LOGI(TAG, "Distância: %s %s", String(data.distance), "cm");

            gDataSubmissionPending++;
            break;
          }        
        }
      }

      state = ((gDataSubmissionPending > 0) ? CONNECT : REST);
      break;
    }

    case CONNECT:
    { 
      ESP_LOGI(TAG, "\nEntering CONNECT state");
      digitalWrite(LEDR, LOW);
      analogWriteFrequency(20);
      
      if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();
      double t0 = millis();
      while (WiFi.status() != WL_CONNECTED) {
        if (millis() - t0 > 5000) {
          ESP_LOGE(TAG, "WiFi connection failed");
          break;
        }
      }

      if (!client.connected()) reconnect();
      client.loop();

      state = ((WiFi.status() == WL_CONNECTED && client.connected()) ? TRANSMIT : REST);
      break;
    }

    case TRANSMIT:
    {
      ESP_LOGI(TAG, "\nEntering TRANSMIT state");
      digitalWrite(LEDR, LOW);
      analogWriteFrequency(10);

      // Criando a mensagem com os dados
      ESP_LOGI(TAG, "MESSAGE: %s", data.message);
      
      // Publicar uma mensagem no tópico MQTT
      client.publish(mqtt_topic, data.message);
      // client.publish(mqtt_topic,"{'value':32}");
      gDataSubmissionPending--;

      state = REST;
      break;
    }

    case REST:
    { 
      ESP_LOGI(TAG, "\nEntering REST state");
      digitalWrite(LEDR, HIGH);
      analogWriteFrequency(20);

      double elapsed_time =  millis() - time_cycle_starts;

      while (elapsed_time < TIME_BETWEEN_SAMPLING_WINDOWS){
        elapsed_time =  millis() - time_cycle_starts;
        // ESP_LOGD(TAG, "elapsed_time = %s", String(elapsed_time));
      }

      state = SAMPLE;
      break;
    }
  }
}

//*****************************
// AUXILIARY FUNCTIONS
//*****************************

// Função de configurações do módulo LoRa mesh (o ID definido no config.h indica de é master ou slaver)
static bool config_lora(void){
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

// Função par iniciar a conecção com o WiFi
static bool SetupWiFi(void) {
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  ESP_LOGD(TAG, "Connecting to WiFi...");
  double t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t0 > 10000) {
      ESP_LOGE(TAG, "WiFi connection failed");
      return false;
    }
  }
  ESP_LOGI(TAG, "IP address: %s\n", WiFi.localIP().toString().c_str());
  return true;
}

// Função de callback para quando uma mensagem é recebida
static void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Função para conectar ao broker MQTT
static void reconnect(void) {
  // Loop até conectar ao broker
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    // Tentativa de conexão
    if (client.connect(mqtt_client_id)) {
      Serial.println("Conectado!");
      // Subscrever ao tópico
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("Falha na conexão, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

// Função que formata a mensagem MQTT
// String message_mqtt(SensorData data) {
//   return ("{\"Humidity\":" + String(data.humidity) + 
//           ",\"Temperature\":" + String(data.temperature) + 
//           ",\"Rain_dig\":" + String(data.rain_dig) + 
//           ",\"Rain_analog\":" + String(data.rain_analog) + 
//           ",\"Distance\":" + String(data.distance) + 
//           "}");
// }

const char* message_mqtt(SensorData data) {
  static char buffer[200]; 
  snprintf(buffer, sizeof(buffer),
           "{\"Humidity\":%.2f,\"Temperature\":%.2f,\"Rain_dig\":%d,\"Rain_analog\":%d,\"Distance\":%.2f}",
           data.humidity, data.temperature, data.rain_dig, data.rain_analog, data.distance);
  return buffer;
}

// Função de configuração dos pinos dos leds sinalizadores
static void SetupLEDs(void) {
  analogReadResolution(12); 
  analogWriteFrequency(20); // Set the PWM frequency to 1 Hz 
  
  pinMode(LEDR, OUTPUT); // Estados : 1 ou 0
  pinMode(LEDG, OUTPUT); // Pisca em variadas frequencias

  digitalWrite(LEDR, HIGH);
  analogWrite(LEDG, 127); // Generate PWM signal with 50% duty cycle
}
