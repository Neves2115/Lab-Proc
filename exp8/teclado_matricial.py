#!/usr/bin/env python3
import time
from RPi import GPIO

# Definicao dos pinos GPIO (BCM)
ROWS = [18, 23, 24, 25]  # Pinos de Saida (Linhas)
COLS = [12, 16, 20, 21]  # Pinos de Entrada com Pull-Down (Colunas)

KEYPAD = [
    ['1', '2', '3', 'A'],
    ['4', '5', '6', 'B'],
    ['7', '8', '9', 'C'],
    ['*', '0', '#', 'D']
]

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

for row_pin in ROWS:
    GPIO.setup(row_pin, GPIO.OUT)
    GPIO.output(row_pin, GPIO.LOW)

for col_pin in COLS:
    GPIO.setup(col_pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

def read_keypad():
    for r_idx, row_pin in enumerate(ROWS):
        GPIO.output(row_pin, GPIO.HIGH)
        for c_idx, col_pin in enumerate(COLS):
            if GPIO.input(col_pin) == GPIO.HIGH:
                time.sleep(0.04)  # Debounce
                if GPIO.input(col_pin) == GPIO.HIGH:
                    while GPIO.input(col_pin) == GPIO.HIGH:
                        time.sleep(0.01)  # Aguarda soltar a tecla
                    GPIO.output(row_pin, GPIO.LOW)
                    return KEYPAD[r_idx][c_idx]
        GPIO.output(row_pin, GPIO.LOW)
    return None

print("Monitorando Teclado Matricial... (Ctrl+C para sair)")
try:
    while True:
        key = read_keypad()
        if key:
            print(f"Tecla Pressionada: {key}")
        time.sleep(0.02)
except KeyboardInterrupt:
    print("\nEncerrando teste do teclado.")
finally:
    GPIO.cleanup()