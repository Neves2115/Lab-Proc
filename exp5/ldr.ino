#include <WiFi.h>

// ====== Rede (modo Access Point) ======
const char* ssid     = "Controle Web Grupo G";
const char* password = "123456789";

WiFiServer server(80);

// ====== Pinos (ESP32-C3 Super Mini) ======
// ATENCAO: no ESP32-C3 NAO existe GPIO 34. Os canais do ADC1 ficam nos
// GPIO 0 a 4. Ajuste 'ldrPin' para o pino em que o LDR esta REALMENTE ligado.
const int ldrPin    = 0;  // GPIO3 = ADC1 -> entrada analogica do LDR
const int rgbLedPin = 8;  // GPIO8 = LED RGB embarcado (WS2812) do Super Mini

// ====== Parametros ======
int ldrVal = 0;
const int limiarBaixaLuz = 1500;  // calibre conforme suas medicoes no Serial Monitor
const int brilhoLed      = 40;    // 0-255 (o WS2812 e bem brilhante)

// ====== Controle de tempo (nao bloqueante) ======
unsigned long prevLdrMillis   = 0;
unsigned long prevBlinkMillis = 0;
bool ledLigado = false;

// Amarelo = vermelho + verde; apagar = tudo zero
void setAmarelo(bool ligar) {
  if (ligar) {
    neopixelWrite(rgbLedPin, brilhoLed, brilhoLed, 0);
  } else {
    neopixelWrite(rgbLedPin, 0, 0, 0);
  }
}

void setup() {
  Serial.begin(115200);

  analogSetAttenuation(ADC_11db);  // faixa de leitura ~0 a 3,3 V
  setAmarelo(false);

  WiFi.softAP(ssid, password);
  Serial.print("Access Point no IP: ");
  Serial.println(WiFi.softAPIP());
  server.begin();
}

void loop() {
  unsigned long agora = millis();

  // 1) Leitura do LDR a 1 Hz
  if (agora - prevLdrMillis >= 1000) {
    prevLdrMillis = agora;
    ldrVal = analogRead(ldrPin);
    Serial.println(ldrVal);  // util para calibrar o limiar
  }

  // 2) Pisca amarelo (ciclo de 2 s) somente em baixa luminosidade
  if (ldrVal > limiarBaixaLuz) {
    if (agora - prevBlinkMillis >= 1000) {  // alterna a cada 1 s -> periodo de 2 s
      prevBlinkMillis = agora;
      ledLigado = !ledLigado;
      setAmarelo(ledLigado);
    }
  } else {
    if (ledLigado) {            // ao voltar a luz, garante LED apagado
      ledLigado = false;
      setAmarelo(false);
    }
    prevBlinkMillis = agora;    // zera a fase para o proximo ciclo de baixa luz
  }

  // 3) Servidor web (atendimento rapido, com timeout p/ nao travar o loop)
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
            client.print("</strong></p></body></html>");
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
