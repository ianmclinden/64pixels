/* -----------------------------------------------------------------------
 * Title:    8x8 LED dot matrix animations
 * Author:   Alexander Weber alex@tinkerlog.com
 * Date:     21.12.2008
 * Hardware: ATtiny2313
 * Software: AVRMacPack
 *
 * Edited: 06.18.2016 Ian McLinden
 */

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "font.h"

// Change these values to adjust scroll speeds and animation iterations
#define ANIMATION_SCROLL_SPEED 80  // how fast to scroll the animations
#define TEXT_SCROLL_SPEED 120      // how fast to scrill the text
#define REPEAT_ANIMATION 5        // how often to repeat the animation if in cycling mode
#define REPEAT_TEXT 1              // how often to repeat the text if in cycling mode

#define CYCLE 0                    // 1 YES

// How to add a new message:
// * add the new message (only upper case, see font.h)
// * adjust MAX_MESSAGES
// * add the new message to messages
// NOTE: messages may not be longer than 59 chars. Otherwise they will not fit in the buffer.
//                                "123456789012345678901234567890123456789012345678901234567890"
const char message_00[] PROGMEM = "   WTF!?! ";
const char message_01[] PROGMEM = "   I AM NO BOMB! ";
const char message_02[] PROGMEM = "   5   4   3   2   1  ...  BOOM! ";
const char message_03[] PROGMEM = "   I'M SORRY DAVE, I'M AFRAID I CAN'T DO THAT.  ";
const char message_04[] PROGMEM = "   BUY ME A SOLDERING STATION ";
const char message_05[] PROGMEM = "   MAKE STUFF ";
const char message_06[] PROGMEM = "   IF YOU CAN'T OPEN IT, YOU DON'T OWN IT ";
const char message_07[] PROGMEM = "   1337 3L3X7RON!C5 !1!! ";
const char message_08[] PROGMEM = "   MY KUNG FU IS BETTER THAN YOURS ";
const char message_09[] PROGMEM = "   SUDO MAKE ME A SANDWICH ";
const char message_10[] PROGMEM = "   ZOMBIES AHEAD ";
const char message_11[] PROGMEM = "   HTTPS://GITHUB.COM/IANMCLINDEN ";

#define MAX_MESSAGES 12
PGM_P const messages[] PROGMEM = {
    message_00
    ,message_01
    ,message_02
    ,message_03
    ,message_04
    ,message_05
    ,message_06
    ,message_07
    ,message_08
    ,message_09
    ,message_10
    ,message_11
};

#define MAX_ANIMATIONS 4
const uint8_t sprite_00[8] PROGMEM =
{
    0x18,    // ___XX___
    0x3C,    // __XXXX__
    0x7E,    // _XXXXXX_
    0xDB,    // X_XXXX_X
    0xFF,    // XXXXXXXX
    0x24,    // __X__X__
    0x5A,    // _X_XX_X_
    0xA5     // X_X__X_X
};

const uint8_t sprite_01[8] PROGMEM =
{
    0x18,    // ___XX___
    0x3C,    // __XXXX__
    0x7E,    // _XXXXXX_
    0xDB,    // X_XXXX_X
    0xFF,    // XXXXXXXX
    0x24,    // __X__X__
    0x42,    // _X____X_
    0x24     // __X__X__
};

const uint8_t sprite_02[8] PROGMEM =
{
    0x00,    // ________
    0x00,    // ________
    0x14,    // ___X_X__
    0x3E,    // __XXXXX_
    0x3E,    // __XXXXX_
    0x1C,    // ___XXX__
    0x08,    // ____X___
    0x00     // ________
};

const uint8_t sprite_03[8] PROGMEM =
{
    0x00,    // ________
    0x66,    // _XX__XX_
    0xFF,    // XXXXXXXX
    0xFF,    // XXXXXXXX
    0xFF,    // XXXXXXXX
    0x7E,    // _XXXXXX_
    0x3C,    // __XXXX__
    0x18     // ___XX___
};


