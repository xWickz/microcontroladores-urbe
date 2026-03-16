#include <16F877A.h>
#device ADC = 10
#fuses XT, NOWDT, NOLVP, NOPROTECT
#use delay(clock = 4000000)

#include <lcd.c>

// pines del lcd
#define LCD_RS_PIN PIN_D0
#define LCD_RW_PIN PIN_D1
#define LCD_E_PIN PIN_D2
#define LCD_DATA4 PIN_D4
#define LCD_DATA5 PIN_D5
#define LCD_DATA6 PIN_D6
#define LCD_DATA7 PIN_D7

#define USE_PORTB_KBD TRUE
#include <kbd.c>

#define LED_AL PIN_E0

// variables globales para el tiempo y los modos del menu
int8 HORA = 12, MIN = 0, SEG = 0, DIA = 1, MES = 3, ANO = 26;
int8 AL_HORA = 6, AL_MIN = 0;
int1 ALARM_ON = 0, ALARM_SIL = 0;
int8 modo = 0, sub_modo = 0, vista_principal = 0, valor_temporal = 0;

volatile int16 ticks = 0;
volatile int1 flag_1s = 0;
float temperatura = 0.0;

// interrupcion del timer0 para llevar el tiempo exacto
#int_TIMER0
void timer0_isr()
{
    set_timer0(6);
    if (++ticks >= 125)
    {
        ticks = 0;
        flag_1s = 1;
    }
}

// para que no se borre la hora y la alarma si se va la luz
void guardar_config()
{
    write_eeprom(0, AL_HORA);
    write_eeprom(1, AL_MIN);
    write_eeprom(2, DIA);
    write_eeprom(3, MES);
    write_eeprom(4, ANO);
    write_eeprom(5, (int8)ALARM_ON);
}

// cargamos todo lo que estaba guardado al iniciar el pic
void cargar_config()
{
    AL_HORA = read_eeprom(0);
    AL_MIN = read_eeprom(1);
    DIA = read_eeprom(2);
    MES = read_eeprom(3);
    ANO = read_eeprom(4);
    ALARM_ON = (int1)read_eeprom(5);

    // validaciones rapidas para no cargar basura de la memoria
    if (ANO >= 100)
        ANO = 26;
    if (MES > 12 || MES == 0)
        MES = 1;
    if (DIA > 31 || DIA == 0)
        DIA = 1;
    if (AL_HORA > 23)
        AL_HORA = 12;
}

// chequeo de bisiestos para febrero
int8 dias_febrero(int8 y)
{
    if (((2000 + y) % 4 == 0))
        return 29;
    return 28;
}

// funcion para saber cuantos dias tiene el mes actual
int8 obtener_dias_mes(int8 m, int8 y)
{
    if (m == 4 || m == 6 || m == 9 || m == 11)
        return 30;
    if (m == 2)
        return dias_febrero(y);
    return 31;
}

// aqui procesamos lo que el usuario toca en el keypad
void procesar_entrada(int8 *variable, int8 limite_max, char c)
{
    int8 num = c - '0';
    int16 nuevo_val = (int16)valor_temporal * 10 + num;
    if (nuevo_val <= limite_max)
    {
        valor_temporal = (int8)nuevo_val;
        *variable = valor_temporal;
    }
    else
    {
        valor_temporal = num;
        *variable = valor_temporal;
    }
}

