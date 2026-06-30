@ =====================================================================
@  calc_arm.s  -  Calculadora binaria de 4 bits em Assembly ARM (ARMv7)
@  Operacoes: soma, subtracao, multiplicacao, fatorial e divisao.
@  Testado no CPUlator (target ARMv7 / DE1-SoC). Sintaxe GNU as.
@  Convencao: operandos em r0 e r1; resultado em r0 (resto em r1).
@ =====================================================================
.global _start
.text

_start:
    @ ---- Demonstracao: ajuste os operandos para testar no simulador ----
    mov   r0, #12          @ operando A (0..15)
    mov   r1, #3           @ operando B (0..15)
    bl    soma             @ r0 = A + B
    mov   r4, r0           @ r4 = resultado da soma

    mov   r0, #12
    mov   r1, #3
    bl    divisao          @ r0 = A / B (quociente), r1 = resto
    mov   r5, r0           @ r5 = quociente

    mov   r0, #5
    bl    fatorial         @ r0 = 5! = 120
    mov   r6, r0           @ r6 = fatorial

fim_prog:
    b     fim_prog         @ fim do programa (trava o fluxo no simulador)

@ ---------------------------------------------------------------------
soma:                      @ r0 = r0 + r1
    add   r0, r0, r1
    bx    lr

subtracao:                 @ r0 = r0 - r1
    sub   r0, r0, r1
    bx    lr

multiplicacao:             @ r0 = r0 * r1  (MUL e instrucao base no ARMv7)
    mul   r0, r0, r1
    bx    lr

@ ---------------------------------------------------------------------
@ fatorial: r0 = r0!  (iterativo; mesma forma do C: while(n>1){r*=n; n--})
fatorial:
    mov   r2, r0           @ r2 = n
    mov   r0, #1           @ resultado = 1
fat_loop:
    cmp   r2, #1
    ble   fat_fim          @ se n <= 1, termina
    mul   r0, r0, r2       @ resultado *= n
    sub   r2, r2, #1       @ n--
    b     fat_loop
fat_fim:
    bx    lr

@ ---------------------------------------------------------------------
@ divisao: r0 = r0 / r1 (quociente), r1 = resto.
@ Feita por subtracao sucessiva, para ser portavel (sem depender da
@ extensao de divisao) e deixar a guarda de divisor zero explicita.
@ Em hardware com UDIV, bastaria "udiv r0, r0, r1" apos a verificacao.
divisao:
    cmp   r1, #0
    beq   div_zero         @ divisor == 0 ? -> nao divide
    mov   r2, #0           @ quociente = 0
div_loop:
    cmp   r0, r1
    blt   div_fim          @ enquanto dividendo >= divisor
    sub   r0, r0, r1       @ dividendo -= divisor
    add   r2, r2, #1       @ quociente++
    b     div_loop
div_fim:
    mov   r1, r0           @ resto = o que sobrou
    mov   r0, r2           @ r0 = quociente
    bx    lr
div_zero:
    mov   r0, #-1          @ sinalizador de erro (o software exibe o aviso)
    mov   r1, #-1
    bx    lr
