#!/usr/bin/env python3
import time
import smbus2

I2C_ADDR = 0x27  # Endereco I2C padrao (pode ser 0x3F dependendo do modulo)
BUS = smbus2.SMBus(1)

ENABLE = 0b00000100
BACKLIGHT = 0b00001000
CMD = 0
DATA = 1

def lcd_strobe(data):
    BUS.write_byte(I2C_ADDR, data | ENABLE | BACKLIGHT)
    time.sleep(0.0005)
    BUS.write_byte(I2C_ADDR, (data & ~ENABLE) | BACKLIGHT)
    time.sleep(0.0001)

def lcd_write_nibble(data):
    BUS.write_byte(I2C_ADDR, data | BACKLIGHT)
    lcd_strobe(data)

def lcd_send(data, mode=0):
    high = mode | (data & 0xF0)
    low = mode | ((data << 4) & 0xF0)
    lcd_write_nibble(high)
    lcd_write_nibble(low)

def lcd_init():
    lcd_send(0x33, CMD)
    lcd_send(0x32, CMD)
    lcd_send(0x06, CMD)  # Auto increment
    lcd_send(0x0C, CMD)  # Display ON, Cursor OFF
    lcd_send(0x28, CMD)  # 2 linhas, 5x8
    lcd_send(0x01, CMD)  # Clear
    time.sleep(0.005)

def lcd_message(text, line=1):
    addr = 0x80 if line == 1 else 0xC0
    lcd_send(addr, CMD)
    for char in text.ljust(16):
        lcd_send(ord(char), DATA)

print("Testando Display LCD I2C... (Ctrl+C para sair)")
try:
    lcd_init()
    lcd_message("Status: OK", line=1)
    lcd_message("Fechadura: PRONTA", line=2)
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("\nEncerrando e limpando tela.")
    lcd_send(0x01, CMD)
    BUS.close()