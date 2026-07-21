#!/usr/bin/env python3
"""
===================================================================
PCS3732 - Laboratório de Processadores (Poli-USP)
Aula 10: O Desafio da Fechadura Eletrônica
===================================================================
Aplicação Integrada de Fechadura Eletrônica para Raspberry Pi 3
Utilizando Sensor Ultrassônico (HC-SR04) para verificação de tranca.
"""

import time
import warnings
import smbus2
from RPi import GPIO
from gpiozero import DistanceSensor

# Silencia avisos do GPIO ao reiniciar scripts
warnings.filterwarnings("ignore")

# =================================================================
# 1. MAPEAMENTO DE PINOS (BCM) E PARÂMETROS DO SISTEMA
# =================================================================
# Teclado Matricial 4x4
KEYPAD_ROWS = [18, 23, 24, 25]  # Linhas do Teclado (Saídas)
KEYPAD_COLS = [12, 16, 20, 21]  # Colunas do Teclado (Entradas com Pull-Down)

# Sensor Ultrassônico (HC-SR04)
TRIG_PIN   = 14                 # Pino Trigger
ECHO_PIN   = 15                 # Pino Echo
LIMIAR_CM  = 5.0                # Distância (cm) abaixo da qual a tranca é considerada FECHADA

# Periféricos de Saída
BUZZER_PIN = 22                 # Buzzer de Alerta/Feedback

# Matriz do Teclado 4x4
KEYPAD = [
    ['1', '2', '3', 'A'],
    ['4', '5', '6', 'B'],
    ['7', '8', '9', 'C'],
    ['*', '0', '#', 'D']
]

# Configurações do Display LCD I2C (PCF8574)
I2C_ADDR = 0x27
I2C_BUS_NUM = 1

# Parâmetros de Lógica de Negócio
PASSWORD_CORRECT = "1234"       # Senha mestra cadastrada
MAX_ATTEMPTS = 3                # Tentativas máximas antes do bloqueio (RNF1)
COOLDOWN_TIME_SEC = 15          # Tempo de bloqueio temporário em segundos
UNLOCK_TIME_SEC = 5             # Tempo em que a porta permanece liberada após sucesso


# =================================================================
# 2. DRIVER DO DISPLAY LCD 16x2 VIA I2C
# =================================================================
class LCDI2C:
    ENABLE = 0b00000100
    BACKLIGHT = 0b00001000
    CMD = 0
    DATA = 1

    def __init__(self, addr=I2C_ADDR, bus_num=I2C_BUS_NUM):
        self.addr = addr
        self.bus = smbus2.SMBus(bus_num)
        self.init_lcd()

    def _strobe(self, data):
        self.bus.write_byte(self.addr, data | self.ENABLE | self.BACKLIGHT)
        time.sleep(0.0005)
        self.bus.write_byte(self.addr, (data & ~self.ENABLE) | self.BACKLIGHT)
        time.sleep(0.0001)

    def _write_nibble(self, data):
        self.bus.write_byte(self.addr, data | self.BACKLIGHT)
        self._strobe(data)

    def send(self, data, mode=0):
        high = mode | (data & 0xF0)
        low = mode | ((data << 4) & 0xF0)
        self._write_nibble(high)
        self._write_nibble(low)

    def init_lcd(self):
        self.send(0x33, self.CMD)
        self.send(0x32, self.CMD)
        self.send(0x06, self.CMD)  # Auto-incremento do cursor
        self.send(0x0C, self.CMD)  # Display ligado, sem cursor
        self.send(0x28, self.CMD)  # 2 linhas, matriz 5x8
        self.clear()

    def clear(self):
        self.send(0x01, self.CMD)
        time.sleep(0.003)

    def write_line(self, text, line=1):
        """Escreve uma linha de texto ajustada exatamente para 16 caracteres."""
        addr = 0x80 if line == 1 else 0xC0
        self.send(addr, self.CMD)
        formatted_text = text[:16].ljust(16)
        for char in formatted_text:
            self.send(ord(char), self.DATA)

    def close(self):
        self.clear()
        self.bus.close()


