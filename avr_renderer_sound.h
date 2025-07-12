#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "sounds.h"
#include "build/sounds/dzin.h"
#include "build/sounds/bump.h"
#include "build/sounds/spoon.h"
#include "build/sounds/spoon2.h"
#include "build/sounds/click.h"

#define SAMPLE_RATE 8000
#define PWM_TOP 255

void setupTimer2_PWM() {
    DDRD |= (1 << PD3);
    TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << CS20);
    OCR2B = 0;
}

void setupTimer0_Interrupt() {
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01);
    OCR0A = (F_CPU / 8 / SAMPLE_RATE) - 1;
    TIMSK0 = (1 << OCIE0A);
}

Sound current_sound;
volatile uint16_t current_sample = 0;

ISR(TIMER0_COMPA_vect) {
    if (current_sample < current_sound.length) {
        uint8_t sample = pgm_read_byte(&current_sound.start[current_sample++]);
        OCR2B = sample;
    } else {
        TIMSK0 &= ~(1 << OCIE0A);
        OCR2B = 0;
    }
}

void renderer_play_sound(Sound sound) {
    current_sound = sound;
    current_sample = 0;
    TIMSK0 |= (1 << OCIE0A);
}

void renderer_init_sound() {
    setupTimer2_PWM();
    setupTimer0_Interrupt();
    sei();
}

