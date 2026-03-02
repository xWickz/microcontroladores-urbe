#include <16F877A.h>
#device ADC=10 // Añadido para lectura de 10 bits
#fuses XT, NOWDT, NOLVP, NOPROTECT
#use delay(clock=4000000)
#include <lcd.c>

// Configuración de pines LCD (PORTD)
#define LCD_RS_PIN      PIN_D0
#define LCD_RW_PIN      PIN_D1
#define LCD_E_PIN       PIN_D2
#define LCD_DATA4       PIN_D4
#define LCD_DATA5       PIN_D5
#define LCD_DATA6       PIN_D6
#define LCD_DATA7       PIN_D7

#define BTN_MODO   PIN_B0   
#define BTN_INC    PIN_B1   
#define BTN_DEC    PIN_B2   
#define BTN_OK     PIN_B3   
#define LED_AL     PIN_E0

// Variables de 8 bits (Rúbrica: int8)
int8 HORA=12, MIN=0, SEG=0, DIA=1, MES=1, ANO=26;
int8 AL_HORA=6, AL_MIN=0;
int1 ALARM_ON=0, ALARM_SIL=0, ajustar_minutos=0;
int8 modo=0;
int8 ajuste_f = 0;
volatile int16 ticks=0;
volatile int1 flag_1s=0;
int8 ah_disp;  // Para la hora de alarma formateada
char a_ampm;   // Para el am/pm de la alarma
int8 ajustar_reloj = 0;

// Nuevas variables para Temperatura y Vista
int8 vista_principal = 0; // 0: Fecha, 1: Temperatura
float temperatura = 0.0;
int16 valor_adc;

#int_TIMER0
void timer0_isr() {
   set_timer0(6);
   if(++ticks >= 125) { ticks=0; flag_1s=1; }
}

// Función para determinar si el año es bisiesto
int8 dias_febrero(int8 y) {
   if(((2000+y) % 4 == 0)) return 29;
   return 28;
}

// Guardado del Configurado eeprom
void guardar_config() {
   write_eeprom(0, AL_HORA);
   write_eeprom(1, AL_MIN);
   write_eeprom(2, DIA);
   write_eeprom(3, MES);
   write_eeprom(4, ANO);
   write_eeprom(5, (int8)ALARM_ON);
}

// Leer datos de EEPROM
void cargar_config() {
   AL_HORA  = read_eeprom(0);
   AL_MIN   = read_eeprom(1);
   DIA      = read_eeprom(2);
   MES      = read_eeprom(3);
   ANO      = read_eeprom(4);
   ALARM_ON = (int1)read_eeprom(5);
   
   if(ANO >= 100)  ANO = 26;  
   if(MES > 12 || MES == 0) MES = 2;
   if(DIA > 31 || DIA == 0) DIA = 3;
   if(AL_HORA > 23) AL_HORA = 12;
   if(AL_MIN > 59)  AL_MIN = 1;
}

