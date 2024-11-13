/*
 * ActExp3-Exp2-BalizaLED_PWM.c
 *
 * Created: 31/10/2024 09:41:37
 * Author : Ramiro Uffelmann
 */ 

// Definiciones -----------------------------------------------------------------------------------------
#define F_CPU 16000000UL		// 16 MHz Frecuencia del cristal.

// Pines usados por la librería lcd_328.h:
#define RS	eS_PORTB0			// Pin RS = PB0 (8) (Reset).
#define EN	eS_PORTB1			// Pin EN = PB1 (9) (Enable).
#define D4	eS_PORTB2			// Pin D4 = PB2 (10) (Data D4).
#define D5	eS_PORTB3			// Pin D5 = PB3 (11) (Data D5).
#define D6	eS_PORTB4			// Pin D6 = PB4 (12) (Data D6).
#define D7	eS_PORTB5			// Pin D7 = PB5 (13) (Data D7).

// Pines usados para los botones
#define P1 PD1					// Encender y apagar baliza.
#define P2 PD2					// Cambiar modo de operación.

// Macros y constantes ----------------------------------------------------------------------------------
#define	sbi(p,b)		p |= _BV(b)				//	Para setear el bit b de p.
#define	cbi(p,b)		p &= ~(_BV(b))			//	Para borrar el bit b de p.
#define	tbi(p,b)		p ^= _BV(b)				//	Para togglear el bit b de p.
#define is_high(p,b)	(p & _BV(b)) == _BV(b)	//	Para testear si el bit b de p es 1.
#define is_low(p,b)		(p & _BV(b)) == 0		//	Para testear si el bit b de p es 0.

#include <stdio.h>				// Cabecera estándar de E/S.
#include <avr/io.h>
#include <avr/interrupt.h>		// Contiene macros para uso de interrupciones.
#include "lcd_328.h"			// Contiene funciones para manejo del LCD.
#include <string.h>

// Declaración de Funciones -----------------------------------------------------------------------------
void confTIMERS();				// Función p/configuración de timers.
void confCONVAD();				// Función p/configuración del conversor AD.
void confPUERTOS();				// Función p/configuración de puertos.
void convertirAD();				// Función que realiza conversión AD.
void calcularMedia();

// Variables globales -------------
char buffer[16];				// Vector de caracteres que almacena string (16 = Nº de filas del LCD).
int P1_ant = 1, P1_act = 1, flag_P1 = 1;
int P2_ant = 1, P2_act = 1, flag_P2 = 1;
int balizaEncendida = 1;
int modoOperacion = 0;			// Modo 0 es normal, y modo 1 es intermitente.

volatile float periodo = 0;		// Periodo de intermitencia.
volatile int intensidad = 0;	// Intensidad, de 10 a 95 %.
volatile int CONV_k = 0;		// Muestra actual de la conversión AD.
volatile int CONV_km1 = 0;		// Una muestra anterior de la conversión AD.
volatile int CONV_km2 = 0;		// Dos muestras anteriores de la conversión AD.
volatile int CONV_km3 = 0;		// Tres muestras anteriores de la conversión AD.
volatile int CONV_prom = 0;		// Promedio de 4 muestras.
float frec;
volatile int mux = 0;			// mux alterna entre 0 y 1. 0=ADC0; 1=ADC1.
volatile int cont = 0;

const float mPeriodo = 9.0/1023.0;
const float mIntensidad = 85/1023.0;

