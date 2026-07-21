#!/usr/bin/env python3
"""
===================================================================
PCS3732 - Laboratório de Processadores (Poli-USP)
Teste Isolado: Sensor Ultrassônico de Distância (HC-SR04)
===================================================================
"""

from gpiozero import DistanceSensor
from time import sleep
import warnings

# Ignora avisos de pinos já em uso ao reiniciar o script
warnings.filterwarnings("ignore")

# 1. MAPEAMENTO DE PINOS GPIO (BCM) E CONFIGURAÇÃO
TRIG_PIN = 14
ECHO_PIN = 15
LIMIAR_CM = 5.0  # Distância abaixo da qual considera a porta/tranca FECHADA

# Instancia o sensor ultrassônico (max_distance de 3 metros para evitar timeouts longos)
sensor = DistanceSensor(echo=ECHO_PIN, trigger=TRIG_PIN, max_distance=3.0)

print("=== Teste Isolado: Sensor Ultrassônico (HC-SR04) ===")
print(f"Pinos configurados: TRIG = GPIO {TRIG_PIN} | ECHO = GPIO {ECHO_PIN}")
print(f"Limiar de detecção física: {LIMIAR_CM} cm\n")
print("Monitorando leituras... (Pressione Ctrl+C para encerrar)\n")

try:
    while True:
        # A propriedade 'distance' do gpiozero retorna o valor em metros (0.0 a 3.0)
        dist_cm = sensor.distance * 100
        
        # Determina se a tranca/porta está alinhada com o sensor
        estado = "TRANCADA / FECHADA" if dist_cm < LIMIAR_CM else "ABERTA / DESTRAVADA"
        
        print(f"Distância Medida: {dist_cm:5.1f} cm  |  Status: {estado}")
        sleep(0.5)

except KeyboardInterrupt:
    print("\n[INFO] Encerrando teste do sensor ultrassônico.")
finally:
    sensor.close()