void main()
{
    port_b_pullups(TRUE);
    setup_adc_ports(AN0_AN1_VSS_VREF);
    setup_adc(ADC_CLOCK_INTERNAL);
    set_adc_channel(0);

    lcd_init();
    kbd_init();
    cargar_config();

    setup_timer_0(RTCC_INTERNAL | RTCC_DIV_32);
    set_timer0(6);
    enable_interrupts(INT_TIMER0);
    enable_interrupts(GLOBAL);

    int8 refresco_lcd = 0;

    while (TRUE)
    {
        char tecla = kbd_getc();

        // cada segundo actualizamos el reloj y la temperatura
        if (flag_1s)
        {
            flag_1s = 0;
            temperatura = (float)read_adc() / 10.0;
            if (++SEG >= 60)
            {
                SEG = 0;
                ALARM_SIL = 0;
                if (++MIN >= 60)
                {
                    MIN = 0;
                    if (++HORA >= 24)
                    {
                        HORA = 0;
                        DIA++;
                        // control del calendario automatico
                        if (DIA > obtener_dias_mes(MES, ANO))
                        {
                            DIA = 1;
                            if (++MES > 12)
                            {
                                MES = 1;
                                ANO++;
                            }
                        }
                    }
                }
            }
        }

        // logica para prender el led de la alarma
        if (ALARM_ON && !ALARM_SIL && HORA == AL_HORA && MIN == AL_MIN)
        {
            output_high(LED_AL);
            if (tecla != 0)
            {
                ALARM_SIL = 1;
                output_low(LED_AL);
            }
        }
        else
        {
            output_low(LED_AL);
        }

        // aqui manejamos los menus con asterisco y numeral
        if (tecla != 0)
        {
            if (tecla == '*')
            {
                guardar_config();
                if (++modo > 4)
                    modo = 0;
                sub_modo = 0;
                valor_temporal = 0;
                lcd_putc("\f");
            }
            else if (tecla == '#')
            {
                if (modo == 0)
                {
                    vista_principal = !vista_principal;
                    lcd_putc("\f");
                }
                else
                {
                    if (++sub_modo > 2)
                        sub_modo = 0;
                    valor_temporal = 0;
                }
            }
            // edicion de los valores segun el modo donde estemos
            else if (tecla >= '0' && tecla <= '9' && modo != 0)
            {
                if (modo == 2)
                {
                    if (sub_modo == 0)
                        procesar_entrada(&AL_HORA, 23, tecla);
                    else if (sub_modo == 1)
                        procesar_entrada(&AL_MIN, 59, tecla);
                    else if (sub_modo == 2)
                    {
                        // switch am pm sumando 12 a la hora
                        if (AL_HORA < 12)
                            AL_HORA += 12;
                        else
                            AL_HORA -= 12;
                    }
                    ALARM_ON = 1;
                }
                else if (modo == 3)
                {
                    if (sub_modo == 0)
                        procesar_entrada(&HORA, 23, tecla);
                    else if (sub_modo == 1)
                        procesar_entrada(&MIN, 59, tecla);
                    else if (sub_modo == 2)
                    {
                        if (HORA < 12)
                            HORA += 12;
                        else
                            HORA -= 12;
                    }
                }
                else if (modo == 4)
                {
                    if (sub_modo == 0)
                    {
                        procesar_entrada(&DIA, obtener_dias_mes(MES, ANO), tecla);
                        if (DIA == 0)
                            DIA = 1;
                    }
                    else if (sub_modo == 1)
                    {
                        procesar_entrada(&MES, 12, tecla);
                        if (MES == 0)
                            MES = 1;
                    }
                    else if (sub_modo == 2)
                    {
                        procesar_entrada(&ANO, 99, tecla);
                    }
                }
            }
            refresco_lcd = 50;
        }

        // actualizacion de la pantalla lcd
        if (++refresco_lcd >= 50)
        {
            refresco_lcd = 0;
            int8 h_disp = HORA;
            char ampm = (HORA >= 12) ? 'p' : 'a';
            if (h_disp > 12)
                h_disp -= 12;
            if (h_disp == 0)
                h_disp = 12;

            switch (modo)
            {
            case 0: // vista normal con hora fecha y temp
                lcd_gotoxy(1, 1);
                printf(lcd_putc, "%02u:%02u:%02u %cm      ", h_disp, MIN, SEG, ampm);
                lcd_gotoxy(1, 2);
                if (vista_principal == 0)
                    printf(lcd_putc, "%02u/%02u/20%02u      ", DIA, MES, ANO);
                else
                    printf(lcd_putc, "TEMP: %2.1f %cC   ", temperatura, 223);
                break;

            case 1: // chequeo rapido de la alarma
                lcd_gotoxy(1, 1);
                printf(lcd_putc, "HORA: %02u:%02u %cm", h_disp, MIN, ampm);
                lcd_gotoxy(1, 2);
                int8 ah_m = (AL_HORA >= 12) ? ((AL_HORA > 12) ? AL_HORA - 12 : 12) : ((AL_HORA == 0) ? 12 : AL_HORA);
                printf(lcd_putc, "ALARMA: %02u:%02u %cm ", ah_m, AL_MIN, (AL_HORA >= 12) ? 'p' : 'a');
                break;

            case 2: // editar la alarma
                lcd_gotoxy(1, 1);
                if (sub_modo == 0)
                    lcd_putc("EDIT: HORA ALM  ");
                else if (sub_modo == 1)
                    lcd_putc("EDIT: MIN ALM   ");
                else
                    lcd_putc("EDIT: AM-PM ALM ");

                int8 ah_disp = (AL_HORA >= 12) ? ((AL_HORA > 12) ? AL_HORA - 12 : 12) : ((AL_HORA == 0) ? 12 : AL_HORA);
                char ap_alm = (AL_HORA >= 12) ? 'p' : 'a';

                lcd_gotoxy(1, 2);
                printf(lcd_putc, "SET: %02u:%02u %cm      ", ah_disp, AL_MIN, ap_alm);
                break;

            case 3: // editar el reloj principal
                lcd_gotoxy(1, 1);
                if (sub_modo == 0)
                    lcd_putc("EDIT: HORA RELOJ");
                else if (sub_modo == 1)
                    lcd_putc("EDIT: MIN RELOJ ");
                else
                    lcd_putc("EDIT: AM-PM RELO");

                lcd_gotoxy(1, 2);
                printf(lcd_putc, "HORA: %02u:%02u %cm    ", h_disp, MIN, ampm);
                break;

            case 4: // editar la fecha
                lcd_gotoxy(1, 1);
                if (sub_modo == 0)
                    lcd_putc("EDIT: DIA        ");
                else if (sub_modo == 1)
                    lcd_putc("EDIT: MES        ");
                else
                    lcd_putc("EDIT: ANO        ");
                lcd_gotoxy(1, 2);
                printf(lcd_putc, "F: %02u/%02u/20%02u      ", DIA, MES, ANO);
                break;
            }
        }
        delay_ms(1);
    }
}