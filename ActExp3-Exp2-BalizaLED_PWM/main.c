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
    while (1) {
    }
}

// FUNCIONES ------------------
void confPuertos() {
	DDRD = 0xFF;
}

void confTIMERS(){
	// Timer para Fast PWM
	TCCR0A = 0b00100011;			// COM0A en 00 (pin OC0A desconectado), COM0B en 10 (PWM no invertido), WGM01 y WGM00 en 1 para Fast PWM.
	TCCR0B = 0b00001011;			// WGM02 en 1 para que el TOP sea OCR0A. CS01 y CS00 en 1 para prescaler = 64.
	OCR0A  = 82;					// TOP en 82.
	OCR0B  = 41;					// Valor del ciclo útil.
}