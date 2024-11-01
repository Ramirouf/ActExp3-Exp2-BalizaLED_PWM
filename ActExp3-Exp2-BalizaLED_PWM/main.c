/*
 * ActExp3-Exp2-BalizaLED_PWM.c
 *
 * Created: 31/10/2024 09:41:37
 * Author : Ramiro Uffelmann
 */ 

#include <avr/io.h>


int main(void) {
	confTIMERS();
	confPuertos();
	confCONVAD();
    while (1) {
    }
}

// FUNCIONES ------------------
void confPuertos() {
	DDRD = 0xFF;
}

void confTIMERS() {
	// Timer para Fast PWM
	TCCR0A = 0b00100011;			// COM0A en 00 (pin OC0A desconectado), COM0B en 10 (PWM no invertido), WGM01 y WGM00 en 1 para Fast PWM.
	TCCR0B = 0b00001011;			// WGM02 en 1 para que el TOP sea OCR0A. CS01 y CS00 en 1 para prescaler = 64.
	OCR0A  = 82;					// TOP en 82.
	OCR0B  = 82 * 0.75;				// Valor del ciclo útil. Indicado en el multiplicador del periodo.
}

void confCONVAD() {				// Config. del conversor AD. Opera en modo free-running.
	DIDR0 &= ~((1<<ADC0D));				// Desconecta la parte digital del pin ADC0/PF0.
	ADMUX &= ~((0<<REFS1)|(1<<REFS0)|(0<<ADLAR));		// Config. la ref. de tensión tomada del pin AVCC (placa Arduino AVCC = Vcc = 5V).
								// Conversión AD de 10 bits (ADLAR = 0) y con el Multiplexor selecciona canal 0 (ADC0/PF0).
	ADCSRB = 0x00;				// Modo free-running.
	ADCSRA = 0x87;				// Habilita funcionamiento del ADC (bit ADEN=1) y prescaler en 128.
}