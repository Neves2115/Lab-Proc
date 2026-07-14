#!/usr/bin/env python3
from gpiozero import AngularServo, TonalBuzzer, Button
from gpiozero.tones import Tone
from time import sleep, perf_counter

SERVO_PIN = 18
BUZZER_PIN = 4
BTN_MAIS = 23
BTN_MENOS = 24

PULSE_TIME = 0.10
PASSO_BPM = 5
BPM_MIN = 30
BPM_MAX = 240

servo = AngularServo(
    SERVO_PIN,
    min_angle=-90,
    max_angle=90,
    min_pulse_width=0.001,
    max_pulse_width=0.002,
)
buzzer = TonalBuzzer(BUZZER_PIN)
btn_mais = Button(BTN_MAIS, pull_up=True, bounce_time=0.05)
btn_menos = Button(BTN_MENOS, pull_up=True, bounce_time=0.05)

bpm = 60
estado = False

def aumentar_bpm():
    global bpm
    bpm = min(BPM_MAX, bpm + PASSO_BPM)
    print(f"BPM = {bpm}")

def diminuir_bpm():
    global bpm
    bpm = max(BPM_MIN, bpm - PASSO_BPM)
    print(f"BPM = {bpm}")

btn_mais.when_pressed = aumentar_bpm
btn_menos.when_pressed = diminuir_bpm

print("Botão + aumenta o BPM")
print("Botão - diminui o BPM")
print(f"Faixa: {BPM_MIN} a {BPM_MAX} BPM")

try:
    while True:
        t0 = perf_counter()
        intervalo = 60.0 / bpm

        servo.angle = -45 if estado else 45
        buzzer.play(Tone("A5"))
        sleep(PULSE_TIME)
        buzzer.stop()

        estado = not estado

        restante = intervalo - (perf_counter() - t0)
        if restante > 0:
            sleep(restante)
finally:
    buzzer.stop()
    servo.detach()
    btn_mais.close()
    btn_menos.close()
