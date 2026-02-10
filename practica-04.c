/*
Realizar un reloj, calendario, alarma que se muestre en un módulo LCD 16x2. El control se
debe hacer mediante 4 pulsadores.
- Utilizar el PORTA o el PORTB como entrada de los pulsadores
- Se debe aplicar un filtro anti-rebote en la programación del microcontrolador
- Utilizar el PORTB o el PORTD como la salida para el módulo LCD 16x2
- Utilizar una frecuencia de 4MHz para el reloj del PIC
- Temporizar mediante un timer (TMR0 o TMR1) no use la sentencia delay_ms()
- Utilizar variables enteras de 8 bits para cada uno de los parámetros a utilizar (hora,
minutos, segundos, día, mes, año, hora y minutos de la alarma)
- Formatos de presentación: HH:MM:SS am – DD/MM/AAAA – HH:MM pm según
corresponda, la pantalla debe mostrar Hora/Fecha ; Hora/Alarma ; Ajuste (este a su
criterio, pero debe ser lógico y funcional)
- La selección de cada función se hará presionando 1 o 2 pulsadores
- Los valores deben poder modificarse y evitar valores herrados, se debe tomar en
cuenta los años bisiestos para el avance y ajuste del calendario
- La alarma y la fecha ajustadas debe ser gualdada en la EEPROM y leídas al inicio
- La cuenta del tiempo debe estar siempre activa, sin importar que se muestre
*/

// Configuración del PIC16F877A
#include <16F877A.h>
#include <lcd.c>
#fuses XT, NOWDT, NOLVP, NOPROTECT
#use delay(clock=4000000)


// Configuración de pines LCD en el puerto D
#define LCD_RS_PIN  PIN_D0
#define LCD_RW_PIN  PIN_D1
#define LCD_E_PIN   PIN_D2
#define LCD_DATA4   PIN_D4
#define LCD_DATA5   PIN_D5
#define LCD_DATA6   PIN_D6
#define LCD_DATA7   PIN_D7

// Definimos los botones en el puerto B
#define BTN_MODO   PIN_B0   
#define BTN_INC    PIN_B1   
#define BTN_DEC    PIN_B2   
#define BTN_OK     PIN_B3   
#define LED_AL     PIN_E0

// Variables para el tiempo, fecha y alarmas
int8 HORA=12, MIN=0, SEG=0, DIA=1, MES=1, ANO=26; // Hora inicial 12:00:00 del 01/01/2026
int8 AL_HORA=6, AL_MIN=0; // Alarma inicial a las 6:00 am
int1 ALARM_ON=0, ALARM_SIL=0, ajustar_minutos=0; // Flag para saber si la alarma esta sonando y si se esta ajustando los minutos de la alarma
int8 modo=0; // 0: Hora/Fecha, 1: Hora/Alarma, 2: Ajuste Alarma, 3: Ajuste Reloj, 4: Ajuste Fecha
int8 ajuste_f = 0; // Variable para saber que parte de la fecha se esta ajustando (dia, mes o año)
volatile int16 ticks=0; // Contador de ticks del timer para llevar el tiempo real
volatile int1 flag_1s=0; // Flag para avisar que paso 1 segundo y actualizar el reloj
int8 ah_disp;  // Para mostrar la hora de alarma en formato 12h
char a_ampm;   // Indica si la alarma es am o pm
int8 ajustar_reloj = 0;

// Interrupcion del timer para llevar el tiempo real
#int_TIMER0
void timer0_isr() {
   set_timer0(6);
   if(++ticks >= 125) { ticks=0; flag_1s=1; } // Cuando llega a 125 ticks paso 1 segundo
}

// Funcion para chequear si el año es bisiesto y ajustar febrero
int8 dias_febrero(int8 y) {
   if(((2000+y) % 4 == 0)) return 29;
   return 28;
}

// Guardamos los datos importantes para que no se borren al apagar
void guardar_config() {
   write_eeprom(0, AL_HORA);
   write_eeprom(1, AL_MIN);
   write_eeprom(2, DIA);
   write_eeprom(3, MES);
   write_eeprom(4, ANO);
   write_eeprom(5, (int8)ALARM_ON);
}

// Recuperamos los datos de la eeprom al encender el pic
void cargar_config() {
   AL_HORA  = read_eeprom(0);
   AL_MIN   = read_eeprom(1);
   DIA      = read_eeprom(2);
   MES      = read_eeprom(3);
   ANO      = read_eeprom(4);
   ALARM_ON = (int1)read_eeprom(5);
   
   // Si la memoria esta vacia o tiene datos raros, ponemos valores seguros
   if(ANO >= 100)  ANO = 26;  
   if(MES > 12 || MES == 0) MES = 2;
   if(DIA > 31 || DIA == 0) DIA = 3;
   
   if(AL_HORA > 23) AL_HORA = 12;
   if(AL_MIN > 59)  AL_MIN = 1;
}

