#include <WiFi.h>
#include <PubSubClient.h>

// SSID e senha da rede
#define SSID "Calebe S10+"
#define PASS "calebes10plus"

// Host e Porta do MQTT
#define MQTT_HOST "broker.emqx.io"
// #define MQTT_USERNAME "ESP32_1"
// #define MQTT_PASSWORD "Ap0rt@l123"
#define MQTT_PORT 1883

#define COMMAND_TOPIC "grupo02/command"
#define CALLBACK_TOPIC "grupo02/callback"

WiFiClient espClient;
PubSubClient client(espClient);

// Classe responsável por gerenciar o WiFi e os LEDs
class WiFiManager {

  private:
    StateMachine* stateMachine;
    int ledPin;

    void blinkLed(int ledPin) {
        bool state = digitalRead(ledPin);
        digitalWrite(ledPin, !state);
    }

    int connectionTimeout = 5000;

  public:
      WiFiManager(StateMachine* _stateMachine, int _ledPin) : 
        ledPin(_ledPin), 
        stateMachine(_stateMachine)
      {
        pinMode(ledPin, OUTPUT);
      }

      void connect(const char* ssid, const char* pass) 
      { 
        WiFi.begin(ssid, pass, 6);

        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED) {
          if (millis() - start >= connectionTimeout) {
            Serial.println("Falha ao conectar na rede.");
            return;
          }
          delay(500);
        } 
      }

      void verifyConnection(int ledPin) {
        if (WiFi.status() == WL_CONNECTED) 
          { digitalWrite(ledPin, HIGH); return; }

        if (millis() - stateMachine->get_wifiMillis() >= 500) {
          stateMachine->set_wifiMillis(millis());
          blinkLed(ledPin);
        }
      }
};

class MQTT {
  private:
    PubSubClient* client;
    const char* mqtt_server;
    int mqtt_port;

  public:
    MQTT(const char* mqtt_server, int mqtt_port, PubSubClient* _pubSubClient) 
      : client(_pubSubClient) {
      this->mqtt_server = mqtt_server;
      this->mqtt_port = mqtt_port;

      client->setServer(mqtt_server, mqtt_port);
      client->setCallback(callback);
    }

    void verifyConnection() {
      if (!client->connected()) {
        reconnect();
      }
      client->loop();
    }

    void publish(const char* topic, const char* message) {
      if (client->connected()) {
        client->publish(topic, message);
      }
    }

    void encryptMessage(const char* message) {

    }

    void reconnect() {
      String clientId = "ESP32_Client";
      while (!client->connected()) {
        Serial.print("Conectando ao broker MQTT...");
        if (client->connect(clientId.c_str())) {
          Serial.println("Conectado.");
          client->subscribe(COMMAND_TOPIC, 1);
        } else {
          Serial.print("Falha na conexão, rc=");
          Serial.print(client->state());
          Serial.println(" Tentando novamente em 5 segundos.");
          delay(5000);
        }
      }
    }
};

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Mensagem recebida: ");
  Serial.println(topic);
  for(int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
  }
  Serial.println();
}

// Classe para representar o semáforo
class Semaforo {
  int vermelho;
  int amarelo;
  int verde;
  int currentCycle = 1;

public:
  // Construtor para inicializar as portas
  Semaforo(int p_vermelho, int p_amarelo, int p_verde) {
    vermelho = p_vermelho;
    amarelo = p_amarelo;
    verde = p_verde;
  }

  // Método para configurar as portas
  void configurar() {
    pinMode(vermelho, OUTPUT);
    pinMode(amarelo, OUTPUT);
    pinMode(verde, OUTPUT);
  }

  // Método para definir o sinal no vermelho
  void sinalVermelho() {
    digitalWrite(vermelho, HIGH);
    digitalWrite(amarelo, LOW);
    digitalWrite(verde, LOW);
  }

  // Método para definir o sinal no amarelo
  void sinalAmarelo() {
    digitalWrite(vermelho, LOW);
    digitalWrite(amarelo, HIGH);
    digitalWrite(verde, LOW);
  }

  // Método para definir o sinal no verde
  void sinalVerde() {
    digitalWrite(vermelho, LOW);
    digitalWrite(amarelo, LOW);
    digitalWrite(verde, HIGH);
  }

  void desligado() {
    digitalWrite(vermelho, LOW);
    digitalWrite(amarelo, LOW);
    digitalWrite(verde, LOW);
  }

  void verifyCycle() {
    int analogValue = analogRead(4);
    
    Serial.print("Analog Value = ");
    Serial.println(analogValue);

    if (analogValue < 200) {
      cicloNoturno();
      return;
    }

    if (currentCycle == 1) {
      cicloAberto();
      currentCycle = 0;
      return;
    }

    cicloFechado();
  }

  void cicloAberto() {
    sinalVerde();
    delay(4000);

    sinalAmarelo();
    delay(2000);
  }

  void cicloFechado() {
    sinalVermelho();
  }

  void cicloNoturno() {
    sinalAmarelo();
    delay(1000);
    desligado();
    delay(1000);
  }

};

// Instanciando o semáforo
Semaforo semaforo(2, 32, 33);

void setup() {
  semaforo.configurar();  // Aciona a função de configuração das portas do semáforo
  wifiManager = new WiFiManager(&stateMachine, WIFI_LED);
  mqtt = new MQTT(MQTT_HOST, MQTT_PORT, &client);

  wifiManager->connect(SSID, PASS);

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600); 
}

void loop() {
  wifiManager->verifyConnection(WIFI_LED);
  mqtt->verifyConnection();

  semaforo.verifyCycle();
}