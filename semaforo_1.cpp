// Configuração dos LEDs
const int ledVerde = 12;
const int ledAmarelo = 13;
const int ledVermelho = 14;

// Classe Semaforo
class Semaforo {
  private:
    int ledVerde, ledAmarelo, ledVermelho;
    bool cicloAberto; // Estado do ciclo (aberto ou fechado)
    
  public:
    // Construtor
    Semaforo(int lv, int la, int lr) 
      : ledVerde(lv), ledAmarelo(la), ledVermelho(lr), cicloAberto(true) {
      pinMode(ledVerde, OUTPUT);
      pinMode(ledAmarelo, OUTPUT);
      pinMode(ledVermelho, OUTPUT);
    }

    // Métodos de controle do semáforo
    void sinalVermelho() {
      digitalWrite(ledVerde, LOW);
      digitalWrite(ledAmarelo, LOW);
      digitalWrite(ledVermelho, HIGH);
    }

    void sinalAmarelo() {
      digitalWrite(ledVerde, LOW);
      digitalWrite(ledVermelho, LOW);
      digitalWrite(ledAmarelo, HIGH);
      delay(2000);
      digitalWrite(ledAmarelo, LOW);
    }

    void sinalVerde() {
      digitalWrite(ledVermelho, LOW);
      digitalWrite(ledAmarelo, LOW);
      digitalWrite(ledVerde, HIGH);
      delay(2000);
      digitalWrite(ledVerde, LOW);
    }

    // Verifica e alterna o ciclo
    void verifyCycle() {
      if (cicloAberto) {
        // Executa o ciclo de sinal amarelo e verde
        sinalAmarelo();
        sinalVerde();
      } else {
        // Mantém o sinal vermelho
        sinalVermelho();
      }
    }

    // Métodos para alternar o ciclo manualmente (simulando controle externo)
    void abrirCiclo() {
      cicloAberto = true;
    }

    void fecharCiclo() {
      cicloAberto = false;
    }
};

Semaforo semaforo(ledVerde, ledAmarelo, ledVermelho); // Instância do semáforo

void setup() {
  Serial.begin(115200);
  semaforo.fecharCiclo(); // Inicializa o semáforo no sinal vermelho
}

void loop() {
  semaforo.verifyCycle();
  delay(1000);

  // Simulação para alternar o ciclo
  // Aperte o botão de reset para reiniciar o ciclo manualmente
  semaforo.abrirCiclo(); // Abre o ciclo para que o semáforo passe ao amarelo e verde
  delay(8000);           // Tempo de teste entre os ciclos
  semaforo.fecharCiclo(); // Fecha o ciclo para que o semáforo fique no vermelho
}
