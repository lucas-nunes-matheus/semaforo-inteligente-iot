# Semáforo Inteligente com ESP32, LDR e MQTT

- [Vídeo do Projeto em funcionamento](https://youtu.be/5fIagZ0IAeo)

## Membros do grupo:
- Vinicius Testa Passos
- Arthur Bretas
- Vinicius Gomes Ibiapina
- Lucas Matheus Nunes
- Davi D'avila Versan
- Calebe Matias

## Índice

- [Introdução](#introdução)
- [Objetivos](#objetivos)
- [Materiais Necessários](#materiais-necessários)
- [Parte 1: Configuração do Semáforo](#parte-1-configuração-do-semáforo)
  - [Esquema Elétrico](#esquema-elétrico)
  - [Montagem Passo a Passo](#montagem-passo-a-passo)
  - [Código Fonte](#código-fonte)
  - [Explicação do Código](#explicação-do-código)
- [Parte 2: Integração com MQTT](#parte-2-integração-com-mqtt)
  - [Configuração do Broker MQTT](#configuração-do-broker-mqtt)
  - [Implementação no Código](#implementação-no-código)
  - [Teste de Comunicação](#teste-de-comunicação)
- [Resultados](#resultados)
- [Conclusão](#conclusão)
- [Referências](#referências)

---

## Introdução

Este projeto apresenta um **semáforo inteligente** utilizando ESP32, LEDs, sensor LDR e integração com o protocolo MQTT. O objetivo é criar um sistema de controle de tráfego ajustável de forma local e remota, com conectividade para monitoramento em tempo real.

---

## Objetivos

- Controlar dois conjuntos de LEDs representando semáforos.
- Detectar a luminosidade ambiente com um sensor LDR para alternar entre modos de operação.
- Integrar o semáforo a um broker MQTT para controle e monitoramento remotos.

---

## Materiais Necessários

- 1 x Placa ESP32
- LEDs (vermelho, amarelo, verde)
- 1 x Sensor LDR
- Resistores variados (10kΩ para o LDR, outros conforme necessário)
- Protoboard e jumpers
- Computador com Arduino IDE instalado
- Conexão Wi-Fi
- Conta em um broker MQTT (HiveMQ usado como exemplo)

---

## Parte 1: Configuração do Semáforo

### Esquema Elétrico

O diagrama abaixo apresenta a ligação dos LEDs e do sensor LDR ao ESP32:

- **LEDs do semáforo 1:** GPIO 32, 33, 26 (vermelho, amarelo, verde)
- **LEDs do semáforo 2:** GPIO 18, 16, 4 (vermelho, amarelo, verde)
- **Sensor LDR:** GPIO 34 (pino analógico)

### Montagem Passo a Passo

1. **Conexão dos LEDs:**
   - Conecte os LEDs aos pinos indicados e adicione resistores para limitar a corrente.

2. **Conexão do Sensor LDR:**
   - Um terminal no **3.3V** e o outro no GPIO 34, com resistor de 10kΩ para o GND, formando um divisor de tensão.

3. **Verificação:**
   - Teste as conexões antes de carregar o código.

---

### Código Fonte

```cpp
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
```

---

### Explicação do Código

1. **Classe `ControladorLED`:** Garante modularidade e reaproveitamento do código para os dois semáforos.
2. **Lógica de Controle:**
   - **Modo Noturno:** Quando a luminosidade ambiente é baixa, os LEDs amarelos piscam.
   - **Modo Normal:** Alternância entre verde e vermelho para os dois semáforos.
3. **MQTT:** Integração para controle remoto futuro.

---

## Parte 2: Integração com MQTT

### Configuração do Broker MQTT

- Broker: HiveMQ Cloud
- Porta: 8883 (segura)
- Usuário e senha fornecidos pelo serviço.

### Implementação no Código

- **Bibliotecas:** `WiFiClientSecure` para TLS e `PubSubClient` para MQTT.
- **Callback:** Recebe mensagens para controle dos LEDs.

### Teste de Comunicação

1. Configure o tópico "controle/leds" no broker.
2. Publique mensagens e verifique a resposta no ESP32.

---

## Resultados

- Detecção precisa da luminosidade ambiente.
- Alternância confiável entre os estados dos LEDs.
- Comunicação com o broker MQTT estabelecida.

---

## Conclusão

Este projeto demonstrou como implementar um sistema de semáforo inteligente com controle remoto, mostrando viabilidade em cenários de IoT e cidades inteligentes.

---

## Referências

- [Vídeo do Projeto em funcionamento](https://youtu.be/5fIagZ0IAeo)
- [Documentação do ESP32](https://www.espressif.com/en/products/socs/esp32/resources)
- [HiveMQ MQTT Client](https://www.hivemq.com/)
- [Arduino IDE](https://www.arduino.cc/en/software)
