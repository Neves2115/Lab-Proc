#!/usr/bin/env python3
from gpiozero import PWMOutputDevice
from time import sleep
import warnings
warnings.filterwarnings("ignore")

BUZZER_PIN = 22
buzzer = PWMOutputDevice(BUZZER_PIN)

def beep_sucesso():
    print("Sinal: SUCESSO (2 bipes curtos)")
    for _ in range(2):
        buzzer.value = 0.5  # Duty cycle 50%
        sleep(0.1)
        buzzer.value = 0
        sleep(0.1)

def beep_erro():
    print("Sinal: ERRO (1 bipe longo)")
    buzzer.value = 0.5
    sleep(0.8)
    buzzer.value = 0

print("Testando Buzzer de Feedback... (Ctrl+C para sair)")
try:
    while True:
        beep_sucesso()
        sleep(1.5)
        beep_erro()
        sleep(2.0)
except KeyboardInterrupt:
    print("\nEncerrando teste do buzzer.")
finally:
    buzzer.close()