const uint8_t sprite_04[8] PROGMEM =
{
    0x3c,    // __XXXX__
    0x42,    // _X____X_
    0xA5,    // X_X__X_X
    0x81,    // X______X
    0xA5,    // X_X__X_X
    0x99,    // X__XX__X
    0x42,    // _X____X_
    0x3c     // __XXXX__
};

const uint8_t sprite_05[8] PROGMEM =
{
    0x3c,    // __XXXX__
    0x42,    // _X____X_
    0xA1,    // X_X____X
    0x81,    // X______X
    0xA5,    // X_X__X_X
    0x99,    // X__XX__X
    0x42,    // _X____X_
    0x3c     // __XXXX__
};

const uint8_t sprite_06[8] PROGMEM =
{
    0x24,    // __X__X__
    0x7E,    // _XXXXXX_
    0xDB,    // XX_XX_XX
    0xFF,    // XXXXXXXX
    0xA5,    // X_X__X_X
    0x99,    // X__XX__X
    0x81,    // X______X
    0xC3     // XX____XX
};

const uint8_t sprite_07[8] PROGMEM =
{
    0x24,    // __X__X__
    0x18,    // ___XX___
    0x7E,    // X_XXXX_X
    0xDB,    // XX_XX_XX
    0xFF,    // XXXXXXXX
    0xDB,    // X_XXXX_X
    0x99,    // X__XX__X
    0xC3     // XX____XX
};



uint8_t mode_ee EEMEM = 0;                      // stores the mode in eeprom
static uint8_t screen_mem[8];                   // screen memory
static uint8_t active_row;                      // active row
static uint8_t buffer[60];                      // stores the active message or sprite
static uint8_t message_ptr = 0;                 // points to the active char in the message
static uint8_t message_displayed = 0;           // how often has the message been displayed?
static uint8_t active_char = 0;                 // stores the active char
static uint8_t message_length = 0;              // stores the length of the active message
static uint8_t char_ptr = 0;                    // points to the active col in the char
static uint8_t char_length = 0;                 // stores the length of the active char
static volatile uint16_t counter = 0;           // used for delay function

// prototypes
void delay_ms(uint16_t delay);
void copy_to_display(int8_t x, int8_t y, uint8_t sprite[]);
void display_active_row(void);
void show_char();
void clear_screen(void);
void copy_to_buffer(const uint8_t sprite[8]);
void scroll_animation(const uint8_t sprite_1[], const uint8_t sprite_2[]);
void pulse_animation(const uint8_t sprite_1[8], const uint8_t sprite_2[8]);

/*
 * ISR TIMER0_OVF_vect
 * Handles overflow interrupts of timer 0.
 *
 * 4MHz
 * ----
 * Prescaler 8 ==> 1953.1 Hz
 * Complete display = 244 Hz
 *
 */
ISR(TIMER0_OVF_vect) {
    display_active_row();
    counter++;
}


/*
 * copy_to_display
 * Copies sprite data to the screen memory at the given position.
 */
                  // 0 < 8,    0
void copy_to_display(int8_t x, int8_t y, uint8_t sprite[8]) {
    int8_t i, t;
    uint8_t row;
    for (i = 0; i < 8; i++) {   // 0
        t = i-y;    // 0
        row = ((t >= 0) && (t < 8)) ? sprite[t] : 0x00;
        row = (x >= 0) ? (row >> x) : (row << -x);
        screen_mem[i] = row;
    }
}



/*
 * display_active_col
 * Deactivates the active column and displays the next one.
 * Data is read from screen_mem.
 *
 * HSN-1088AS common anode         |
 *    B0B1B5A1B6D2D3B3      +-----+
 * PA0 o o o o o o o o       |     |
 * PD4 o o o o o o o o      _+_    |
 * PD1 o o o o o o o o      \ /    |
 * PD5 o o o o o o o o     __V__   |
 * PB4 o o o o o o o o       |     |
 * PD0 o o o o o o o o    ---+-----C---
 * PB2 o o o o o o o o             |
 * PB7 o o o o o o o o
 *
 */
