#!/usr/bin/env python3
"""
===================================================================
PCS3732 - Laboratório de Processadores (Poli-USP)
Fechadura Eletrônica Integrada Completa
Periféricos: Teclado 4x4, LCD I2C, Sensor Ultrassônico, Servo e Buzzer
===================================================================
"""

import time
import warnings
import smbus2
from RPi import GPIO
from gpiozero import DistanceSensor, Servo

# Silencia avisos do GPIO ao reiniciar scripts
warnings.filterwarnings("ignore")

# =================================================================
# 1. MAPEAMENTO DE PINOS (BCM) E PARÂMETROS DO SISTEMA
# =================================================================
# Servomotor (PWM)
SERVO_PIN  = 18                 # Pino PWM para o Servomotor

# Teclado Matricial 4x4 (GPIO 18 alterado para GPIO 17)
KEYPAD_ROWS = [16, 20, 21, 26]  # Linhas do Teclado (Saídas)
KEYPAD_COLS = [19, 13, 6, 5]  # Colunas do Teclado (Entradas com Pull-Down)

# Sensor Ultrassônico (HC-SR04)
TRIG_PIN   = 14                 # Pino Trigger
ECHO_PIN   = 15                 # Pino Echo
LIMIAR_CM  = 5.0                # Distância (cm) abaixo da qual a porta é considerada FECHADA

