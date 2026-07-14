#!/usr/bin/env python3
from gpiozero import AngularServo, TonalBuzzer
from gpiozero.tones import Tone
from time import sleep, perf_counter

SERVO_PIN = 18
BUZZER_PIN = 4
PULSE_TIME = 0.10

servo = AngularServo(
    SERVO_PIN,
    min_angle=-90,
    max_angle=90,
    min_pulse_width=0.001,
    max_pulse_width=0.002,
)
buzzer = TonalBuzzer(BUZZER_PIN)

estado = False

try:
    while True:
        t0 = perf_counter()

        servo.angle = -45 if estado else 45
        buzzer.play(Tone("A5"))
        sleep(PULSE_TIME)
        buzzer.stop()

        estado = not estado

        restante = 1.0 - (perf_counter() - t0)
        if restante > 0:
            sleep(restante)
finally:
    buzzer.stop()
    servo.detach()