void main() {
   setup_adc_ports(NO_ANALOGS); // Todo digital
   set_tris_b(0x0F); // Pines de botones como entrada
   set_tris_e(0x00); // Puerto E para el led de alarma
   output_low(LED_AL);
   
   cargar_config(); 
   
   setup_timer_0(RTCC_INTERNAL | RTCC_DIV_32);
   set_timer0(6);
   
   lcd_init();
   enable_interrupts(INT_TIMER0);
   enable_interrupts(GLOBAL);

   while(TRUE) {
      // Logica para avanzar el reloj cada vez que el timer avisa un segundo
      if(flag_1s) {
         flag_1s = 0;
         if(++SEG >= 60) {
            SEG = 0; ALARM_SIL = 0;
            if(++MIN >= 60) {
               MIN = 0;
               if(++HORA >= 24) { 
                  HORA = 0; DIA++;
                  int8 d_max = 31;
                  // Ajustamos los dias segun el mes actual
                  if(MES==4||MES==6||MES==9||MES==11) d_max=30;
                  else if(MES==2) d_max = dias_febrero(ANO); 
                  
                  if(DIA > d_max) { 
                     DIA=1; 
                     if(++MES>12){ MES=1; ANO++; } 
                  }
                  guardar_config(); // Auto guardado al cambiar de dia
               }
            }
         }
      }

      // Control del led de la alarma y silenciador
      if(ALARM_ON && !ALARM_SIL && HORA == AL_HORA && MIN == AL_MIN) {
         output_high(LED_AL);
         if(modo == 0 && input(BTN_OK)) { 
            ALARM_SIL = 1; output_low(LED_AL); 
          }
      } else { 
         output_low(LED_AL); 
      }

      // Cambio de menus con el boton modo
      if(input(BTN_MODO)) {
         delay_ms(20); // Filtro antirebote
         guardar_config(); 
         if(++modo > 4) modo = 0; 
         ajustar_minutos = 0; 
         lcd_putc("\f"); // Limpiar pantalla al cambiar
         while(input(BTN_MODO));
      }

      // Convertimos formato de 24h a 12h para mostrar am/pm
      int8 h_disp = HORA;
      char ampm = 'a';
      if(h_disp >= 12) { 
         ampm = 'p'; if(h_disp > 12) h_disp -= 12; 
      }
      if(h_disp == 0) h_disp = 12;

      switch(modo) {
         case 0: // Pantalla principal de Hora y Fecha
            lcd_gotoxy(1,1);
            printf(lcd_putc, "%02u:%02u:%02u %cm      ", h_disp, MIN, SEG, ampm);
            lcd_gotoxy(1,2);
            printf(lcd_putc, "%02u/%02u/20%02u      ", DIA, MES, ANO); 
            break;

         case 1: // Ver la alarma configurada
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

          case 2: // Menu para editar la alarma
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

         case 3: // Menu para poner la hora del reloj
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

         case 4: // Ajuste de fecha con cambio de mes inteligente
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
               if(ajuste_f == 0) { // Incrementar dia y saltar de mes si hace falta
                  if(++DIA > d_max_aj) { 
                     DIA = 1; 
                     if(++MES > 12) { MES = 1; if(++ANO > 99) ANO = 0; }
                  }
               }
               else if(ajuste_f == 1) { 
                  if(++MES > 12) MES = 1;
                  // Si el dia no existe en el nuevo mes, lo bajamos al maximo
                  int8 d_chk = 31;
                  if(MES==4||MES==6||MES==9||MES==11) d_chk = 30;
                  else if(MES==2) d_chk = dias_febrero(ANO);
                  if(DIA > d_chk) DIA = d_chk;
               }
               else { if(++ANO > 99) ANO = 0; }
               while(input(BTN_INC));
            }

            if(input(BTN_DEC)) {
               delay_ms(20);
               if(ajuste_f == 0) { // Retroceder dia y cambiar mes/año atras
                  if(DIA == 1) { 
                     if(--MES < 1) { MES = 12; if(ANO == 0) ANO = 99; else ANO--; }
                     int8 d_prev = 31;
                     if(MES==4||MES==6||MES==9||MES==11) d_prev = 30;
                     else if(MES==2) d_prev = dias_febrero(ANO);
                     DIA = d_prev;
                  } else DIA--;
               }
               else if(ajuste_f == 1) { 
                  if(MES == 1) MES = 12; else MES--;
                  int8 d_chk = 31;
                  if(MES==4||MES==6||MES==9||MES==11) d_chk = 30;
                  else if(MES==2) d_chk = dias_febrero(ANO);
                  if(DIA > d_chk) DIA = d_chk;
               }
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
      delay_ms(40); // Pequeña pausa para no refrescar el LCD tan rapido
   }
}