/* desafio_calculadora_freenove.c
 * Calculadora binaria STANDALONE no Raspberry Pi (kit Freenove FNK0054):
 *   - Entrada: teclado matricial 4x4 (Cap. 21), varredura por GPIO (WiringPi)
 *   - Saida:   LCD1602 via I2C com expansor PCF8574 (Cap. 19)
 * Opera sem PC: apenas com o teclado e o display.
 *
 * Depende da WiringPi (instalada conforme o Cap. 0 do manual Freenove).
 * Compilar:  gcc desafio_calculadora_freenove.c -o calc -lwiringPi -lwiringPiDev
 * Executar:  sudo ./calc
 *
 * Referencias Freenove (FNK0054):
 *   Cap. 19 LCD1602       -> .../19_LCD1602.html
 *   Cap. 21 Matrix Keypad -> .../21_Matrix_Keypad.html
 */
#include <wiringPi.h>
#include <pcf8574.h>
#include <lcd.h>
#include <stdio.h>

/* ---------------- LCD1602 via PCF8574 (padrao do Cap. 19) ---------------- */
#define PCF_ADDR 0x27        /* endereco I2C do modulo (use 0x3F se necessario) */
#define BASE     64          /* base dos pinos virtuais do PCF8574 */
#define RS  (BASE+0)
#define RW  (BASE+1)
#define EN  (BASE+2)
#define LED (BASE+3)         /* backlight */
#define D4  (BASE+4)
#define D5  (BASE+5)
#define D6  (BASE+6)
#define D7  (BASE+7)
int lcdhd;

/* ---------- Teclado matricial 4x4 (varredura por GPIO, Cap. 21) ----------
   Pinos em numeracao BCM (wiringPiSetupGpio). Ajuste a sua ligacao real. */
const int linhas[4]  = {18, 23, 24, 25};   /* saidas */
const int colunas[4] = {12, 16, 20, 21};   /* entradas com pull-up */
const char MAPA[4][4] = {
  {'1','2','3','A'},   /* A = soma (+)        */
  {'4','5','6','B'},   /* B = subtracao (-)   */
  {'7','8','9','C'},   /* C = multiplicacao(*)*/
  {'*','0','#','D'}    /* D = divisao (/), # = igual, * = limpar */
};

void keypadInit(void){
  for(int i=0;i<4;i++){ pinMode(linhas[i], OUTPUT); digitalWrite(linhas[i], HIGH); }
  for(int j=0;j<4;j++){ pinMode(colunas[j], INPUT); pullUpDnControl(colunas[j], PUD_UP); }
}

/* Retorna a tecla pressionada (0 se nenhuma). Varre linha a linha. */
char keypadGetKey(void){
  char tecla = 0;
  for(int i=0;i<4;i++){
    digitalWrite(linhas[i], LOW);                       /* ativa a linha i */
    for(int j=0;j<4;j++){
      if(digitalRead(colunas[j]) == LOW){               /* tecla pressionada */
        while(digitalRead(colunas[j]) == LOW) delay(5); /* espera soltar (debounce) */
        tecla = MAPA[i][j];
      }
    }
    digitalWrite(linhas[i], HIGH);
  }
  return tecla;
}

/* ---------------- Helpers do LCD ---------------- */
void lcdSetup(void){
  pcf8574Setup(BASE, PCF_ADDR);
  for(int i=0;i<8;i++) pinMode(BASE+i, OUTPUT);
  digitalWrite(LED, HIGH);     /* liga o backlight */
  digitalWrite(RW, LOW);       /* modo escrita */
  lcdhd = lcdInit(2, 16, 4, RS, EN, D4, D5, D6, D7, 0, 0, 0, 0);
}
void mostra(const char* l1, const char* l2){
  lcdClear(lcdhd);
  lcdPosition(lcdhd, 0, 0); lcdPuts(lcdhd, l1);
  lcdPosition(lcdhd, 0, 1); lcdPuts(lcdhd, l2);
}

int main(void){
  if(wiringPiSetupGpio() == -1){ printf("Erro ao iniciar a WiringPi\n"); return 1; }
  keypadInit();
  lcdSetup();

  long A = 0, B = 0;
  char op = 0;
  int lendoB = 0;
  char buf[17];

  mostra("Calc 4 bits", "Digite A");

  for(;;){
    char k = keypadGetKey();
    if(!k){ delay(20); continue; }

    if(k == '*'){                                  /* limpar */
      A = 0; B = 0; op = 0; lendoB = 0;
      mostra("Calc 4 bits", "Digite A");
      continue;
    }

    if(k >= '0' && k <= '9'){                       /* digito */
      int d = k - '0';
      if(!lendoB){ A = A*10 + d; sprintf(buf, "%ld", A); mostra("Operando A:", buf); }
      else       { B = B*10 + d; sprintf(buf, "%ld", B); mostra("Operando B:", buf); }
      continue;
    }

    if(k=='A' || k=='B' || k=='C' || k=='D'){       /* operacao */
      op = k; lendoB = 1;
      mostra("Operando B:", "0");
      continue;
    }

    if(k == '#'){                                   /* igual */
      if(op == 'D' && B == 0){                      /* guarda de divisao por zero */
        mostra("ERRO", "Divisao por zero");
        continue;
      }
      long r = 0;
      switch(op){
        case 'A': r = A + B; break;
        case 'B': r = A - B; break;
        case 'C': r = A * B; break;
        case 'D': r = A / B; break;
        default:  mostra("Pressione", "uma operacao"); continue;
      }
      sprintf(buf, "%ld", r);
      mostra("Resultado:", buf);
    }
  }
  return 0;
}
