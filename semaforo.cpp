// Bibliotecas necessárias
#include <WiFi.h>        
#include <PubSubClient.h> 
#include <WiFiClientSecure.h> 
#include <Ticker.h>      
#include <Arduino.h>
#include <Wire.h>
#include <sstream>
#include <ArduinoJson.h>
#include <esp_system.h>

// Pinos para o primeiro conjunto de LEDs
#define LED_VERMELHO_1 32
#define LED_AMARELO_1 33
#define LED_VERDE_1 26

// Pinos para o segundo conjunto de LEDs
#define LED_VERMELHO_2 18
#define LED_AMARELO_2 16
#define LED_VERDE_2 4

// Pino do sensor LDR
#define SENSOR_LUZ 34

// Configuração da rede Wi-Fi
const char* redeWiFi = "RedmiTesta";
const char* senhaWiFi = "12Teste34";

// Configuração do broker MQTT
const char* servidorMQTT = "69cf4987764d4e8bb28208efac5443a9.s1.eu.hivemq.cloud";
const int portaMQTT = 8883;
const char* usuarioMQTT = "ESP32_1";
const char* senhaMQTT = "Ap0rt4l123";

String mensagemRecebida;

// Objetos para rede e MQTT
WiFiClientSecure clienteSeguro;
PubSubClient mqttCliente(clienteSeguro);

