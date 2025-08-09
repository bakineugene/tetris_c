#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "../screen.h"
#include "../colours.h"
#include "../renderer.h"
#include "../events.h"
#include "sound.h"

#define START_DATA_TRANSACTION()    PORTB &= ~(1 << PB2)
#define END_DATA_TRANSACTION()  PORTB |= (1 << PB2)

#define RENDERER_EVENT_DELAY 80
#define NUM_DEVICES 6
#define MATRIX_SIDE 8

volatile uint8_t dim_border = 0;
volatile uint8_t dim_counter = 0;

void timer1_init(void) {
    TCCR1B |= (1 << WGM12);
    OCR1A = 32;
    TIMSK1 |= (1 << OCIE1A);
    TCCR1B |= (1 << CS12) | (1 << CS10);
}

ISR(TIMER1_COMPA_vect) {
    if (dim_counter++ > 10) dim_counter = 0;
    if (dim_counter > 9) dim_border = 0; else dim_border = 1;
}

void ADC_Init() {
    ADMUX = (1 << REFS0) | (0 << ADLAR);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    DIDR0 = (1 << ADC0D);
}

uint16_t ADC_Read(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

void initSPI(void) {
  DDRB |= (1 << PB2);	    // SS (CS - Chip Select)
  DDRB |= (1 << PB3);       // MOSI (DATA OUT)
  DDRB |= (1 << PB5);       // SCK (CLK)

  SPCR |= (1 << MSTR);      // Clockmaster 
  SPCR |= (1 << SPE);       // Enable SPI

  END_DATA_TRANSACTION();
}

void writeByte(uint8_t byte) {
  SPDR = byte;                      // SPI starts sending immediately  
  while(!(SPSR & (1 << SPIF)));     // Loop until complete bit set
}

void writeCommand(uint8_t command, uint8_t data) {
  writeByte(command);
  writeByte(data);
}

void initMatrix() {
	// Set display brighness
	START_DATA_TRANSACTION();
	for(int i = 0; i < NUM_DEVICES; i++) writeCommand(0x0A, 0x07);
	END_DATA_TRANSACTION();

	// Set display refresh
	START_DATA_TRANSACTION();
	for(int i = 0; i < NUM_DEVICES; i++) writeCommand(0x0B, 0x07);
	END_DATA_TRANSACTION();

	// Turn on the display
	START_DATA_TRANSACTION();
	for(int i = 0; i < NUM_DEVICES; i++) writeCommand(0x0C, 0x01);
	END_DATA_TRANSACTION();

	// Disable Display-Test
	START_DATA_TRANSACTION();
	for(int i = 0; i < NUM_DEVICES; i++) writeCommand(0x0F, 0x00);
	END_DATA_TRANSACTION();
}

uint8_t buffer[NUM_DEVICES * MATRIX_SIDE];
uint8_t dim_buffer[NUM_DEVICES * MATRIX_SIDE];

void initBuffer() {
	for(int i = 0; i < NUM_DEVICES * MATRIX_SIDE; ++i) buffer[i] = 0x00;
}       

void sendBuffer() {
    for(int column = 1; column < 9; ++column) {
        START_DATA_TRANSACTION();
        if (dim_border == 0) {
            for(int i = 0; i < NUM_DEVICES; ++i) writeCommand(column, buffer[column + i * MATRIX_SIDE - 1]);
        } else {
            for(int i = 0; i < NUM_DEVICES; ++i) writeCommand(column, dim_buffer[column + i * MATRIX_SIDE - 1]);
        }
        END_DATA_TRANSACTION();
    }
}

int last_event = RENDERER_EVENT_DELAY;
enum Event renderer_get_event() {
    if (last_event <= 0) {
        last_event = RENDERER_EVENT_DELAY;
        
        uint16_t adc_value = ADC_Read(0);

        if (adc_value <= 100) {
            return EVENT_EMPTY;
        } else if (adc_value < 450 && adc_value > 100) {
            return EVENT_SPACE;
        } else if (adc_value >= 450 && adc_value < 600) {
            return EVENT_RIGHT;
        } else if (adc_value >= 600 && adc_value < 750) {
            return EVENT_DOWN;
        } else if (adc_value >= 750 && adc_value < 900) {
            return EVENT_UP;
        } else {
            return EVENT_LEFT;
        }
    }

    return EVENT_EMPTY;
}

int renderer_init(void) {
    initSPI();
    ADC_Init();
    initMatrix();
    initBuffer();
    sendBuffer();
    renderer_init_sound();
    timer1_init();
    return 0;
}

const uint8_t bit_set = 0b00000001;
void renderer_render(uint8_t *screen) {
    for (int i = 0; i < NUM_DEVICES; ++i) {
        uint8_t bit = 0b00000000;
        uint8_t dim_bit = 0b00000000;
        char x;
        char y;
        for (int column = 0; column < MATRIX_SIDE; ++column) {
            for (int row = 0; row < MATRIX_SIDE; ++row) {
                bit = bit << 1;
                dim_bit = dim_bit << 1;

                if (i == 5) {
                    x = column;
                    y = row;
                }
                if (i == 2) {
                    x = column + MATRIX_SIDE;
                    y = row;
                }
                if (i == 1) {
                    x = column + MATRIX_SIDE;
                    y = row + MATRIX_SIDE;
                }
                if (i == 4) {
                    x = column;
                    y = row + MATRIX_SIDE;
                }
                if (i == 0) {
                    x = MATRIX_SIDE * 2 - column - 1;
                    y = MATRIX_SIDE * 3 - row - 1;
                }
                if (i == 3) {
                    x = MATRIX_SIDE - column - 1;
                    y = MATRIX_SIDE * 3 - row - 1;
                }

                if (*(screen + x * SCREEN_Y + y) != 0) {
                    bit |= bit_set;
                    dim_bit |= bit_set;
                }
                if (*(screen + x * SCREEN_Y + y) == COLOUR_WALL) {
                    dim_bit = dim_bit &~ bit_set;
                }
            }
            buffer[NUM_DEVICES * MATRIX_SIDE - i * MATRIX_SIDE - 1 - column] = bit;
            dim_buffer[NUM_DEVICES * MATRIX_SIDE - i * MATRIX_SIDE - 1 - column] = dim_bit;
        }
    }
    sendBuffer();	
}

void renderer_delay(int delay) {
    last_event -= delay;
    while (delay > 0) {
        sendBuffer();
        delay = delay - 1;
        _delay_ms(1);
    }
}

void renderer_destroy() {

}

Renderer new_renderer() {
    Renderer renderer;

    renderer.get_event = renderer_get_event;
    renderer.init = renderer_init;
    renderer.render = renderer_render;
    renderer.delay = renderer_delay;
    renderer.destroy = renderer_destroy;
    renderer.play_sound = renderer_play_sound;

    return renderer;
}