ISR(TIMER2_OVF_vect) {				// Timer de 10 ms. Quita rebote de P1 y P2, encuestando.
	// Si actualmente el botón está en bajo, y hace 10 ms estaba en  
	// alto, es porque se acaba de pulsar dicho botón.
	
	// Quitar rebote de P1. Enciende y apaga la baliza.
	P1_act = is_high(PIND, P1);
	if(P1_ant && P1_act) flag_P1 = 1;
	if(!P1_act && P1_ant) {
		if (balizaEncendida)	{
			balizaEncendida = 0;
			TCCR0A &= ~(1<<COM0B1);
		}
		else {
			TCCR0A |= (1<<COM0B1);
			balizaEncendida = 1;
		}
	}
	P1_ant = P1_act;
	
	// Quitar rebote de P2. Cambia modo de operación.
	P2_act = is_high(PIND, P2);
	if(P2_ant && P2_act) flag_P2 = 1;
	if(!P2_act && P2_ant)	modoOperacion = !modoOperacion;
	P2_ant = P2_act;
	
	// Activar o desactivar salida PWM.
	if(modoOperacion & balizaEncendida) {	// Sólo encender y apagar LEDs si está en modo de intermitencia.
		cont += 1;
		if(cont <= (periodo / 2) * 1000 * 0.87){
			sbi(TCCR0A, COM0B1);	// Mantener encendido
		} else {
			cbi(TCCR0A, COM0B1);	// Mantener apagado.
		}
		if(cont >= periodo*1000) cont = 0;
	} else {
		cont = 0;
	}
	
	// Cada 10 ms, actualizar el duty cycle del PWM.
	OCR0B  = 82 * (intensidad/100.0);		// Valor del ciclo útil. Indicado en el multiplicador del periodo.
	TCNT2 = 6;	// Recarga valor de precarga, para que la temporización sea de 10 ms.
}

ISR(ADC_vect){
	CONV_k = ADC;			// Carga la muestra actual del registro ADCH:ADCL.
	if(!mux) {	// Se seleccionó ADC0
		periodo = mPeriodo * CONV_k + 1;
		sbi(ADMUX, MUX0);	// Seleccionar ADC1 para la próxima lectura.
	} else {	// Se seleccionó ADC1
		intensidad = mIntensidad * CONV_k + 10;
		cbi(ADMUX, MUX0);	// Seleccionar ADC0 para la próxima lectura.
	}
	
	mux = !mux;
}

ISR(TIMER0_COMPA_vect) {
	tbi(PORTD, PD0);
}

int main(void) {
	confPUERTOS();
	confTIMERS();
	confCONVAD();
	sei();				// Habilita las interrup. globalmente.
	Lcd4_Init();		// Inicializa el LCD.
	Lcd4_Clear();		// Borra el display.
	
    while (1) {

		Lcd4_Set_Cursor(1,0);
		if(modoOperacion)	sprintf(buffer, "MODO: %s", "INTERMITEN");
		else				sprintf(buffer, "MODO: %s", "NORMAL    ");
		Lcd4_Write_String(buffer);
		
		Lcd4_Set_Cursor(2,8);
		Lcd4_Write_String(" ");
		Lcd4_Set_Cursor(2,0);
		sprintf(buffer, "T: %.1f s I: %d%% ", periodo, intensidad);
		Lcd4_Write_String(buffer);
    }
}

// FUNCIONES ------------------
void confPUERTOS() {
	DDRD = 0b11111001;	// PD1 y PD2 como entrada, para P1 y P2, respectivamente.
	DDRB = 0xFF;		// Puerto B todo como salida, para conectar display.
	DDRC = 0x00;		// ADC0 y ADC1 como entradas, el resto como salida.
}

void confTIMERS() {
	// Timer para Fast PWM
	TCCR0A = 0b00100011;			// COM0A en 00 (pin OC0A desconectado), COM0B en 10 (PWM no invertido), WGM01 y WGM00 en 1 para Fast PWM.
	TCCR0B = 0b00001011;			// WGM02 en 1 para que el TOP sea OCR0A. CS01 y CS00 en 1 para prescaler = 64.
	OCR0A  = 82;					// TOP en 82.
	OCR0B  = 82 * 0.95;				// Valor del ciclo útil. Indicado en el multiplicador del periodo.
	TIMSK0 = 0x02;
	
	// Timer de 10 ms para quitar rebote de P1 y P2.
	TCCR2A = (0<<WGM01)|(0<<WGM00);	// Modo timer free-run.
	TCCR2B = (0<<WGM22)|(1<<CS22)
			 |(0<<CS21)|(0<<CS20);	// Modo timer y prescaler = 64.
	TIMSK2 = (1<<TOIE0);			// Habilito interrupción de este timer.
	TCNT2  = 6;						// VPC = 6
}

void confCONVAD() {					// Config. del conversor AD. Opera en modo free-running.
	DIDR0 = 0x03;					// Desconecta la parte digital del pin ADC0/PF0 y de ADC1.
	ADMUX = 0b01000000;
	ADCSRB = 0b00000011;			// Timer/Counter0 compare match A
	ADCSRA = 0b11101111;			// Habilita funcionamiento del ADC (bit ADEN=1) y prescaler en 128.
}