void main() {
   // Configuración ADC para el LM35
   setup_adc_ports(AN0); // Pin A0 analógico
   setup_adc(ADC_CLOCK_INTERNAL);
   set_adc_channel(0);

   set_tris_b(0x0F); 
   set_tris_e(0x00);
   output_low(LED_AL);
   
   cargar_config(); 
   
   setup_timer_0(RTCC_INTERNAL | RTCC_DIV_32);
   set_timer0(6);
   
   lcd_init();
   enable_interrupts(INT_TIMER0);
   enable_interrupts(GLOBAL);

   while(TRUE) {
      if(flag_1s) {
         flag_1s = 0;
         
         // Lectura de Temperatura
         valor_adc = read_adc();
         temperatura = (valor_adc * 0.48828); // Conversión para LM35 a 5V (500/1024)

         if(++SEG >= 60) {
            SEG = 0; ALARM_SIL = 0;
            if(++MIN >= 60) {
               MIN = 0;
               if(++HORA >= 24) { 
                  HORA = 0; DIA++;
                  int8 d_max = 31;
                  if(MES==4||MES==6||MES==9||MES==11) d_max=30;
                  else if(MES==2) d_max = dias_febrero(ANO); 
                  if(DIA > d_max) { DIA=1; if(++MES>12){ MES=1; ANO++; } }
               }
            }
         }
      }

      // Alarma (RE0)
      if(ALARM_ON && !ALARM_SIL && HORA == AL_HORA && MIN == AL_MIN) {
         output_high(LED_AL);
         if(modo == 0 && input(BTN_OK)) { 
            ALARM_SIL = 1; output_low(LED_AL); 
          }
      } else { 
         output_low(LED_AL); 
      }

    // Alternar vistas con BTN_OK (Botón 3) en Modo 0
      // Sustituimos output_read por input_state para verificar si el LED_AL está activo
      if(modo == 0 && input(BTN_OK) && !input_state(LED_AL)) {
         delay_ms(20);
         vista_principal = !vista_principal; // Alterna entre 0 y 1 (Fecha / Temperatura)
         lcd_putc("\f"); // Limpia para cambio suave
         while(input(BTN_OK)); // Espera a que se suelte el botón
      }

      // Botón MODO
      if(input(BTN_MODO)) {
         delay_ms(20);
         guardar_config(); 
         if(++modo > 4) modo = 0; 
         ajustar_minutos = 0; 
         lcd_putc("\f");
         while(input(BTN_MODO));
      }

      // Lógica AM/PM
      int8 h_disp = HORA;
      char ampm = 'a';
      if(h_disp >= 12) { 
         ampm = 'p'; if(h_disp > 12) h_disp -= 12; 
      }
      if(h_disp == 0) h_disp = 12;

      switch(modo) {
         case 0: // HORA + (FECHA o TEMP)
            lcd_gotoxy(1,1);
            printf(lcd_putc, "%02u:%02u:%02u %cm      ", h_disp, MIN, SEG, ampm);
            
            lcd_gotoxy(1,2);
            if(vista_principal == 0) {
               printf(lcd_putc, "%02u/%02u/20%02u      ", DIA, MES, ANO); 
            } else {
               printf(lcd_putc, "TEMP: %2.1f %cC   ", temperatura, 223); // 223 es símbolo grados
            }
            break;

         case 1: // HORA / ALARMA
            lcd_gotoxy(1,1);
            printf(lcd_putc, "HORA: %02u:%02u %cm", h_disp, MIN, ampm);
            lcd_gotoxy(1,2);
            ah_disp = AL_HORA;
            a_ampm = 'a';
            if(ah_disp >= 12) { 
               a_ampm = 'p'; if(ah_disp > 12) ah_disp -= 12; 
            }
            if(ah_disp == 0) ah_disp = 12;
            printf(lcd_putc, "ALM: %02u:%02u %cm ", ah_disp, AL_MIN, a_ampm);
            break;

         case 2: // AJUSTE ALARMA
            lcd_gotoxy(1,1);
            if(ajustar_reloj == 0)      lcd_putc("EDIT: HORA ALM  ");
            else if(ajustar_reloj == 1) lcd_putc("EDIT: MIN ALM   ");
            else                        lcd_putc("EDIT: ALM AM/PM ");
            
            ah_disp = AL_HORA;
            a_ampm = 'a';
            if(ah_disp >= 12) { a_ampm = 'p'; if(ah_disp > 12) ah_disp -= 12; }
            if(ah_disp == 0) ah_disp = 12;

            lcd_gotoxy(1,2);
            printf(lcd_putc, "SET: %02u:%02u %cm      ", ah_disp, AL_MIN, a_ampm);
            
            if(input(BTN_INC)) {
               delay_ms(20);
               if(ajustar_reloj == 0) { if(++AL_HORA >= 24) AL_HORA = 0; }
               else if(ajustar_reloj == 1) { if(++AL_MIN >= 60) AL_MIN = 0; }
               else { if(AL_HORA < 12) AL_HORA += 12; else AL_HORA -= 12; }
               while(input(BTN_INC));
            }
            if(input(BTN_DEC)) {
               delay_ms(20);
               if(ajustar_reloj == 0) { if(AL_HORA == 0) AL_HORA = 23; else AL_HORA--; }
               else if(ajustar_reloj == 1) { if(AL_MIN == 0) AL_MIN = 59; else AL_MIN--; }
               else { if(AL_HORA < 12) AL_HORA += 12; else AL_HORA -= 12; }
               while(input(BTN_DEC));
            }
            if(input(BTN_OK)) { 
               delay_ms(20); 
               if(++ajustar_reloj > 2) ajustar_reloj = 0; 
               ALARM_ON = 1; 
               while(input(BTN_OK)); 
            }
            break;

         case 3: // AJUSTE RELOJ
            lcd_gotoxy(1,1);
            if(ajustar_reloj == 0)      lcd_putc("EDIT: HORA RELOJ");
            else if(ajustar_reloj == 1) lcd_putc("EDIT: MIN RELOJ ");
            else                        lcd_putc("EDIT: AM / PM   ");
            
            lcd_gotoxy(1,2);
            printf(lcd_putc, "HORA: %02u:%02u %cm   ", h_disp, MIN, ampm);
            
            if(input(BTN_INC)) {
               delay_ms(20);
               if(ajustar_reloj == 0) { if(++HORA >= 24) HORA = 0; }
               else if(ajustar_reloj == 1) { if(++MIN >= 60) MIN = 0; }
               else { if(HORA < 12) HORA += 12; else HORA -= 12; }
               while(input(BTN_INC));
            }
            if(input(BTN_DEC)) {
               delay_ms(20);
               if(ajustar_reloj == 0) { if(HORA == 0) HORA = 23; else HORA--; }
               else if(ajustar_reloj == 1) { if(MIN == 0) MIN = 59; else MIN--; }
               else { if(HORA < 12) HORA += 12; else HORA -= 12; }
               while(input(BTN_DEC));
            }
            if(input(BTN_OK)) { 
               delay_ms(20); 
               if(++ajustar_reloj > 2) ajustar_reloj = 0; 
               while(input(BTN_OK)); 
            }
            break;

         case 4: // AJUSTE FECHA
            lcd_gotoxy(1,1);
            if(ajuste_f == 0)      lcd_putc("EDIT: DIA       ");
            else if(ajuste_f == 1) lcd_putc("EDIT: MES       ");
            else                   lcd_putc("EDIT: ANO       ");
            
            lcd_gotoxy(1,2);
            printf(lcd_putc, "F: %02u/%02u/20%02u      ", DIA, MES, ANO);
            
            int8 d_max_aj = 31;
            if(MES==4||MES==6||MES==9||MES==11) d_max_aj = 30;
            else if(MES==2) d_max_aj = dias_febrero(ANO);

            if(input(BTN_INC)) {
               delay_ms(20);
               if(ajuste_f == 0) { 
                  if(++DIA > d_max_aj) { DIA = 1; if(++MES > 12) { MES = 1; if(++ANO > 99) ANO = 0; } }
               }
               else if(ajuste_f == 1) { if(++MES > 12) MES = 1; }
               else { if(++ANO > 99) ANO = 0; }
               while(input(BTN_INC));
            }
            if(input(BTN_DEC)) {
               delay_ms(20);
               if(ajuste_f == 0) { 
                  if(DIA == 1) { 
                     if(--MES < 1) { MES = 12; if(ANO == 0) ANO = 99; else ANO--; }
                     DIA = (MES==2) ? dias_febrero(ANO) : ((MES==4||MES==6||MES==9||MES==11)?30:31);
                  } else DIA--;
               }
               else if(ajuste_f == 1) { if(MES == 1) MES = 12; else MES--; }
               else { if(ANO == 0) ANO = 99; else ANO--; }
               while(input(BTN_DEC));
            }
            if(input(BTN_OK)) { 
               delay_ms(20); 
               if(++ajuste_f > 2) ajuste_f = 0; 
               while(input(BTN_OK)); 
            }
            break;
      }
      delay_ms(40);
   }
}