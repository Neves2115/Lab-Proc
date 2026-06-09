/*********
  Controle Combinado: LED e Servomotor - Versão Purificada (Sem Serial)
*********/

#include <WiFi.h>

const char* ssid     = "Controle Web Grupo G";
const char* password = "123456789";

WiFiServer server(80);
String header;

// Pinos de Saída
const int ledPin   = 7; 
const int servoPin = 6; 

// Resoluções de Hardware
const int ledResolution   = 8;  // 0 a 255
const int servoResolution = 10; // 0 a 1023

// Variáveis globais de estado
int dutyVal = 128;    
int servoAngle = 90;  

void setup() {
  // Inicialização direta do hardware PWM (Executado apenas uma vez)
  ledcAttach(ledPin, 1000, ledResolution);   // LED fixo em 1 kHz
  ledcAttach(servoPin, 50, servoResolution); // Servo fixo em 50 Hz

  // Aplica as posições/brilhos iniciais padrão
  ledcWrite(ledPin, dutyVal);
  int dutyServo = map(servoAngle, 0, 180, 26, 123);
  ledcWrite(servoPin, dutyServo);

  // Inicializa a rede sem fio (O IP padrão do ESP32 em modo AP é sempre 192.168.4.1)
  WiFi.softAP(ssid, password);
  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    String currentLine = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        header += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Envio do cabeçalho de resposta HTTP padrão
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // ============================================
            // PROCESSAMENTO DOS PARÂMETROS DA URL
            // ============================================
            if (header.indexOf("GET /?") >= 0) {
              int p_duty  = header.indexOf("duty=");
              int p_angle = header.indexOf("&angle=");
              int p_end   = header.indexOf(" HTTP");

              if (p_duty >= 0 && p_angle >= 0 && p_end >= 0) {
                int novoDuty  = header.substring(p_duty + 5, p_angle).toInt();
                int novoAngle = header.substring(p_angle + 7, p_end).toInt();

                // Filtros de salvaguarda de limites
                if(novoDuty >= 0 && novoDuty <= 255) dutyVal = novoDuty;
                if(novoAngle >= 0 && novoAngle <= 180) servoAngle = novoAngle;

                // Atualização direta do Duty Cycle nos drivers de silício
                ledcWrite(ledPin, dutyVal);
                
                int dutyServo = map(servoAngle, 0, 180, 26, 123);
                ledcWrite(servoPin, dutyServo);
              }
            }

            // INTERFACE GRÁFICA DO USUÁRIO (HTML / CSS)
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name='viewport' content='width=device-width, initial-scale=1'>");
            client.println("<meta charset='UTF-8'><title>Controle de Periféricos</title>");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".box { margin: 30px; padding: 20px; border: 2px solid #4CAF50; display: inline-block; border-radius: 10px; font-size: 20px; min-width: 320px;}");
            client.println("input[type=range] { width: 85%; margin: 15px 0; }");
            client.println("button { color: white; background-color: #4CAF50; border: none; cursor: pointer; font-weight: bold; font-size: 20px; margin: 20px; padding: 12px 24px; border-radius: 5px;}");
            client.println(".status-div { font-size: 18px; font-weight: bold; margin-top: 20px; color: #333; text-align: left; padding-left: 20px; }</style></head>");

            client.println("<body><h1>Painel de Controle PWM</h1>");
            client.println("<div class='box'>");

            // Slider LED
            client.println("<label>💡 Intensidade do LED (0 a 255): </label><br>");
            client.println("<input type='range' id='dutyInput' min='0' max='255' value='" + String(dutyVal) + "'><br><br>");
            
            // Slider Servo
            client.println("<label>⚙️ Posição do Servo (0° a 180°): </label><br>");
            client.println("<input type='range' id='angleInput' min='0' max='180' value='" + String(servoAngle) + "'><br>");

            client.println("<button onclick='enviarDados()'>Atualizar Periféricos</button>");

            // Feedback Visual na própria tela do dispositivo
            client.println("<div class='status-div'>");
            client.println("• LED: " + String(dutyVal) + " [Fixo: 1000 Hz]<br>");
            client.println("• Servo: " + String(servoAngle) + "° [Fixo: 50 Hz]");
            client.println("</div>");

            client.println("</div>");

            // Script de captura de dados e submissão
            client.println("<script>");
            client.println("function enviarDados() {");
            client.println("  const duty = document.getElementById('dutyInput').value;");
            client.println("  const angle = document.getElementById('angleInput').value;");
            client.println("  window.location.href = '/?duty=' + duty + '&angle=' + angle;");
            client.println("}");
            client.println("</script></body></html>");

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
