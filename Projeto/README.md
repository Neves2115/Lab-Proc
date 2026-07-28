# Central de comandos por voz para o kit Freenove

## Estrutura de código
- `src/central_voz_freenove/hardware/led.py`
- `src/central_voz_freenove/hardware/servo.py`
- `src/central_voz_freenove/hardware/lcd.py`
- `src/central_voz_freenove/hardware/distance.py`
- `src/central_voz_freenove/hardware/matrix.py`
- `src/central_voz_freenove/recognition/commands.py`
- `src/central_voz_freenove/recognition/vosk_engine.py`
- `src/central_voz_freenove/audio/recorder.py`
- `src/central_voz_freenove/app/controller.py`
- `src/central_voz_freenove/main.py`

## Como testar em modo simulado
```bash
python -m central_voz_freenove.main
```

## Ordem recomendada de integração
1. LED
2. Servo
3. LCD
4. Sensor de distância
5. Matriz de LEDs
6. Reconhecimento de fala com Vosk
