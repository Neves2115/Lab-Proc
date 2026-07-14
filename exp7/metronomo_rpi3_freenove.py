#!/usr/bin/env python3
"""
metronomo_rpi3.py

Experimento para Raspberry Pi 3 + Freenove FNK0019:
- teste isolado de LED com PWM e frequências variadas
- teste isolado de servomotor
- teste isolado de buzzer
- metrônomo integrado com temporização estável
- ajuste de BPM por botões físicos

Pinos em BCM (ajuste se o seu hardware estiver ligado em outros GPIOs):
- LED PWM: GPIO18
- Servo:   GPIO12
- Buzzer:  GPIO22
- Botão +: GPIO23
- Botão -: GPIO24

Dependência principal: gpiozero
No Raspberry Pi OS, a GPIO Zero costuma vir instalada por padrão.
"""

from __future__ import annotations

import argparse
import sys
import time
import threading
from dataclasses import dataclass

try:
    from gpiozero import PWMLED, Servo, Buzzer, Button
except ImportError as exc:
    print("Erro: a biblioteca gpiozero não está disponível neste ambiente.", file=sys.stderr)
    print("No Raspberry Pi OS, instale com: sudo apt install python3-gpiozero", file=sys.stderr)
    raise SystemExit(1) from exc


LED_PIN = 18
SERVO_PIN = 12
BUZZER_PIN = 22
BTN_UP_PIN = 23
BTN_DOWN_PIN = 24

LED_FREQS = [50, 100, 250, 500, 1000, 2000]
LED_DUTY = 0.5

SERVO_MIN_ANGLE = 20
SERVO_MAX_ANGLE = 160
SERVO_HOLD = 0.09

DEFAULT_BPM = 60
MIN_BPM = 30
MAX_BPM = 240


@dataclass
class SharedState:
    bpm: int = DEFAULT_BPM
    running: bool = True


def cleanup(*devices) -> None:
    for dev in devices:
        try:
            if dev is not None:
                dev.close()
        except Exception:
            pass


def led_pwm_test() -> None:
    """
    Varia a frequência do PWM mantendo duty cycle fixo em 50%.
    Útil para observar em que faixa o brilho parece estável e quando o LED passa a piscar.
    """
    led = PWMLED(LED_PIN, frequency=100)

    print("\n[Teste LED PWM]")
    print("GPIO18 com duty cycle fixo de 50%.")
    print("Ajustando frequências: " + ", ".join(f"{f} Hz" for f in LED_FREQS))
    print("Ctrl+C para sair.\n")

    try:
        while True:
            for freq in LED_FREQS:
                led.frequency = freq
                led.value = LED_DUTY
                print(f"Frequência = {freq:4d} Hz | duty cycle = {LED_DUTY*100:.0f}%")
                time.sleep(2.0)
    except KeyboardInterrupt:
        pass
    finally:
        led.off()
        cleanup(led)


def servo_test() -> None:
    """
    Move o servo entre dois extremos para validar o controle isolado.
    """
    servo = Servo(
        SERVO_PIN,
        min_pulse_width=0.001,
        max_pulse_width=0.002,
        frame_width=0.020,
    )

    print("\n[Teste Servo]")
    print("Movendo entre posição mínima, média e máxima.")
    print("Ctrl+C para sair.\n")

    try:
        while True:
            servo.min()
            print("Servo: min")
            time.sleep(1.0)

            servo.mid()
            print("Servo: mid")
            time.sleep(1.0)

            servo.max()
            print("Servo: max")
            time.sleep(1.0)
    except KeyboardInterrupt:
        pass
    finally:
        servo.detach()
        cleanup(servo)


def buzzer_test() -> None:
    """
    Aciona o buzzer em pulsos curtos para validar o circuito isoladamente.
    """
    buzzer = Buzzer(BUZZER_PIN)

    print("\n[Teste Buzzer]")
    print("Gerando pulsos curtos de 120 ms.")
    print("Ctrl+C para sair.\n")

    try:
        while True:
            buzzer.on()
            print("Buzzer: ON")
            time.sleep(0.12)
            buzzer.off()
            print("Buzzer: OFF")
            time.sleep(0.88)
    except KeyboardInterrupt:
        pass
    finally:
        buzzer.off()
        cleanup(buzzer)


def run_metronome() -> None:
    """
    Metrônomo integrado:
    - servo marca o pulso em um movimento curto
    - buzzer emite o click
    - botões físicos aumentam/diminuem o BPM
    """
    led = PWMLED(LED_PIN, frequency=100)  # opcional: indicador visual
    servo = Servo(
        SERVO_PIN,
        min_pulse_width=0.001,
        max_pulse_width=0.002,
        frame_width=0.020,
    )
    buzzer = Buzzer(BUZZER_PIN)
    state = SharedState()

    lock = threading.Lock()

    btn_up = Button(BTN_UP_PIN, pull_up=True, bounce_time=0.05)
    btn_down = Button(BTN_DOWN_PIN, pull_up=True, bounce_time=0.05)

    def set_bpm(delta: int) -> None:
        with lock:
            state.bpm = max(MIN_BPM, min(MAX_BPM, state.bpm + delta))
            print(f"BPM ajustado para {state.bpm}")

    btn_up.when_pressed = lambda: set_bpm(+5)
    btn_down.when_pressed = lambda: set_bpm(-5)

    print("\n[Metrônomo]")
    print("Botão + -> aumenta BPM")
    print("Botão - -> diminui BPM")
    print(f"Faixa: {MIN_BPM} a {MAX_BPM} BPM | inicial: {DEFAULT_BPM} BPM")
    print("Ctrl+C para encerrar.\n")

    next_tick = time.perf_counter()
    beat = 0

    try:
        while state.running:
            with lock:
                bpm = state.bpm
            interval = 60.0 / bpm

            next_tick += interval
            beat += 1

            # Pulso visual
            led.value = 0.35 if (beat % 2 == 0) else 0.85

            # Pulso mecânico + sonoro
            servo.max() if (beat % 2 == 0) else servo.min()
            buzzer.on()

            time.sleep(SERVO_HOLD)
            buzzer.off()

            # Retorna o servo para o meio após o clique
            servo.mid()
            led.value = 0.0

            # Compensa o tempo gasto para manter a cadência
            sleep_time = next_tick - time.perf_counter()
            if sleep_time > 0:
                time.sleep(sleep_time)
            else:
                # Se atrasou, reposiciona a agenda para evitar drift acumulado
                next_tick = time.perf_counter()

            print(f"Beat {beat:04d} | BPM {bpm} | intervalo {interval:.3f} s")

    except KeyboardInterrupt:
        print("\nEncerrando...")
    finally:
        buzzer.off()
        led.off()
        servo.detach()
        cleanup(btn_up, btn_down, buzzer, servo, led)


def main() -> None:
    parser = argparse.ArgumentParser(description="Experimento de PWM e metrônomo para Raspberry Pi 3")
    parser.add_argument(
        "modo",
        choices=["led", "servo", "buzzer", "metronomo"],
        help="Selecione o experimento: led, servo, buzzer ou metronomo",
    )
    args = parser.parse_args()

    if args.modo == "led":
        led_pwm_test()
    elif args.modo == "servo":
        servo_test()
    elif args.modo == "buzzer":
        buzzer_test()
    else:
        run_metronome()


if __name__ == "__main__":
    main()