# Periféricos de Saída
BUZZER_PIN = 12               # Buzzer de Alerta/Feedback

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
# 3. CONTROLADOR INTEGRADO DA FECHADURA COM SERVOMOTOR
# =================================================================
class FechaduraEletronica:
    def __init__(self):
        self._setup_gpio()
        self.sensor_ultrassonico = DistanceSensor(
            echo=ECHO_PIN,
            trigger=TRIG_PIN,
            max_distance=3.0
        )
        self.servo = Servo(SERVO_PIN)
        self.lcd = LCDI2C()
        
        self.buffer_senha = ""
        self.falhas_consecutivas = 0
        self.sistema_autorizado = False

        # Inicia a fechadura no estado TRANCADO
        self.travar_mecanismo()

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

    def travar_mecanismo(self):
        """Move o servo para a posição de tranca física (0°)."""
        print("[MECANISMO] Posicionando servomotor: TRANCADO")
        self.servo.min()
        time.sleep(0.4)

    def destravar_mecanismo(self):
        """Move o servo para a posição de liberação física (90°/180°)."""
        print("[MECANISMO] Posicionando servomotor: DESTRANCADO")
        self.servo.max()
        time.sleep(0.4)

    def _get_distancia_cm(self):
        """Retorna a distância medida pelo sensor ultrassônico em centímetros."""
        return self.sensor_ultrassonico.distance * 100.0

    def _is_porta_fechada(self):
        """Retorna True se a distância medida for menor que LIMIAR_CM."""
        return self._get_distancia_cm() < LIMIAR_CM

    def beep(self, duracao, repeticoes=1, intervalo=0.1):
        """Emite sinal sonoro de feedback no buzzer."""
        for _ in range(repeticoes):
            GPIO.output(BUZZER_PIN, GPIO.HIGH)
            time.sleep(duracao)
            GPIO.output(BUZZER_PIN, GPIO.LOW)
            time.sleep(intervalo)

    def ler_tecla(self):
        """Varredura do teclado matricial com debounce."""
        for r_idx, r_pin in enumerate(KEYPAD_ROWS):
            GPIO.output(r_pin, GPIO.HIGH)
            for c_idx, c_pin in enumerate(KEYPAD_COLS):
                if GPIO.input(c_pin) == GPIO.HIGH:
                    time.sleep(0.04)  # Debounce
                    if GPIO.input(c_pin) == GPIO.HIGH:
                        while GPIO.input(c_pin) == GPIO.HIGH:
                            time.sleep(0.01)
                        GPIO.output(r_pin, GPIO.LOW)
                        return KEYPAD[r_idx][c_idx]
            GPIO.output(r_pin, GPIO.LOW)
        return None

    def executar_cooldown(self):
        """Bloqueio temporário por falhas consecutivas (RNF1)."""
        self.lcd.write_line("BLOQUEADO (3/3)", line=1)
        self.beep(0.5)
        for t in range(COOLDOWN_TIME_SEC, 0, -1):
            self.lcd.write_line(f"Aguarde {t:2d}s...", line=2)
            time.sleep(1)
            
        self.falhas_consecutivas = 0
        self.buffer_senha = ""
        self.reset_display_home()

    def disparar_alarme_violacao(self):
        """Dispara alarme sonoro e visual em caso de abertura forçada (RF3)."""
        self.lcd.write_line("! ALERTA CRITICO !", line=1)
        self.lcd.write_line("PORTA VIOLADA!", line=2)
        dist_atual = self._get_distancia_cm()
        print(f"\n[ALERTA RF3] Violação física detectada! Distância: {dist_atual:.1f} cm (>= {LIMIAR_CM} cm)")
        
        while not self._is_porta_fechada():
            GPIO.output(BUZZER_PIN, GPIO.HIGH)
            time.sleep(0.15)
            GPIO.output(BUZZER_PIN, GPIO.LOW)
            time.sleep(0.15)
            
        print("[ALERTA] Integridade física da porta restabelecida.")
        self.buffer_senha = ""
        self.reset_display_home()

    def processar_senha(self):
        """Valida a senha, acciona o servo e mantém destravado até o fim da execução."""
        if self.buffer_senha == PASSWORD_CORRECT:
            self.lcd.write_line("ACESSO CONCEDIDO", line=1)
            print("[INFO] Senha correta! Acionando Servomotor...")
            self.beep(0.1, repeticoes=2)
            
            # Aciona o servo mecânico para destravar a porta
            self.destravar_mecanismo()
            self.sistema_autorizado = True
            
            # Loop Permanente: Mantém o servo destravado e atualiza o status no LCD
            while True:
                dist = self._get_distancia_cm()
                status = "FECHADA" if dist < LIMIAR_CM else "ABERTA"
                self.lcd.write_line("ACESSO CONCEDIDO", line=1)
                self.lcd.write_line(f"Status: {status}", line=2)
                time.sleep(0.2)
        else:
            self.falhas_consecutivas += 1
            self.lcd.write_line("ACESSO NEGADO!", line=1)
            self.lcd.write_line(f"Falhas: {self.falhas_consecutivas}/{MAX_ATTEMPTS}", line=2)
            print(f"[ERRO] Senha incorreta ({self.falhas_consecutivas}/{MAX_ATTEMPTS})")
            self.beep(0.8)
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
        """Loop principal de controle."""
        print("=== Fechadura Eletrônica Integrada (com Servomotor no GPIO 18) ===")
        print(f"Pinos: SERVO=GPIO {SERVO_PIN}, TRIG=GPIO {TRIG_PIN}, ECHO=GPIO {ECHO_PIN}")
        print("Pressione 'Ctrl+C' no terminal para encerrar.")
        self.reset_display_home()

        try:
            while True:
                # 1. Verificação contínua de Violação Física (RF3)
                if not self._is_porta_fechada() and not self.sistema_autorizado:
                    self.disparar_alarme_violacao()

                # 2. Polling do Teclado (RF1 & RF2)
                tecla = self.ler_tecla()
                if tecla:
                    if tecla == '#':
                        if len(self.buffer_senha) > 0:
                            self.processar_senha()
                    elif tecla == '*':
                        if len(self.buffer_senha) > 0:
                            self.buffer_senha = self.buffer_senha[:-1]
                            mascara = "*" * len(self.buffer_senha)
                            self.lcd.write_line(mascara, line=2)
                    elif len(self.buffer_senha) < 6:
                        self.buffer_senha += tecla
                        mascara = "*" * len(self.buffer_senha)
                        self.lcd.write_line(mascara, line=2)

                time.sleep(0.02)

        except KeyboardInterrupt:
            print("\n[INFO] Desconectando e retornando servo ao estado inicial...")
        finally:
            self.travar_mecanismo()
            self.sensor_ultrassonico.close()
            self.servo.close()
            self.lcd.close()
            GPIO.cleanup()


if __name__ == "__main__":
    fechadura = FechaduraEletronica()
    fechadura.loop_principal()
