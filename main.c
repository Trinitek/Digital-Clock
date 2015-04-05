
#include <xc.h>
#include <stdint.h>

#define _clock                  LATAbits.LATA5
#define _serial                 LATAbits.LATA4
#define _strobe                 LATCbits.LATC5

#define _led_s                  LATCbits.LATC0
#define _led_m                  LATCbits.LATC6
#define _led_h                  LATCbits.LATC7

#define _button_s               PORTBbits.RB7
#define _button_m               PORTBbits.RB5
#define _button_h               PORTCbits.RC1

int oldButtonS = 1;
int oldButtonM = 1;
int oldButtonH = 1;
int oldDisplay = 2;
int second = 0;
int minute = 0;
int hour = 12;

void initIO(void) {
    /* Every pin is digital I/O */
    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;

    /* Shift registers for LED anodes */
    TRISAbits.TRISA5 = 0;       // clock
    TRISAbits.TRISA4 = 0;       // serial
    TRISCbits.TRISC5 = 0;       // strobe

    /* LED cathodes */
    TRISCbits.TRISC0 = 0;       // led- (seconds)
    LATCbits.LATC0 = 1;

    TRISCbits.TRISC6 = 0;       // led- (minutes)
    LATCbits.LATC6 = 1;

    TRISCbits.TRISC7 = 0;       // led- (hours)
    LATCbits.LATC7 = 1;

    /* Active low button inputs */
    TRISBbits.TRISB7 = 1;       // second++ button
    WPUBbits.WPUB7 = 1;         // pullup enabled

    TRISBbits.TRISB5 = 1;       // minute++ button
    WPUBbits.WPUB5 = 1;         // pullup enabled

    TRISCbits.TRISC1 = 1;       // hour++ button
                                // using pullup in hardware
    OPTION_REGbits.nWPUEN = 0;  // global pull-up disable bit is disabled
}

void initOscillator(void) {
    OSCCONbits.IRCF = 0b1100;   // 2 MHz CPU clock
}

void initTimer(void) {
    /* TMR0 - button debouncing */
    OPTION_REGbits.TMR0CS = 0;  // use CPU clock divided by 4
    OPTION_REGbits.PSA = 0;     // assign prescaler to timer
    OPTION_REGbits.PS = 0b111;  // 1/256 prescale rate = 1.593 kHz

    /* TMR1 - time keeping */
    T1CONbits.TMR1CS = 0b11;    // internal 32.768 kHz oscillator
    T1CONbits.nT1SYNC = 1;      // do not synchronize asynchronous clock input
    TMR1H = 0;                  // clear counter
    TMR1L = 0;
    T1CONbits.TMR1ON = 1;       // enable timer
}

void shiftOut(uint16_t pattern) {
    uint16_t mask = 0x8000;

    _clock = 1;

    for (int i = 16; i > 0; i--) {

        _serial = (mask & pattern) ? 1 : 0;

        _clock = 0;
        _clock = 1;

        mask = mask >> 1;
    }

    // update shift register output
    _strobe = 1;
    _strobe = 0;
}

void updateDisplay(void) {
    uint16_t pattern;

    if (oldDisplay == 2) {
        // push seconds display into pattern
        oldDisplay = 0;

        // extract 1's digit
        pattern = (1 << (second % 10));

        // extract 10's digit
        if (second / 10) pattern = pattern ^ (1 << ((second / 10) + 9));

        // shift pattern out
        shiftOut(pattern);

        _led_h = 1;
        _led_s = 0;

    } else if (oldDisplay == 0) {
        // push minutes display into pattern
        oldDisplay++;

        // extract 1's digit
        pattern = (1 << (minute % 10));

        // extract 10's digit
        if (minute / 10) pattern = pattern ^ (1 << ((minute / 10) + 9));

        // shift pattern out
        shiftOut(pattern);

        _led_s = 1;
        _led_m = 0;

    } else if (oldDisplay == 1) {
        // push hours display into pattern
        oldDisplay++;

        // extract 1's digit
        pattern = (1 << (hour % 10));

        // extract 10's digit
        if (hour / 10) pattern = pattern ^ (1 << ((hour / 10) + 9));

        // shift pattern out
        shiftOut(pattern);

        _led_m = 1;
        _led_h = 0;
        
    }
}

void incrementHours(void) {
    if (hour == 12) {
        hour = 1;
    } else hour++;
}

void incrementMinutes(void) {
    if (minute == 59) {
        minute = 0;
        incrementHours();
    } else minute++;
}

void incrementSeconds(void) {
    if (second == 59) {
        second = 0;
        incrementMinutes();
    } else second++;
}

void main(void) {
    initIO();
    initOscillator();
    initTimer();

    while (1) {
        /* Check to see if the timer has counted beyond 32768 */
        if (TMR1H == 0x80) {
            // reset the timer
            TMR1H = 0;
            TMR1L = 0;

            incrementSeconds();
        }

        /* Check for button input */
        // all buttons are active low
        if (TMR0 >= (unsigned) 192) {
            if (oldButtonS == 0 && _button_s == 1) incrementSeconds();
            if (oldButtonM == 0 && _button_m == 1) incrementMinutes();
            if (oldButtonH == 0 && _button_h == 1) incrementHours();

            oldButtonS = _button_s;
            oldButtonM = _button_m;
            oldButtonH = _button_h;
        }

        /* Update display */
        updateDisplay();
    }
}