void display_active_row(void) {
    
    uint8_t row;
    
    // shut down all rows and columns
    PORTA = (1 << PA0) | (0 << PA1);
    PORTB = (0 << PB0) | (0 << PB1) | (1 << PB2) | (0 << PB3) |
            (1 << PB4) | (0 << PB5) | (0 << PB6) | (1 << PB7);
    PORTD = (1 << PD0) | (1 << PD1) | (0 << PD2) | (0 << PD3) |
            (1 << PD4) | (1 << PD5) | (0 << PD6);
    
    // next row
    active_row = (active_row+1) % 8;
    row = screen_mem[active_row];
    
    // output all columns, switch leds on.
    // column 1
    if ((row & 0x80) == 0x80) {
        PORTB |= (1 << PB0);
    }
    // column 2
    if ((row & 0x40) == 0x40) {
        PORTB |= (1 << PB1);
    }
    // column 3
    if ((row & 0x20) == 0x20) {
        PORTB |= (1 << PB5);
    }
    // column 4
    if ((row & 0x10) == 0x10) {
        PORTA |= (1 << PA1);
    }
    // column 5
    if ((row & 0x08) == 0x08) {
        PORTB |= (1 << PB6);
    }
    // column 6
    if ((row & 0x04) == 0x04) {
        PORTD |= (1 << PD2);
    }
    // column 7
    if ((row & 0x02) == 0x02) {
        PORTD |= (1 << PD3);
    }
    // column 8
    if ((row & 0x01) == 0x01) {
        PORTB |= (1 << PB3);
    }
    
    // activate row
    switch (active_row) {
        case 0:
            PORTA &= ~(1 << PA0);
            break;
        case 1:
            PORTD &= ~(1 << PD4);
            break;
        case 2:
            PORTD &= ~(1 << PD1);
            break;
        case 3:
            PORTD &= ~(1 << PD5);
            break;
        case 4:
            PORTB &= ~(1 << PB4);
            break;
        case 5:
            PORTD &= ~(1 << PD0);
            break;
        case 6:
            PORTB &= ~(1 << PB2);
            break;
        case 7:
            PORTB &= ~(1 << PB7);
            break;
    }
}


/*
 * show_char
 * Displays the actual message.
 * Scrolls the screen to the left and draws new pixels on the right.
 */
void show_char() {
    uint8_t i;
    uint8_t b;
    
    // blit the screen to the left
    for (i = 0; i < 8; i++) {
        screen_mem[i] <<= 1;
    }
    // advance a char if needed
    if (char_ptr == char_length) {
        message_ptr++;
        if (message_ptr == message_length) {
            message_ptr = 0;
            message_displayed++;
        }
        active_char = buffer[message_ptr] - CHAR_OFFSET;
        char_length = pgm_read_byte(&font[active_char * 4 + 3]);
        char_ptr = 0;
        return; // this makes the space between two chars
    }
    // read pixels for current column of char
    b = pgm_read_byte(&font[active_char * 4 + char_ptr++]);
    // write pixels into screen memory
    for (i = 0; i < 7; i++) {
        if ((b & (1 << i)) == (1 << i)) {
            screen_mem[i+1] |= 0x01;
        }
    }
}


void clear_screen(void) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        screen_mem[i] = 0x00;
    }
}



/*
 * copy_to_buffer
 * Copies the given sprite from PROGMEM to RAM.
 */
void copy_to_buffer(const uint8_t sprite[8]) {
    memcpy_P(buffer, sprite, 8);
}



/*
 * scroll_animation
 * Uses sprite_1 and sprite_2 to draw a simple animation.
 */
