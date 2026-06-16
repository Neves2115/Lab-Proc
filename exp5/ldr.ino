#include <WiFi.h>

const char* ssid     = "Controle Web Grupo G";
const char* password = "123456789";

WiFiServer server(80);
String header;

const int ldrPin     = 34; // Pino de leitura analógica
const int ledBuiltIn = 2;  // Pino do LED built-in

int ldrVal = 0;
const int limiarBaixaLuz = 1500; 

unsigned long prevLdrMillis   = 0;
unsigned long prevBlinkMillis = 0;
bool builtinLedState          = false;

void setup() {
  pinMode(ledBuiltIn, OUTPUT);
  analogSetAttenuation(ADC_11db);

  WiFi.softAP(ssid, password);
  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. Leitura do LDR (1 Hz)
  if (currentMillis - prevLdrMillis >= 1000) {
    prevLdrMillis = currentMillis;
    ldrVal = analogRead(ldrPin);
  }

  // 2. Lógica do LED (Piscar a cada 2 segundos quando luz < limiar)
  if (ldrVal < limiarBaixaLuz) {
    if (currentMillis - prevBlinkMillis >= 1000) {
      prevBlinkMillis = currentMillis;
      builtinLedState = !builtinLedState;
      digitalWrite(ledBuiltIn, builtinLedState);
    }
  } else {
    builtinLedState = false;
    digitalWrite(ledBuiltIn, LOW);
  }

  // 3. Servidor Web
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            client.println("<!DOCTYPE html><html><body>");
            client.println("<h1>Monitor de Luminosidade</h1>");
            client.println("<p>Valor do LDR: <strong>" + String(ldrVal) + "</strong></p>");
            client.println("<script>setTimeout(function(){ location.reload(); }, 1000);</script>");
            client.println("</body></html>");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
  }
}
