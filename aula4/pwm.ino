/*********
  Controle de Intensidade e Frequência do LED (PWM) via Webserver - CORRIGIDO
*********/

#include <WiFi.h>

const char* ssid     = "Controle PWM ESP32";
const char* password = "123456789";

WiFiServer server(80);
String header;

// Configurações do PWM (ESP32 Core 3.x)
const int ledPin = 7;       // Pino do LED Externo
const int resolution = 8;   // 8 bits (Valores de 0 a 255)

// Variáveis globais para manter o estado
int dutyVal = 128;          // Inicial em ~50%
int freqVal = 1000;         // Inicial em 1kHz

void setup() {
  Serial.begin(115200);

  // Inicializa o PWM direto no pino 7
  ledcAttach(ledPin, freqVal, resolution);
  ledcWrite(ledPin, dutyVal);

  // Inicializa o WiFi como Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Endereço IP do AP: ");
  Serial.println(IP);

  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("Novo Cliente.");
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

            // ============================================
            // PROCESSAMENTO DOS PARÂMETROS PWM NO ESP32
            // ============================================
            if (header.indexOf("GET /?") >= 0) {
              int p_duty = header.indexOf("duty=");
              int p_freq = header.indexOf("&freq=");
              int p_end  = header.indexOf(" HTTP");

              if (p_duty >= 0 && p_freq >= 0 && p_end >= 0) {
                int novoDuty = header.substring(p_duty + 5, p_freq).toInt();
                int novaFreq = header.substring(p_freq + 6, p_end).toInt();

                if(novoDuty >= 0 && novoDuty <= 255) dutyVal = novoDuty;
                if(novaFreq >= 100 && novaFreq <= 40000) freqVal = novaFreq;

                // Reconfigura o hardware com os novos valores recebidos
                ledcAttach(ledPin, freqVal, resolution);
                ledcWrite(ledPin, dutyVal);
                
                Serial.printf("PWM Atualizado -> Freq: %d Hz | Duty: %d\n", freqVal, dutyVal);
              }
            }

            // INTERFACE WEB (HTML / CSS)
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name='viewport' content='width=device-width, initial-scale=1'>");
            client.println("<meta charset='UTF-8'><title>Controle PWM ESP32</title>");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".box { margin: 30px; padding: 20px; border: 2px solid #4CAF50; display: inline-block; border-radius: 10px; font-size: 20px; min-width: 300px;}");
            client.println("input[type=number], input[type=range] { width: 80%; font-size: 18px; margin: 10px; padding: 5px; }");
            client.println("button { color: white; background-color: #4CAF50; border: none; cursor: pointer; font-weight: bold; font-size: 22px; margin: 20px; padding: 10px 20px; border-radius: 5px;}");
            client.println(".status-div { font-size: 22px; font-weight: bold; margin-top: 20px; color: #333; }</style></head>");

            client.println("<body><h1>Controle de LED Externo via PWM</h1>");
            client.println("<div class='box'>");

            // Sliders e Inputs configurados com os valores dinâmicos do ESP32
            client.println("<label>Intensidade (0 a 255): </label><br>");
            client.println("<input type='range' id='dutyInput' min='0' max='255' value='" + String(dutyVal) + "'><br><br>");
            
            client.println("<label>Frequência (Hz): </label><br>");
            client.println("<input type='number' id='freqInput' min='100' max='40000' value='" + String(freqVal) + "'><br>");

            client.println("<button onclick='enviarDados()'>Atualizar LED</button>");

            // Div de Status
            client.println("<div class='status-div'>");
            client.println("Frequência Atual: " + String(freqVal) + " Hz<br>");
            client.println("Duty Cycle Atual: " + String(dutyVal) + " (" + String((dutyVal * 100) / 255) + "%)");
            client.println("</div>");

            client.println("</div>");

            // JAVASCRIPT (CORRIGIDO)
            client.println("<script>");
            client.println("function enviarDados() {");
            client.println("  const duty = document.getElementById('dutyInput').value;");
            client.println("  const freq = document.getElementById('freqInput').value;");
            client.println("  window.location.href = '/?duty=' + duty + '&freq=' + freq;");
            client.println("}");
            client.println("</script>"); // <-- Corrigido aqui (estava <script>)
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
    Serial.println("Cliente desconectado.");
  }
}