void scroll_animation(const uint8_t sprite_1[8], const uint8_t sprite_2[8]) {
    uint8_t i;
    int8_t x;
    for (i = 0; i < REPEAT_ANIMATION; i++) {
        
        copy_to_buffer(sprite_1);   // push on right
        for (x = -8; x <= 0; x++) {
            copy_to_display(x, 0, buffer);
            _delay_ms(ANIMATION_SCROLL_SPEED);
        }
        
        _delay_ms(200);
        copy_to_buffer(sprite_2);
        copy_to_display(0, 0, buffer);
        _delay_ms(200);
        copy_to_buffer(sprite_1);
        copy_to_display(0, 0, buffer);
        _delay_ms(200);
        copy_to_buffer(sprite_2);
        copy_to_display(0, 0, buffer);
        _delay_ms(200);
        copy_to_buffer(sprite_1);
        
        for (x = 0; x < 8; x++) {   // push off right
            copy_to_display(x, 0, buffer);
            _delay_ms(ANIMATION_SCROLL_SPEED);
        }
    }
}

/*
 * pulse_animation
 * Uses sprite_1 and sprite_2 to pulse an animation
 */
void pulse_animation(const uint8_t sprite_1[8], const uint8_t sprite_2[8]) {
    uint8_t i;
    for (i = 0; i < REPEAT_ANIMATION; i++) {
        copy_to_buffer(sprite_1);
        copy_to_display(0, 0, buffer);
        _delay_ms(750);
        copy_to_buffer(sprite_2);
        copy_to_display(0, 0, buffer);
        _delay_ms(180);
    }
}


int main(void) {
    uint8_t mode = 0;
    
    TCCR0B |= (1 << CS01);  // T0 setup, prescale 8
    TIMSK |= (1 << TOIE0);  // T0 interrupt enable
    
    // Enable Output
    DDRA |= 0x03;
    DDRB |= 0xFF;
    DDRD |= 0x3F;
    
    // Turn off Columns & Rows
    PORTA = (1 << PA0) | (0 << PA1);
    PORTB = (0 << PB0) | (0 << PB1) | (1 << PB2) | (0 << PB3) |
            (1 << PB4) | (0 << PB5) | (0 << PB6) | (1 << PB7);
    PORTD = (1 << PD0) | (1 << PD1) | (0 << PD2) | (0 << PD3) |
            (1 << PD4) | (1 << PD5) | (0 << PD6);
    
    // enable pull ups
    PORTD |= (1 << PD6);
    
    // say hello, toggle row 1 (pixel 0,0)
//    PORTB |= (1 << PB0);    // col 1 on
//    for (i = 0; i < 5; i++) {
//        PORTA &= ~(1 << PA0);
//        _delay_ms(50);
//        PORTA |= (1 << PA0);
//        _delay_ms(50);
//    }
    
    // read last mode from eeprom
    // 0 mean cycle through all modes and messages
    mode = eeprom_read_byte(&mode_ee);
    if (mode >= (MAX_ANIMATIONS + MAX_MESSAGES + 1)) {
        mode = 1;
    }
    eeprom_write_byte(&mode_ee, mode + 1);
    
    sei();
    
    while (1) {
        
        switch (mode) {
            case 1: // Alien 1
                scroll_animation(sprite_00, sprite_01);
                break;
            case 2: // Heart
                pulse_animation(sprite_03, sprite_02);
                break;
            case 3: // Wink
                pulse_animation(sprite_04, sprite_05);
                break;
            case 4: // Alien 2
                scroll_animation(sprite_06, sprite_07);
                break;
            default: // Messages
                strcpy_P((char *)buffer, (char *)pgm_read_word(&(messages[mode - (MAX_ANIMATIONS+1)])));
                message_length = strlen((char *)buffer);
                while (message_displayed < REPEAT_TEXT) {
                    show_char();
                    _delay_ms(TEXT_SCROLL_SPEED);
                }
                message_displayed = 0;
                break;
        }
        
        // cycle through all modes
        if (CYCLE) {
            mode++;
            clear_screen();
            if (mode >= (MAX_ANIMATIONS + MAX_MESSAGES + 1)) {
                mode = 1;
            }
        }
        
    }
    
    return 0;
    
}
