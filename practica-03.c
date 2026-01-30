/*
Realizar un contador ascendente y descendente (0 – 59 y 59 – 0 segundos) con displays de
7 segmentos multiplexados. El control se debe hacer mediante 4 pulsadores.
- Utilizar el PORTA o el PORTB como entrada de los pulsadores
- Utilizar el PORTB o el PORTD como la salida para los displays
- Utilizar una frecuencia de 4MHz para el reloj del PIC
- Temporizar mediante un timer (TMR0 ó TMR1) no use la sentencia delay_ms()
- Utilizar una única variable entera de 8 bits para el tiempo en segundo
- Se debe aplicar un filtro anti-rebote en la programación del microcontrolador
- La selección de cada función se hará presionando 1 pulsador según lo siguiente:
a) Pulsador 1: inicia temporización ascendente
b) Pulsador 2: inicia temporización descendente
c) Pulsador 3: pausa de la temporización
d) Pulsador 4: Reinicio de la temporización
- Se debe poder cambiar del modo ascendente y descendente y viceversa en
cualquier momento sin que se reinicie la cuenta del tiempo (esto es posible porque
ambos casos es la misma variable)
*/

#include <16f877a.h>
#fuses XT, NOWDT, NOLVP, NOPROTECT
#use delay(clock=4000000)

#define TUNI PORTC, 0
#define TDEC PORTC, 1

byte const display[10] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x67};
#byte PORTD = 8
#byte PORTC = 7
#byte PORTB = 6

int modo = 0;
int8 segundos = 0;
int8 contador = 0;

#int_TIMER0
void timer0_isr() {
    set_timer0(0);
    contador++;
    if(contador >= 60) { 
        contador = 0;
        if(modo == 1 && segundos < 59) segundos++;
        else if(modo == 2 && segundos > 0) segundos--;
    }
}

void pantalla() {
    int d = segundos / 10;
    int u = segundos % 10;

    // Unidades
    PORTD = display[u];
    bit_set(TUNI);
    delay_us(500);
    bit_clear(TUNI);

    // Decenas
    PORTD = display[d];
    bit_set(TDEC);
    delay_us(500);
    bit_clear(TDEC);
}

void main() {
    set_tris_d(0);
    set_tris_c(0);
    set_tris_b(0xFF);

    setup_timer_0(RTCC_INTERNAL | RTCC_DIV_64);
    set_timer0(0);
    enable_interrupts(INT_TIMER0);
    enable_interrupts(GLOBAL);

    while(TRUE) {
        
        int8 botones = PORTB & 0x0F;

        if(bit_test(botones,0)) modo = 1; // sbir
        else if(bit_test(botones,1)) modo = 2;  // bajar
        else if(bit_test(botones,2)) modo = 0;  // detener
        else if(bit_test(botones,3)) segundos = 0; // reiniciar

        if(modo == 1) ;
        else if(modo == 2) ; 

        pantalla();
    }
}