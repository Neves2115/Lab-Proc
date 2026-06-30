# =====================================================================
#  calc_riscv.s - Calculadora binaria de 4 bits em Assembly RISC-V
#  Operacoes: soma, subtracao, multiplicacao, fatorial e divisao.
#  Testado no CPUlator (target RISC-V RV32IM). Sintaxe GNU as.
#  Convencao: operandos em a0 e a1; resultado em a0 (resto em a1).
# =====================================================================
.global _start
.text

_start:
    li    a0, 12           # operando A (0..15)
    li    a1, 3            # operando B (0..15)
    jal   ra, soma         # a0 = A + B
    mv    s0, a0           # s0 = resultado da soma

    li    a0, 12
    li    a1, 3
    jal   ra, divisao      # a0 = A / B (quociente), a1 = resto
    mv    s1, a0           # s1 = quociente

    li    a0, 5
    jal   ra, fatorial     # a0 = 5! = 120
    mv    s2, a0           # s2 = fatorial

fim_prog:
    j     fim_prog         # fim do programa (trava o fluxo no simulador)

# ---------------------------------------------------------------------
soma:                      # a0 = a0 + a1
    add   a0, a0, a1
    ret

subtracao:                 # a0 = a0 - a1
    sub   a0, a0, a1
    ret

multiplicacao:             # a0 = a0 * a1  (extensao M)
    mul   a0, a0, a1
    ret

# ---------------------------------------------------------------------
# fatorial: a0 = a0!  (iterativo)
fatorial:
    mv    t1, a0           # t1 = n
    li    a0, 1            # resultado = 1
    li    t0, 1
fat_loop:
    ble   t1, t0, fat_fim  # se n <= 1, termina
    mul   a0, a0, t1       # resultado *= n
    addi  t1, t1, -1       # n--
    j     fat_loop
fat_fim:
    ret

# ---------------------------------------------------------------------
# divisao: a0 = a0 / a1 (quociente), a1 = resto, por subtracao sucessiva.
# Guarda de divisor zero explicita. Em hardware (extensao M) poderia ser
# "div a0, a0, a1" apos a verificacao; lembrando que o DIV do RISC-V NAO
# gera trap por divisao por zero (retorna todos os bits em 1).
divisao:
    beqz  a1, div_zero     # divisor == 0 ?
    li    t2, 0            # quociente = 0
div_loop:
    blt   a0, a1, div_fim   # enquanto dividendo >= divisor
    sub   a0, a0, a1       # dividendo -= divisor
    addi  t2, t2, 1        # quociente++
    j     div_loop
div_fim:
    mv    a1, a0           # resto
    mv    a0, t2           # quociente
    ret
div_zero:
    li    a0, -1           # sinalizador de erro (o software exibe o aviso)
    li    a1, -1
    ret
