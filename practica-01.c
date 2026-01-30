/*
Realizar un secuencial de 8 leds con una duración de 8 segundos, basado en tiempos
múltiplos de 125mS (125mS, 250mS, 500mS, 1S) con una duración total de 8S, luego debe
apagar todos los leds por un periodo de 2S y finalmente repetir este ciclo de forma infinita.
- Utilizar el PORTB o el PORTD como la salida hacia los leds
- Utilizar una frecuencia de 4MHz para el reloj del PIC
- Temporizar con la sentencia "delay_ms();"
- Utilizar bucle infinito para la secuencia de trabajo
*/

#include <16F877A.h>
#fuses XT, NOWDT, NOLVP, NOPROTECT
#use delay(clock=4000000) // 4mhz

#byte PORTB = 6

void main() {

   set_tris_b(0);
   output_b(0x00);
   
   while (TRUE) {
      for (int i = 0; i < 8; i++) {
         output_high(PIN_B0 + i);
         for (int j = 0; j < 8; j++) {
            delay_ms(125);
         }
         output_low(PIN_B0 + i);
      }
      
      output_b(0x00);
      delay_ms(2000);
   }
}
