#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define REPETICOES 5
#define ITERACOES 10000000UL

typedef int16_t tipo_t;

uint16_t MASK;
uint16_t BIT_SINAL;

tipo_t comp2(tipo_t x)
{
    uint16_t v = (uint16_t)x & MASK;

    if (v & BIT_SINAL)
        v |= ~MASK;

    return (tipo_t)v;
}

tipo_t soma(tipo_t a, tipo_t b)
{
    return comp2(a + b);
}

tipo_t sub(tipo_t a, tipo_t b)
{
    return comp2(a - b);
}

tipo_t mult(tipo_t a, tipo_t b)
{
    return comp2(a * b);
}

tipo_t fat(tipo_t n)
{
    if (n < 0)
        return 0;

    tipo_t r = 1;

    for (tipo_t i = 2; i <= n; i++)
        r = comp2(r * i);

    return r;
}

double tempo_ns(clock_t ini, clock_t fim)
{
    return (double)(fim - ini) * 1e9 / CLOCKS_PER_SEC;
}

int main()
{
    volatile tipo_t r;
    tipo_t a = -3;
    tipo_t b = 5;

    clock_t ini, fim;

    int tamanhos[] = {4, 8, 16};

    for (int t = 0; t < 3; t++)
    {
        int bits = tamanhos[t];

        MASK = (1U << bits) - 1;

        // Caso especial para 16 bits
        if (bits == 16)
            MASK = 0xFFFF;

        BIT_SINAL = 1U << (bits - 1);

        printf("\n=====================================\n");
        printf("ALU DE %d BITS\n", bits);
        printf("=====================================\n");

        printf("\nSOMA\n");
        for (int k = 0; k < REPETICOES; k++)
        {
            ini = clock();

            for (unsigned long i = 0; i < ITERACOES; i++)
                r = soma(a, b);

            fim = clock();

            printf("Execucao %d: %.3f ns/op\n",
                   k + 1,
                   tempo_ns(ini, fim) / ITERACOES);
        }

        printf("\nSUBTRACAO\n");
        for (int k = 0; k < REPETICOES; k++)
        {
            ini = clock();

            for (unsigned long i = 0; i < ITERACOES; i++)
                r = sub(a, b);

            fim = clock();

            printf("Execucao %d: %.3f ns/op\n",
                   k + 1,
                   tempo_ns(ini, fim) / ITERACOES);
        }

        printf("\nMULTIPLICACAO\n");
        for (int k = 0; k < REPETICOES; k++)
        {
            ini = clock();

            for (unsigned long i = 0; i < ITERACOES; i++)
                r = mult(a, b);

            fim = clock();

            printf("Execucao %d: %.3f ns/op\n",
                   k + 1,
                   tempo_ns(ini, fim) / ITERACOES);
        }

        printf("\nFATORIAL\n");
        for (int k = 0; k < REPETICOES; k++)
        {
            ini = clock();

            for (unsigned long i = 0; i < ITERACOES; i++)
                r = fat(5);

            fim = clock();

            printf("Execucao %d: %.3f ns/op\n",
                   k + 1,
                   tempo_ns(ini, fim) / ITERACOES);
        }
    }

    return 0;
}
