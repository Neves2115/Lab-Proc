from gpiozero import Buzzer
from time import sleep

BUZZER_PIN = 12

buzzer = Buzzer(BUZZER_PIN)

try:
    while True:
        buzzer.on()
        print("Buzzer ligado")
        sleep(1)

        buzzer.off()
        print("Buzzer desligado")
        sleep(1)

except KeyboardInterrupt:
    buzzer.off()
