#include "events.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "avr_renderer_sound.h"

#define START_DATA_TRANSACTION()    PORTB &= ~(1 << PB2)
#define END_DATA_TRANSACTION()  PORTB |= (1 << PB2)

#define RENDERER_EVENT_DELAY 80
#define NUM_DEVICES 6

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

uint8_t buffer [NUM_DEVICES * 8];	

void initBuffer() {
	for(int i = 0; i < NUM_DEVICES * 8; ++i) buffer[i] = 0x00;
}       

void sendBuffer() {   
   for(int column = 1; column < 9; ++column) {
       START_DATA_TRANSACTION();
       for(int i = 0; i < NUM_DEVICES; ++i) writeCommand(column, buffer[column + i*8 - 1]);
       END_DATA_TRANSACTION();
   }
}

/* variables */

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
    return 0;
}

const uint8_t bit_set = 0b00000001;
void renderer_render(char *a) {
    for (int i = 0; i < NUM_DEVICES; ++i) {
        if (i == 5) {
            uint8_t bit = 0b00000000;
            for (int column = 0; column < 8; ++column) {
                char x = column;

                for (int row = 0; row < 8; ++row) {
                    bit = bit << 1;
                    char y = row;
                    if (*(a + x * SCREEN_Y + y) != 0) {
                        bit |= bit_set;
                    }
                }
                buffer[NUM_DEVICES * 8 - i * 8 - 1 - column] = bit;
            }
        }
        if (i == 2) {
            uint8_t bit = 0b00000000;
            for (int column = 0; column < 8; ++column) {
                char x = column + 8;

                for (int row = 0; row < 8; ++row) {
                    bit = bit << 1;
                    char y = row;
                    if (*(a + x * SCREEN_Y + y) != 0) {
                        bit |= bit_set;
                    }
                }
                buffer[NUM_DEVICES * 8 - i * 8 - 1 - column] = bit;
            }
        }
        if (i == 1) {
            uint8_t bit = 0b00000000;
            for (int column = 0; column < 8; ++column) {
                char x = column + 8;

                for (int row = 0; row < 8; ++row) {
                    bit = bit << 1;

                    char y = row + 8;
                    if (*(a + x * SCREEN_Y + y) != 0) {
                        bit |= bit_set;
                    }
                }
                buffer[NUM_DEVICES * 8 - i * 8 - 1 - column] = bit;
            }
        }
        if (i == 4) {
            uint8_t bit = 0b00000000;
            for (int column = 0; column < 8; ++column) {
                char x = column;

                for (int row = 0; row < 8; ++row) {
                    bit = bit << 1;
                    char y = row + 8;
                    if (*(a + x * SCREEN_Y + y) != 0) {
                        bit |= bit_set;
                    }
                }
                buffer[NUM_DEVICES * 8 - i * 8 - 1 - column] = bit;
            }
        }
        if (i == 0) {
            uint8_t bit = 0b00000000;
            for (int column = 0; column < 8; ++column) {
                char x = 8 + 8 - column - 1;

                for (int row = 0; row < 8; ++row) {
                    bit = bit << 1;
                    char y = 8 + 8 + 7 - row;
                    if (*(a + x * SCREEN_Y + y) != 0) {
                        bit |= bit_set;
                    }
                }
                buffer[NUM_DEVICES * 8 - i * 8 - 1 - column] = bit;
            }
        }
        if (i == 3) {
            uint8_t bit = 0b00000000;
            for (int column = 0; column < 8; ++column) {
                char x = 8 - column - 1;

                for (int row = 0; row < 8; ++row) {
                    bit = bit << 1;
                    char y = 8 + 8 + 7 - row;
                    if (*(a + x * SCREEN_Y + y) != 0) {
                        bit |= bit_set;
                    }
                }
                buffer[NUM_DEVICES * 8 - i * 8 - 1 - column] = bit;
            }
        }
    }
    sendBuffer();	
}

void renderer_delay(int delay) {
    last_event -= delay;
    while (delay > 0) {
        delay = delay - 10;
        _delay_ms(10);
    }
}

void renderer_destroy() {

}