# =================================================================
# 3. CONTROLADOR INTEGRADO DA FECHADURA (COM ULTRASSÔNICO)
# =================================================================
class FechaduraEletronica:
    def __init__(self):
        self._setup_gpio()
        self.sensor_ultrassonico = DistanceSensor(
            echo=ECHO_PIN,
            trigger=TRIG_PIN,
            max_distance=3.0
        )
        self.lcd = LCDI2C()
        
        self.buffer_senha = ""
        self.falhas_consecutivas = 0
        self.sistema_autorizado = False

    def _setup_gpio(self):
        GPIO.setmode(GPIO.BCM)
        GPIO.setwarnings(False)

        # Configuração do Teclado Matricial
        for r in KEYPAD_ROWS:
            GPIO.setup(r, GPIO.OUT)
            GPIO.output(r, GPIO.LOW)

        for c in KEYPAD_COLS:
            GPIO.setup(c, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

        # Configuração do Buzzer
        GPIO.setup(BUZZER_PIN, GPIO.OUT)
        GPIO.output(BUZZER_PIN, GPIO.LOW)

    def _get_distancia_cm(self):
        """Retorna a distância medida pelo sensor ultrassônico em centímetros."""
        return self.sensor_ultrassonico.distance * 100.0

    def _is_porta_fechada(self):
        """
        Retorna True se a distância medida for menor que o limiar (LIMIAR_CM),
        indicando que o batente/trava está na posição fechada.
        """
        return self._get_distancia_cm() < LIMIAR_CM

    def beep(self, duracao, repeticoes=1, intervalo=0.1):
        """Emite sinal sonoro de feedback no buzzer."""
        for _ in range(repeticoes):
            GPIO.output(BUZZER_PIN, GPIO.HIGH)
            time.sleep(duracao)
            GPIO.output(BUZZER_PIN, GPIO.LOW)
            time.sleep(intervalo)

    def ler_tecla(self):
        """Varredura do teclado matricial com tratamento de debounce (RF1)."""
        for r_idx, r_pin in enumerate(KEYPAD_ROWS):
            GPIO.output(r_pin, GPIO.HIGH)
            for c_idx, c_pin in enumerate(KEYPAD_COLS):
                if GPIO.input(c_pin) == GPIO.HIGH:
                    time.sleep(0.04)  # Debounce anti-repique
                    if GPIO.input(c_pin) == GPIO.HIGH:
                        # Aguarda a liberação da tecla
                        while GPIO.input(c_pin) == GPIO.HIGH:
                            time.sleep(0.01)
                        GPIO.output(r_pin, GPIO.LOW)
                        return KEYPAD[r_idx][c_idx]
            GPIO.output(r_pin, GPIO.LOW)
        return None

    def executar_cooldown(self):
        """Gere o tempo de bloqueio por tentativas falhas repetidas (RNF1)."""
        self.lcd.write_line("BLOQUEADO (3/3)", line=1)
        self.beep(0.5)
        for t in range(COOLDOWN_TIME_SEC, 0, -1):
            self.lcd.write_line(f"Aguarde {t:2d}s...", line=2)
            time.sleep(1)
            
        self.falhas_consecutivas = 0
        self.buffer_senha = ""
        self.reset_display_home()

    def disparar_alarme_violacao(self):
        """Dispara alarme visual e sonoro para abertura forçada (RF3)."""
        self.lcd.write_line("! ALERTA CRITICO !", line=1)
        self.lcd.write_line("PORTA VIOLADA!", line=2)
        dist_atual = self._get_distancia_cm()
        print(f"\n[ALERTA RF3] Violação física detectada! Distância: {dist_atual:.1f} cm (>= {LIMIAR_CM} cm)")
        
        # Alarme intermitente até que a porta volte à posição fechada (< LIMIAR_CM)
        while not self._is_porta_fechada():
            GPIO.output(BUZZER_PIN, GPIO.HIGH)
            time.sleep(0.15)
            GPIO.output(BUZZER_PIN, GPIO.LOW)
            time.sleep(0.15)
            
        print("[ALERTA] Integridade física da porta restabelecida.")
        self.buffer_senha = ""
        self.reset_display_home()

    def processar_senha(self):
        """Valida a senha digitada pelo usuário."""
        if self.buffer_senha == PASSWORD_CORRECT:
            self.lcd.write_line("ACESSO CONCEDIDO", line=1)
            self.lcd.write_line("PORTA DESTRAVADA", line=2)
            print("[INFO] Senha correta! Destravando fechadura...")
            self.beep(0.1, repeticoes=2)  # Bipe curto duplo (sucesso)
            self.sistema_autorizado = True
            
            # Janela de tempo em que a abertura é permitida sem disparar alarme
            inicio = time.time()
            while time.time() - inicio < UNLOCK_TIME_SEC:
                dist = self._get_distancia_cm()
                status = "FECHADA" if dist < LIMIAR_CM else "ABERTA"
                self.lcd.write_line(f"Status: {status}", line=2)
                time.sleep(0.2)

            self.sistema_autorizado = False
            self.falhas_consecutivas = 0
            self.buffer_senha = ""
            self.reset_display_home()
        else:
            self.falhas_consecutivas += 1
            self.lcd.write_line("ACESSO NEGADO!", line=1)
            self.lcd.write_line(f"Falhas: {self.falhas_consecutivas}/{MAX_ATTEMPTS}", line=2)
            print(f"[ERRO] Senha incorreta ({self.falhas_consecutivas}/{MAX_ATTEMPTS})")
            self.beep(0.8)  # Bipe longo (erro)
            time.sleep(1.5)

            if self.falhas_consecutivas >= MAX_ATTEMPTS:
                self.executar_cooldown()
            else:
                self.buffer_senha = ""
                self.reset_display_home()

    def reset_display_home(self):
        """Restaura a tela inicial do sistema."""
        self.lcd.write_line("DIGITE A SENHA:", line=1)
        self.lcd.write_line("", line=2)

    def loop_principal(self):
        """Loop de controle não-bloqueante."""
        print("=== Sistema de Fechadura Eletrônica (Sensor Ultrassônico) ===")
        print(f"Pinos: TRIG=GPIO {TRIG_PIN}, ECHO=GPIO {ECHO_PIN} | Limiar de Fechamento: {LIMIAR_CM} cm")
        print("Pressione 'Ctrl+C' no terminal para interromper.")
        self.reset_display_home()

        try:
            while True:
                # 1. Verificação contínua de Violação Física via Ultrassom (RF3)
                if not self._is_porta_fechada() and not self.sistema_autorizado:
                    self.disparar_alarme_violacao()

                # 2. Polling do Teclado e Atualização da Interface (RF1 & RF2)
                tecla = self.ler_tecla()
                if tecla:
                    if tecla == '#':  # Submeter Senha
                        if len(self.buffer_senha) > 0:
                            self.processar_senha()
                    elif tecla == '*':  # Backspace
                        if len(self.buffer_senha) > 0:
                            self.buffer_senha = self.buffer_senha[:-1]
                            mascara = "*" * len(self.buffer_senha)
                            self.lcd.write_line(mascara, line=2)
                    elif len(self.buffer_senha) < 6:  # Limite máximo de 6 dígitos
                        self.buffer_senha += tecla
                        mascara = "*" * len(self.buffer_senha)
                        self.lcd.write_line(mascara, line=2)

                time.sleep(0.02)  # Ciclo de varredura curto (<200ms)

        except KeyboardInterrupt:
            print("\n[INFO] Desconectando e limpando periféricos...")
        finally:
            self.sensor_ultrassonico.close()
            self.lcd.close()
            GPIO.cleanup()


if __name__ == "__main__":
    fechadura = FechaduraEletronica()
    fechadura.loop_principal()
