// ============================================================
//  SISTEMA INTEGRADO - Monitoramento + SOS com prioridade
//  ESP32-C3 Super Mini
//  - LDR via ADC, valor no webserver local a 1 Hz
//  - Baixa luminosidade: pisca AMARELO (ciclo de 2 s)
//  - Botao SOS por interrupcao: LED fica VERMELHO por 3 s
//    com PRIORIDADE sobre o pisca amarelo (secao 6.6 do relatorio).
// ============================================================

#include <WiFi.h>

// ---- Rede (Access Point) ----
const char* ssid     = "Controle Web Grupo G";
const char* password = "123456789";
WiFiServer server(80);

// ---- Pinos (ESP32-C3 Super Mini) ----
const int ldrPin    = 3;  // GPIO3 = ADC1 (LDR). NAO use GPIO 34 (nao existe no C3)
const int rgbLedPin = 8;  // GPIO8 = LED RGB embarcado (WS2812)
const int botaoPin  = 7;  // GPIO7 = botao SOS (pull-down externo -> RISING)

// ---- Parametros ----
int ldrVal = 0;
const int limiarBaixaLuz   = 1500;   // calibre no Serial Monitor
const int brilho           = 40;     // 0-255
const unsigned long duracaoVermelho = 3000;  // 3 s de emergencia

// ---- Tempo (loop nao bloqueante) ----
unsigned long prevLdrMillis   = 0;
unsigned long prevBlinkMillis = 0;
unsigned long inicioVermelho  = 0;
bool ledAmareloLigado = false;
bool emergenciaAtiva  = false;

// ---- Estado compartilhado com a ISR ----
volatile bool sosPendente = false;
volatile unsigned long ultimoSos = 0;
const unsigned long debounceMs = 50;

// Cores no LED RGB embarcado
void setApagado()  { neopixelWrite(rgbLedPin, 0, 0, 0); }
void setAmarelo()  { neopixelWrite(rgbLedPin, brilho, brilho, 0); } // R + G
void setVermelho() { neopixelWrite(rgbLedPin, brilho, 0, 0); }      // R

// ISR curta: debounce + sinaliza a flag. Nada de delay/processamento aqui.
void IRAM_ATTR isrSos() {
  unsigned long agora = millis();
  if (agora - ultimoSos > debounceMs) {
    ultimoSos = agora;
    sosPendente = true;
  }
}

void setup() {
  Serial.begin(115200);

  analogSetAttenuation(ADC_11db);
  pinMode(botaoPin, INPUT);   // pull-down externo
  setApagado();

  attachInterrupt(digitalPinToInterrupt(botaoPin), isrSos, RISING);

  WiFi.softAP(ssid, password);
  Serial.print("Access Point no IP: ");
  Serial.println(WiFi.softAPIP());
  server.begin();
}

void loop() {
  unsigned long agora = millis();

  // 1) LDR sempre a 1 Hz (telemetria nao para, nem durante a emergencia)
  if (agora - prevLdrMillis >= 1000) {
    prevLdrMillis = agora;
    ldrVal = analogRead(ldrPin);
  }

  // 2) SOS pendente -> inicia a emergencia
  if (sosPendente) {
    sosPendente = false;
    emergenciaAtiva = true;
    inicioVermelho = agora;
    setVermelho();
    Serial.println("SOS! LED vermelho por 3 s (prioridade).");
  }

  // 3) Controle do LED com PRIORIDADE para a emergencia
  if (emergenciaAtiva) {
    // Enquanto a emergencia esta ativa, o LED fica vermelho e
    // o pisca amarelo e ignorado.
    if (agora - inicioVermelho >= duracaoVermelho) {
      emergenciaAtiva = false;
      setApagado();
    }
  } else {
    // Modo normal: pisca amarelo (ciclo de 2 s) so em baixa luminosidade
    if (ldrVal > limiarBaixaLuz) {
      if (agora - prevBlinkMillis >= 1000) {   // alterna a cada 1 s -> periodo de 2 s
        prevBlinkMillis = agora;
        ledAmareloLigado = !ledAmareloLigado;
        ledAmareloLigado ? setAmarelo() : setApagado();
      }
    } else {
      if (ledAmareloLigado) {       // ao voltar a luz, apaga
        ledAmareloLigado = false;
        setApagado();
      }
      prevBlinkMillis = agora;
    }
  }

  // 4) Servidor web (rapido, com timeout p/ nao travar o loop)
  WiFiClient client = server.available();
  if (client) {
    unsigned long inicio = millis();
    String currentLine = "";
    while (client.connected() && (millis() - inicio < 1000)) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            client.print("<!DOCTYPE html><html><head><meta charset=\"utf-8\">");
            client.print("<meta http-equiv=\"refresh\" content=\"1\"></head><body>");
            client.print("<h1>Monitor de Luminosidade</h1>");
            client.print("<p>Valor do LDR: <strong>");
            client.print(ldrVal);
            client.print("</strong></p>");
            client.print("<p>Estado: ");
            client.print(emergenciaAtiva ? "EMERGENCIA (SOS)" : "normal");
            client.print("</p></body></html>");
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
  }
}
