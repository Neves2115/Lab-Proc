// ============================================================
//  DESAFIO - Semaforo inteligente com modo noturno
//  ESP32-C3 Super Mini
//  - Ciclo normal: VERDE -> AMARELO -> VERMELHO
//  - Modo noturno (baixa luminosidade pelo LDR): amarelo
//    piscando a 1 Hz
//  - Botao de pedestre por interrupcao: antecipa a troca de
//    forma SEGURA (verde -> amarelo -> vermelho) (secao 8).
// ============================================================

// ---- Pinos (ESP32-C3 Super Mini) ----
const int ldrPin     = 0;  // GPIO3 = ADC1 (LDR)
const int rgbLedPin  = 8;  // GPIO8 = LED RGB embarcado (representa o foco do semaforo)
const int botaoPed   = 7;  // GPIO7 = botao de pedestre (pull-down externo -> RISING)

// ---- Parametros ----
const int limiarNoturno = 1500;   // abaixo disso = "noite" (calibrar)
const int brilho        = 40;     // 0-255

// Duracao de cada fase do ciclo diurno (ms)
const unsigned long tempoVerde    = 5000;
const unsigned long tempoAmarelo  = 2000;
const unsigned long tempoVermelho = 5000;

// Duracao da emergencia no modo noturno
const unsigned long tempoEmergencia = 3000;

// ---- Maquina de estados ----
enum Estado { VERDE, AMARELO, VERMELHO, NOTURNO };
Estado estado = VERDE;
unsigned long inicioEstado = 0;

// ---- Tempo / leituras ----
int ldrVal = 0;
unsigned long prevLdr   = 0;
unsigned long prevBlink = 0;
bool piscaLigado = false;

// ---- Emergencia no modo noturno ----
bool emergenciaAtiva = false;
unsigned long inicioEmergencia = 0;

// ---- Estado compartilhado com a ISR ----
volatile bool pedidoTravessia = false;
volatile unsigned long ultimoPed = 0;
const unsigned long debounceMs = 50;

void setCor(int r, int g, int b) { neopixelWrite(rgbLedPin, r, g, b); }

// ISR curta: debounce + sinaliza o pedido.
void IRAM_ATTR isrPedestre() {
  unsigned long agora = millis();
  if (agora - ultimoPed > debounceMs) {
    ultimoPed = agora;
    pedidoTravessia = true;
  }
}

void trocaEstado(Estado novo) {
  estado = novo;
  inicioEstado = millis();

  // Se sair do modo noturno, limpa a emergencia
  if (novo != NOTURNO) {
    emergenciaAtiva = false;
  }

  switch (novo) {
    case VERDE:    setCor(0, brilho, 0);          break; // verde
    case AMARELO:  setCor(brilho, brilho, 0);     break; // amarelo
    case VERMELHO: setCor(brilho, 0, 0);          break; // vermelho
    case NOTURNO:
      piscaLigado = false;
      prevBlink = millis();
      setCor(0, 0, 0);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);

  pinMode(botaoPed, INPUT);   // pull-down externo
  attachInterrupt(digitalPinToInterrupt(botaoPed), isrPedestre, RISING);

  trocaEstado(VERDE);
}

void loop() {
  unsigned long agora = millis();

  // Leitura do LDR a 1 Hz
  if (agora - prevLdr >= 1000) {
    prevLdr = agora;
    ldrVal = analogRead(ldrPin);
  }

  // Transicao entre modo diurno e noturno
  if (ldrVal > limiarNoturno && estado != NOTURNO) {
    Serial.println("Modo noturno: amarelo piscando a 1 Hz.");
    trocaEstado(NOTURNO);
    pedidoTravessia = false;   // evita pedido antigo entrar no modo noturno
  } else if (ldrVal <= limiarNoturno && estado == NOTURNO) {
    Serial.println("Amanheceu: retomando ciclo normal.");
    trocaEstado(VERMELHO);  // retoma pelo vermelho (mais seguro)
  }

  switch (estado) {

    case NOTURNO:
      // Botao no modo noturno: vermelho por 3 segundos
      if (pedidoTravessia && !emergenciaAtiva) {
        pedidoTravessia = false;
        emergenciaAtiva = true;
        inicioEmergencia = agora;
        setCor(brilho, 0, 0);
        Serial.println("Emergencia no modo noturno: vermelho por 3 segundos.");
      }

      if (emergenciaAtiva) {
        setCor(brilho, 0, 0);
        if (agora - inicioEmergencia >= tempoEmergencia) {
          emergenciaAtiva = false;
          prevBlink = agora;
          piscaLigado = false;
          setCor(0, 0, 0);
        }
      } else {
        // 1 Hz = um ciclo liga/desliga por segundo -> alterna a cada 500 ms
        if (agora - prevBlink >= 500) {
          prevBlink = agora;
          piscaLigado = !piscaLigado;
          if (piscaLigado) setCor(brilho, brilho, 0);
          else             setCor(0, 0, 0);
        }
      }
      break;

    case VERDE:
      // Pedido de pedestre antecipa a troca; senao, segue o tempo normal
      if (pedidoTravessia || (agora - inicioEstado >= tempoVerde)) {
        pedidoTravessia = false;
        trocaEstado(AMARELO);
      }
      break;

    case AMARELO:
      if (agora - inicioEstado >= tempoAmarelo) {
        trocaEstado(VERMELHO);
      }
      break;

    case VERMELHO:
      // Atende a travessia durante o vermelho (com seguranca)
      if (pedidoTravessia) {
        pedidoTravessia = false;
        Serial.println("Travessia de pedestres liberada.");
      }
      if (agora - inicioEstado >= tempoVermelho) {
        trocaEstado(VERDE);
      }
      break;
  }
}