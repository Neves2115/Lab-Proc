from gpiozero import TonalBuzzer
from gpiozero.tones import Tone
from time import sleep

buzzer = TonalBuzzer(4)

while True:
    buzzer.play(Tone("A4"))
    sleep(1)
    buzzer.stop()
    sleep(1)
