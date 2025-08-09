#include <stdlib.h>

#include "colours.h"

#define NUMBER_OF_COLOURS 7
enum Colour colours[NUMBER_OF_COLOURS] = {
    COLOUR_RED,
    COLOUR_ORANGE,
    COLOUR_YELLOW,
    COLOUR_GREEN,
    COLOUR_BLUE,
    COLOUR_DEEP_BLUE,
    COLOUR_VIOLET
}; 

enum Colour get_random_colour() {
    return colours[rand() % NUMBER_OF_COLOURS];
}

