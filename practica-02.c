/*
Realizar 8 secuencias con 8 leds, con una duración de 4 segundos cada una, basado en
tiempos múltiplos de 125mS (125mS, 250mS, 500mS, 1S), debe apagar todos los leds al
finalizar hasta que se seleccione una nueva rutina. Las rutinas se deben seleccionar
mediante 4 pulsadores (cada pulsador o combinación de dos inicializara una rutina).
- Utilizar el PORTA o el PORTB como entrada de los pulsadores
- Utilizar el PORTB o el PORTD como la salida hacia los leds
- Utilizar una frecuencia de 4MHz para el reloj del PIC
- Temporizar con la sentencia “delay_ms();”
- Utilizar bucle infinito para la secuencia de trabajo
- La selección de cada secuencia se hará presionando 1 o 2 de los pulsadores
- Se debe aplicar un filtro anti-rebote en la programación del microcontrolador
*/

#include <16F877A.h>
#fuses XT, NOWDT, NOLVP, NOPROTECT
#use delay(clock=4000000) // 4mhz

#byte PORTB = 8

void secuencia1();
void secuencia2();
void secuencia3();
void secuencia4();
void secuencia5();
void secuencia6();
void secuencia7();
void secuencia8();

void main() {

   set_tris_b(0x0F);
   set_tris_d(0x00);

   output_d(0x00);

   while(TRUE) {

      // RB0
      if (input(PIN_B0) && !input(PIN_B1) && !input(PIN_B2)) {
         delay_ms(30);
         if (input(PIN_B0)) secuencia1();
      }

      // RB1
      if (input(PIN_B1) && !input(PIN_B0) && !input(PIN_B2)) {
         delay_ms(30);
         if (input(PIN_B1)) secuencia2();
      }

      // RB2
      if (input(PIN_B2) && !input(PIN_B0) && !input(PIN_B1)) {
         delay_ms(30);
         if (input(PIN_B2)) secuencia3();
      }

      // RB3
      if (input(PIN_B3)) {
         delay_ms(30);
         if (input(PIN_B3)) secuencia4();
      }

      // RB0 + RB1
      if (input(PIN_B0) && input(PIN_B1)) {
         delay_ms(30);
         if (input(PIN_B0) && input(PIN_B1)) secuencia5();
      }

      // RB0 + RB2
      if (input(PIN_B0) && input(PIN_B2)) {
         delay_ms(30);
         if (input(PIN_B0) && input(PIN_B2)) secuencia6();
      }

      // RB1 + RB2
      if (input(PIN_B1) && input(PIN_B2)) {
         delay_ms(30);
         if (input(PIN_B1) && input(PIN_B2)) secuencia7();
      }

      // RB2 + RB3
      if (input(PIN_B2) && input(PIN_B3)) {
         delay_ms(30);
         if (input(PIN_B2) && input(PIN_B3)) secuencia8();
      }
   }
}

// Secuencia 1: Corrimiento simple
void secuencia1() {
   for(int t=0; t<4; t++) {
      for(int i=0; i<8; i++) {
         output_high(PIN_D0+i);
         delay_ms(125);
         output_low(PIN_D0+i);
      }
   }
   output_d(0x00);
}

// Secuencia 2: Todos ON / OFF (250 ms)
void secuencia2() {
   for(int i=0; i<8; i++) {
      output_d(0xFF);
      delay_ms(250);
      output_d(0x00);
      delay_ms(250);
   }
}

// Secuencia 3: Patrón alternado
void secuencia3() {
   for(int i=0; i<16; i++) {
      output_d(0xAA);
      delay_ms(250);
      output_d(0x55);
      delay_ms(250);
   }
   output_d(0x00);
}

// Secuencia 4: Corrimiento inverso
void secuencia4() {
   for(int t=0; t<4; t++) {
      for(int i=7; i>=0; i--) {
         output_high(PIN_D0+i);
         delay_ms(125);
         output_low(PIN_D0+i);
      }
   }
   output_d(0x00);
}

// Secuencia 5: Mitades
void secuencia5() {
   for(int i=0; i<16; i++) {
      output_d(0x0F);
      delay_ms(250);
      output_d(0xF0);
      delay_ms(250);
   }
   output_d(0x00);
}

// Secuencia 6: Acumulativo
void secuencia6() {
   int mask = 0;
   for(int i=0; i<8; i++) {
      mask = (mask << 1) | 1;
      output_d(mask);
      delay_ms(500);
   }
   output_d(0x00);
}

// Secuencia 7: Rebote
void secuencia7() {
   for(int t=0; t<2; t++) {
      for(int i=0; i<8; i++) {
         output_high(PIN_D0+i);
         delay_ms(125);
         output_low(PIN_D0+i);
      }
      for(int i=6; i>0; i--) {
         output_high(PIN_D0+i);
         delay_ms(125);
         output_low(PIN_D0+i);
      }
   }
   output_d(0x00);
}

// Secuencia 8: Todos encendidos progresivo
void secuencia8() {
   for(int i=0; i<8; i++) {
      output_d((1<<(i+1))-1);
      delay_ms(500);
   }
   output_d(0x00);
}