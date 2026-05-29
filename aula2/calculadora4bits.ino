/*********
  Calculadora Complemento de 2 (4 bits) - LEDs Persistentes e Botões Diretos
*********/

#include <WiFi.h>

const char* ssid     = "Calculadora ESP32 Grupo G";
const char* password = "123456789";

WiFiServer server(80);
String header;

// Definição dos pinos dos LEDs
const int pinBit3 = 7; // MSB (Sinal)
const int pinBit2 = 6;
const int pinBit1 = 5;
const int pinBit0 = 4; // LSB

// VARIÁVEIS GLOBAIS: Mantêm o estado dos LEDs na memória do ESP32 entre as requisições
String resultadoStr = "-";
bool temOverflow = false;
int resultadoInt = 0;

void atualizaLedsFisicos() {
  if (temOverflow) {
    // Apaga os LEDs em caso de Overflow
    digitalWrite(pinBit3, LOW);
    digitalWrite(pinBit2, LOW);
    digitalWrite(pinBit1, LOW);
    digitalWrite(pinBit0, LOW);
  } else {
    // Isola os 4 bits do complemento de 2 e atualiza os pinos
    byte bits = resultadoInt & 0x0F; 
    digitalWrite(pinBit3, (bits >> 3) & 0x01); 
    digitalWrite(pinBit2, (bits >> 2) & 0x01); 
    digitalWrite(pinBit1, (bits >> 1) & 0x01); 
    digitalWrite(pinBit0, (bits >> 0) & 0x01); 
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(pinBit3, OUTPUT);
  pinMode(pinBit2, OUTPUT);
  pinMode(pinBit1, OUTPUT);
  pinMode(pinBit0, OUTPUT);
  
  // Inicializa os LEDs com base no estado global inicial (tudo apagado)
  atualizaLedsFisicos();

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Endereço IP do AP: ");
  Serial.println(IP);
  
  server.begin();
}

void loop(){
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
            
            // PROCESSAMENTO NO ESP32
            if (header.indexOf("GET /?") >= 0) {
              int p1 = header.indexOf("v1=");
              int p2 = header.indexOf("&op=");
              int p3 = header.indexOf("&v2=");
              int p4 = header.indexOf(" HTTP");
              
              if (p1 >= 0 && p2 >= 0 && p3 >= 0 && p4 >= 0) {
                int v1 = header.substring(p1 + 3, p2).toInt();
                String op = header.substring(p2 + 4, p3);
                int v2 = header.substring(p3 + 4, p4).toInt();
                
                // Reseta variáveis globais antes do novo cálculo
                temOverflow = false; 
                
                if (op == "som") {
                  int soma = v1 + v2;
                  if (soma > 7 || soma < -8) {
                    temOverflow = true;
                  } else { 
                    resultadoStr = String(soma); 
                    resultadoInt = soma; 
                  }
                } 
                else if (op == "sub") {
                  int sub = v1 - v2;
                  if (sub > 7 || sub < -8) {
                    temOverflow = true;
                  } else { 
                    resultadoStr = String(sub); 
                    resultadoInt = sub; 
                  }
                }
                
                // Aplica a nova configuração de LEDs imediatamente
                atualizaLedsFisicos();
              }
            }
            
            // INTERFACE WEB (HTML / CSS)
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name='viewport' content='width=device-width, initial-scale=1'>");
            client.println("<meta charset='UTF-8'><title>Calculadora ESP32</title>");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".box { margin: 30px; padding: 20px; border: 2px solid #4CAF50; display: inline-block; border-radius: 10px; font-size: 20px;}");
            client.println("select, button { font-size: 22px; margin: 10px; padding: 8px; border-radius: 5px;}");
            client.println("button { color: white; border: none; cursor: pointer; font-weight: bold; min-width: 120px;}");
            client.println(".btn-soma { background-color: #4CAF50; }");
            client.println(".btn-sub { background-color: #2196F3; }");
            client.println(".overflow-div { color: red; font-size: 30px; font-weight: bold; margin-top: 20px; }</style></head>");
            
            client.println("<body><h1>Calculadora 4-bits (Complemento de 2)</h1>");
            client.println("<div class='box'>");
            
            // Seletores limpos apenas com os dois operandos
            client.println("<label>Op 1: </label><select id='op1'></select> &nbsp;&nbsp;");
            client.println("<label>Op 2: </label><select id='op2'></select><br><br>");
            
            // Substituição por dois botões dedicados
            client.println("<button class='btn-soma' onclick=\"enviarDados('som')\">SOMA (+)</button>");
            client.println("<button class='btn-sub' onclick=\"enviarDados('sub')\">SUB (-)</button>");
            
            if (temOverflow) {
              client.println("<div class='overflow-div'>Overflow!</div>");
            } else {
              client.println("<div style='font-size: 26px; font-weight: bold; margin-top:25px;'>Resultado: " + resultadoStr + "</div>");
            }
            client.println("</div>");
            
            // JAVASCRIPT
            client.println("<script>");
            client.println("const s1 = document.getElementById('op1'); const s2 = document.getElementById('op2');");
            
            // Popula os seletores de -8 até 7
            for(int i = -8; i <= 7; i++) {
              client.println("  s1.options[s1.options.length] = new Option(" + String(i) + "," + String(i) + ");");
              client.println("  s2.options[s2.options.length] = new Option(" + String(i) + "," + String(i) + ");");
            }
            
            // Mantém os operandos selecionados na tela resgatando os dados da URL
            client.println("const params = new URLSearchParams(window.location.search);");
            client.println("if(params.has('v1')) s1.value = params.get('v1');");
            client.println("if(params.has('v2')) s2.value = params.get('v2');");
            
            // Função modificada para aceitar a operação dinamicamente pelo botão clicado
            client.println("function enviarDados(operacao) {");
            client.println("  const v1 = s1.value; const v2 = s2.value;");
            client.println("  window.location.href = '/?v1=' + v1 + '&op=' + operacao + '&v2=' + v2;");
            client.println("}</script></body></html>");
            
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