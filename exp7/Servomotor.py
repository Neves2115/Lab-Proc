#!/usr/bin/env python3
from gpiozero import AngularServo
from time import sleep

SERVO_PIN = 18

servo = AngularServo(
    SERVO_PIN,
    min_angle=-90,
    max_angle=90,
    min_pulse_width=0.001,
    max_pulse_width=0.002,
)

try:
    while True:
        servo.angle = -45
        print("Servo: -45")
        sleep(1)

        servo.angle = 0
        print("Servo: 0")
        sleep(1)

        servo.angle = 45
        print("Servo: 45")
        sleep(1)
finally:
    servo.detach()
