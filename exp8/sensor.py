#!/usr/bin/env python3
from gpiozero import Button
from time import sleep
import warnings
warnings.filterwarnings("ignore")

# Pino GPIO BCM conectado ao sensor magnetico
SENSOR_PIN = 17

sensor = Button(SENSOR_PIN, pull_up=True)

print("Monitorando Sensor Magnetico (Fim de Curso)... (Ctrl+C para sair)")
try:
    while True:
        estado = "TRANCADA / FECHADA" if sensor.is_pressed else "ABERTA / VIOLADA"
        print(f"Estado da Tranca: {estado}")
        sleep(0.5)
except KeyboardInterrupt:
    print("\nEncerrando teste do sensor magnetico.")
finally:
    sensor.close()