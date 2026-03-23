#include <16F877A.h>
#device ADC = 10
#fuses XT, NOWDT, NOPROTECT, NOLVP
#use delay(clock = 4000000)

// --- CONFIGURACIÓN LCD ---
#include <lcd.c>
#define LCD_RS_PIN PIN_D0
#define LCD_RW_PIN PIN_D1
#define LCD_E_PIN PIN_D2
#define LCD_DATA4 PIN_D4
#define LCD_DATA5 PIN_D5
#define LCD_DATA6 PIN_D6
#define LCD_DATA7 PIN_D7

#include <internal_eeprom.c>
#include <stdlib.h>

// --- TECLADO MATRICIAL 4x4 ---
char const KEYS[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

char get_kbd()
{
    int8 f, c;
    for (f = 0; f < 4; f++)
    {
        output_b(0xFF ^ (1 << f));
        delay_us(10);
        for (c = 0; c < 4; c++)
        {
            if (!input(PIN_B4 + c))
            {
                delay_ms(10); // Antirebote
                while (!input(PIN_B4 + c))
                    ;
                return KEYS[f][c];
            }
        }
    }
    return 0;
}

// --- VARIABLES GLOBALES ---
float temp = 0.0;
float al_alta, al_baja, setpoint;
volatile int16 ticks = 0;
int8 duty_contraste = 128; // 50% inicial

// --- PROTOTIPOS ---
void leer_sensores();
void actualizar_alarmas();
float ingresar_valor(int8 modo);

#int_TIMER0
void timer0_isr()
{
    ticks++;
    set_timer0(6); // Ajuste para desborde exacto
}

void main()
{
    // Configuracion ADC
    setup_adc_ports(AN0_AN1_VSS_VREF);
    setup_adc(ADC_CLOCK_INTERNAL);

    // PWMs
    setup_timer_2(T2_DIV_BY_16, 255, 1);
    setup_ccp1(CCP_PWM);
    setup_ccp2(CCP_PWM);
    set_pwm2_duty(duty_contraste);

    // Timer0 para refresco sin delay_ms
    setup_timer_0(RTCC_INTERNAL | RTCC_DIV_8);
    set_timer0(6);
    enable_interrupts(INT_TIMER0);
    enable_interrupts(GLOBAL);

    // Puertos e Inicio
    set_tris_a(0x09); // RA0 y RA3 entradas
    set_tris_e(0x00); // Puerto E para LEDs de alarma
    port_b_pullups(TRUE);
    lcd_init();

    // Cargar datos de EEPROM
    al_alta = read_float_eeprom(0);
    al_baja = read_float_eeprom(4);
    setpoint = read_float_eeprom(8);

    // Valores defecto
    if (al_alta > 150 || al_alta < 0)
        al_alta = 50.0;
    if (al_baja > 150 || al_baja < 0)
        al_baja = 15.0;
    if (setpoint > 150 || setpoint < 0)
        setpoint = 30.0;

    while (TRUE)
    {
        // Tarea cada ~500ms usando el Timer0
        if (ticks >= 250)
        {
            leer_sensores();
            actualizar_alarmas();

            lcd_gotoxy(1, 1);
            printf(lcd_putc, "Temp:%2.1f C     ", temp);
            lcd_gotoxy(1, 2);
            printf(lcd_putc, "S%2.0f H%2.0f L%2.0f", setpoint, al_alta, al_baja);
            ticks = 0;
        }

        // Escaneo de teclado
        char k = get_kbd();
        if (k != 0)
        {
            if (k == 'A')
            {
                al_alta = ingresar_valor(1);
                write_float_eeprom(0, al_alta);
            }
            if (k == 'B')
            {
                al_baja = ingresar_valor(2);
                write_float_eeprom(4, al_baja);
            }
            if (k == 'C')
            {
                setpoint = ingresar_valor(3);
                write_float_eeprom(8, setpoint);
            }

            // Control digital de contraste
            if (k == '3')
            {
                if (duty_contraste < 250)
                    duty_contraste += 5;
                set_pwm2_duty(duty_contraste);
            }
            if (k == '6')
            {
                if (duty_contraste > 5)
                    duty_contraste -= 5;
                set_pwm2_duty(duty_contraste);
            }

            lcd_putc('\f'); // Limpiar despues de ajustar
        }
    }
}

void leer_sensores()
{
    set_adc_channel(0);
    delay_us(20);

    long valor = read_adc();
    temp = valor;
    temp = temp / 10;

    float salida = 51.0 + (temp * 2.04);
    if (salida > 255)
        salida = 255;
    if (salida < 51)
        salida = 51;
    set_pwm1_duty((int16)salida);
}

void actualizar_alarmas()
{
    // Salidas por Puerto E para evitar conflictos analógicos
    output_bit(PIN_E0, (temp <= al_baja));  // Prende si es menor a 20
    output_bit(PIN_E2, (temp >= setpoint)); // Prende si es mayor a 30
    output_bit(PIN_E1, (temp >= al_alta));  // Prende si es mayor a 50
}

float ingresar_valor(int8 modo)
{
    char k;
    char data[4] = {' ', ' ', ' ', '\0'};
    int8 i = 0;

    lcd_putc('\f');
    lcd_gotoxy(1, 1);
    if (modo == 1)
        printf(lcd_putc, "Alarma Alta:");
    if (modo == 2)
        printf(lcd_putc, "Alarma Baja:");
    if (modo == 3)
        printf(lcd_putc, "Set Point:");

    while (TRUE)
    {
        k = get_kbd();
        if (k >= '0' && k <= '9' && i < 3)
        {
            data[i] = k;
            lcd_gotoxy(i + 1, 2);
            lcd_putc(k);
            i++;
        }
        if (k == 'D' && i > 0)
            return (float)atof(data); // 'D' como Enter
        if (k == '*')
            return 0.0; // Cancelar
    }
}