// Reconexão ao MQTT
void verificarMQTT() {
  while (!mqttCliente.connected()) {
    Serial.println("Reconectando ao broker MQTT...");
    if (mqttCliente.connect("ClienteMQTT", usuarioMQTT, senhaMQTT)) {
      Serial.println("Conectado ao broker MQTT!");
      mqttCliente.subscribe("controle/leds");
    } else {
      Serial.println("Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

// Configuração Wi-Fi
void conectarWiFi() {
  Serial.println("Tentando conectar ao Wi-Fi...");
  WiFi.begin(redeWiFi, senhaWiFi);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConexão Wi-Fi estabelecida!");
  clienteSeguro.setInsecure();
}

// Callback para mensagens MQTT
void processarMensagem(char* topico, byte* dados, unsigned int tamanho) {
  mensagemRecebida = "";
  for (unsigned int i = 0; i < tamanho; i++) {
    mensagemRecebida += (char)dados[i];
  }
  Serial.println("Mensagem recebida: " + mensagemRecebida);
}

// Classe para controle dos LEDs
class ControladorLED {
  private:
    int ledVermelho;
    int ledAmarelo;
    int ledVerde;
  public:
    ControladorLED(int pinoVermelho, int pinoAmarelo, int pinoVerde) {
      this->ledVermelho = pinoVermelho;
      this->ledAmarelo = pinoAmarelo;
      this->ledVerde = pinoVerde;
    }

    void configurar() {
      pinMode(this->ledVermelho, OUTPUT);
      pinMode(this->ledAmarelo, OUTPUT);
      pinMode(this->ledVerde, OUTPUT);
    }

    void acenderVerde() {
      digitalWrite(this->ledVermelho, LOW);
      digitalWrite(this->ledAmarelo, LOW);
      digitalWrite(this->ledVerde, HIGH);
    }

    void ativarAmarelo() {
      digitalWrite(this->ledVermelho, LOW);
      digitalWrite(this->ledAmarelo, HIGH);
      digitalWrite(this->ledVerde, LOW);
    }

    void acenderVermelho() {
      digitalWrite(this->ledVermelho, HIGH);
      digitalWrite(this->ledAmarelo, LOW);
      digitalWrite(this->ledVerde, LOW);
    }

    void desligarTudo() {
      digitalWrite(this->ledVermelho, LOW);
      digitalWrite(this->ledAmarelo, LOW);
      digitalWrite(this->ledVerde, LOW);
    }

    void piscarAmarelo() {
      digitalWrite(this->ledAmarelo, !digitalRead(this->ledAmarelo));
    }
};

// Objetos para controle dos LEDs
Ticker temporizadorLEDs;
Ticker temporizadorNoturno1;
Ticker temporizadorNoturno2;
ControladorLED ledsConjunto1(LED_VERMELHO_1, LED_AMARELO_1, LED_VERDE_1);
ControladorLED ledsConjunto2(LED_VERMELHO_2, LED_AMARELO_2, LED_VERDE_2);

#define LEDS_1_VERDE 0
#define LEDS_1_AMARELO 1
#define LEDS_1_VERMELHO 2
#define LEDS_2_AMARELO 3
#define LEDS_2_VERMELHO 4

int estadoAtual = LEDS_1_VERDE;

void atualizarEstado() {
  switch (estadoAtual) {
    case LEDS_1_VERDE:
      ledsConjunto1.acenderVerde();
      ledsConjunto2.acenderVermelho();
      estadoAtual = LEDS_1_AMARELO;
      temporizadorLEDs.once(3, atualizarEstado);
      break;

    case LEDS_1_AMARELO:
      ledsConjunto1.ativarAmarelo();
      estadoAtual = LEDS_1_VERMELHO;
      temporizadorLEDs.once(1, atualizarEstado);
      break;

    case LEDS_1_VERMELHO:
      ledsConjunto1.acenderVermelho();
      ledsConjunto2.acenderVerde();
      estadoAtual = LEDS_2_AMARELO;
      temporizadorLEDs.once(3, atualizarEstado);
      break;

    case LEDS_2_AMARELO:
      ledsConjunto2.ativarAmarelo();
      estadoAtual = LEDS_2_VERMELHO;
      temporizadorLEDs.once(1, atualizarEstado);
      break;

    case LEDS_2_VERMELHO:
      ledsConjunto2.acenderVermelho();
      ledsConjunto1.acenderVerde();
      estadoAtual = LEDS_1_VERDE;
      temporizadorLEDs.once(1, atualizarEstado);
      break;
  }
}

// Setup inicial
void setup() {
  Serial.begin(9600);
  conectarWiFi();
  mqttCliente.setServer(servidorMQTT, portaMQTT);
  mqttCliente.setCallback(processarMensagem);

  ledsConjunto1.configurar();
  ledsConjunto2.configurar();
  pinMode(SENSOR_LUZ, INPUT);
  
  atualizarEstado();
}

bool modoNoturno = false;

// Loop principal
void loop() {
  if (!mqttCliente.connected()) {
    verificarMQTT();
  }
  mqttCliente.loop();

  int leituraLDR = analogRead(SENSOR_LUZ);
  String valorSensor = String(leituraLDR);
  mqttCliente.publish("controle/leds", valorSensor.c_str());
  Serial.println("LDR: " + valorSensor);

  if(mensagemRecebida == "CONJUNTO_1") {
    ledsConjunto1.desligarTudo();
    ledsConjunto2.desligarTudo();
    estadoAtual = LEDS_1_VERDE;
  } else if(mensagemRecebida == "CONJUNTO_2") {
    ledsConjunto1.desligarTudo();
    ledsConjunto2.desligarTudo();
    estadoAtual = LEDS_1_VERMELHO;
  }

  if(leituraLDR <= 300) {
    if(!modoNoturno) {
      modoNoturno = true;
      temporizadorLEDs.detach(); // Desativa o timer do modo normal
      ledsConjunto1.desligarTudo();
      ledsConjunto2.desligarTudo();
      // Inicia o pisca-pisca dos amarelos
      temporizadorNoturno1.attach(1, []() { ledsConjunto1.piscarAmarelo(); });
      temporizadorNoturno2.attach(1, []() { ledsConjunto2.piscarAmarelo(); });
    }
  } else {
    if(modoNoturno) {
      modoNoturno = false;
      temporizadorNoturno1.detach(); // Desativa os timers do modo noturno
      temporizadorNoturno2.detach();
      ledsConjunto1.desligarTudo();
      ledsConjunto2.desligarTudo();
      atualizarEstado();
    }
  }
}
