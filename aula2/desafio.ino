#include <Arduino.h>

const int LEDS[4] = {4, 5, 6, 7};

String getDecimalFrom1sComp(uint8_t binValue) {
  binValue &= 0x0F;
  if (binValue == 0b1111) return "-0";
  if (binValue & 0b1000) {
    int val = -((~binValue) & 0b0111);
    return String(val);
  }
  return String(binValue);
}

uint8_t decTo1sComp(int dec) {
  if (dec >= 0) {
    return dec & 0x0F;
  } else {
    return (~abs(dec)) & 0x0F;
  }
}

String binaryToString4bit(uint8_t value) {
  String result = "";
  for (int i = 3; i >= 0; i--) {
    result += ((value >> i) & 1) ? '1' : '0';
  }
  return result;
}

void showResultOnLEDs(uint8_t value) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(LEDS[i], (value >> i) & 1);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  for (int i = 0; i < 4; i++) {
    pinMode(LEDS[i], OUTPUT);
    digitalWrite(LEDS[i], LOW);
  }

  Serial.println("\n=== ALU 4-bit Complemento de Um ===");
  Serial.println("Digite a conta na mesma linha (Ex: 1 + 1 ou 3 - 2)");
  Serial.println("Limites permitidos: -7 a +7");
  Serial.println("-----------------------------------");
}

void loop() {
  while (Serial.available() == 0) { delay(10); }
  
  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.length() == 0) return;

  int valA, valB;
  char op;

  if (sscanf(input.c_str(), "%d %c %d", &valA, &op, &valB) == 3) {
    
    // Verificação de limites
    if (valA < -7 || valA > 7 || valB < -7 || valB > 7) {
      Serial.println("Erro: Os valores devem estar entre -7 e +7.");
      return;
    }
    if (op != '+' && op != '-') {
      Serial.println("Erro: Operacao deve ser '+' ou '-'.");
      return;
    }

    // Converte os números decimais informados para Complemento de 1 (uint8_t)
    uint8_t opA = decTo1sComp(valA);
    uint8_t opB_original = decTo1sComp(valB);
    
    uint8_t opB_internal = opB_original;
    
    // Se for subtração, inverte os bits do segundo operando
    if (op == '-') {
      opB_internal = (~opB_original) & 0x0F;
    }

    uint16_t resultado = opA + opB_internal;

    // Lógica correta de End-around carry (se passar de 15, soma 1 ao LSB)
    if (resultado > 15) {
      resultado = (resultado & 0x0F) + 1;
    }

    uint8_t res = resultado & 0x0F;

    // Verificação Lógica de Overflow (Sinais iguais na entrada, mas sinal diferente na saída)
    bool msbA = (opA >> 3) & 1;
    bool msbB_int = (opB_internal >> 3) & 1;
    bool msbRes = (res >> 3) & 1;
    bool overflow = (msbA == msbB_int) && (msbA != msbRes);

    Serial.println("-------------------------");
    Serial.print("Entrada   = "); Serial.println(input);

    if (overflow) {
      Serial.println(">> ERRO: OVERFLOW DETECTADO!");
      showResultOnLEDs(0); // Apaga os LEDs em caso de erro
    } else {
      // Seta a saída física
      showResultOnLEDs(res);
      
      Serial.print("A         = "); Serial.print(binaryToString4bit(opA));
      Serial.print(" ("); Serial.print(getDecimalFrom1sComp(opA)); Serial.println(")");
      
      Serial.print("B         = "); Serial.print(binaryToString4bit(opB_original));
      Serial.print(" ("); Serial.print(getDecimalFrom1sComp(opB_original)); Serial.println(")");
      
      Serial.print("Operacao  = "); Serial.println(op);
      
      Serial.print("Resultado = "); Serial.print(binaryToString4bit(res));
      Serial.print(" ("); Serial.print(getDecimalFrom1sComp(res)); Serial.println(")");
    }
    Serial.println("-------------------------\n");

  } else {
    Serial.println("Formato invalido!");
  }
}