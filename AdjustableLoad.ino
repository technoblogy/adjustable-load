/* Adjustable Load - see http://www.technoblogy.com/show?4997

   David Johnson-Davies - www.technoblogy.com - 24th March 2023
   ATtiny84 @ 1 MHz (internal oscillator; BOD disabled)
   
   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license: 
   http://creativecommons.org/licenses/by/4.0/
*/

// Seven-segment definitions
char charArray[] = {
//  ABCDEFG  Segments
  0b1111110, // 0
  0b0110000, // 1
  0b1101101, // 2
  0b1111001, // 3
  0b0110011, // 4
  0b1011011, // 5
  0b1011111, // 6
  0b1110000, // 7
  0b1111111, // 8
  0b1111011, // 9
  0b0000000, // 10  Space
  0b0000001  // 11  Dash
};

const int Dash = 11;
const int Space = 10;
const int Ndigits = 3;                       // Number of digits

volatile int Current = 0;                    // Current in mA
volatile unsigned long Charge = 0;           // Charge in mA-seconds

volatile char Buffer[] = {Space, Space, Space};
int digit = 0;
int DP = -1;  // Decimal point position 0 or 1

// Pin assignments
char Digits[] = {0, 1, 2};                   // Bit positions in port B

// Display multiplexer **********************************************

void DisplayNextDigit() {
  static int LastReading;
  DDRB = 0;                                  // Make all digits inputs 
  DDRA = 0;                                  // Make all segments inputs 
  digit = (digit+1) % (Ndigits+1);
  if (digit < Ndigits) {
    char segs = charArray[Buffer[digit]];
    if (digit == DP) {
      DDRB = DDRB | 0x04;                    // Decimal point output
      PORTB = PORTB & 0xFB;                  // Decimal point low
    }
    DDRA = segs & 0x7F;                      // Segments outputs
    PORTA = ~segs & 0x7F;                    // Segments low
    DDRB = DDRB | 1<<Digits[digit];          // Make digit an output
    PORTB = PORTB | 1<<Digits[digit];        // Take digit bit high
  } else {
    // Read analogue input
    Current = (ReadADC()*29)/27;
    // If current near zero, display charge
    if (Current < 100) {                     // < 0.1A
      int mAh = Charge/3600;
      if (mAh < 1000) Display(mAh, -1); else Display(mAh/10, 0);
      LastReading = 0;
    } else {
      // Add hysteresis to stop display jumping
      if (abs(LastReading - Current) >= 10) {
        Display((Current + 5)/10, 0);
        LastReading = Current;
      }
    }
  }
}

// Display a three digit decimal number with decimal point
void Display (int number, int point) {
  int blank = false, digit, j=100;
  if (point != 0) blank = true;
  for (int d=0; d<Ndigits ; d++) {
    digit = (number/j) % 10;
    if (digit==0 && blank && d!=2) Buffer[d] = Space;
    else {
      Buffer[d] = digit;
      blank = false;
    }
    j = j / 10;
  }
  DP = point;
}

// Timer/Counter0 multiplexes the display **********************************************

void SetupTimer0 () {
  TCCR0A = 2<<WGM00;                         // CTC mode
  TCCR0B = 0<<WGM02 | 3<<CS00;               // Divide by 64
  OCR0A = 49;                                // Divide by 50 for 312.5Hz interrupt
  TIMSK0 = 1<<OCIE0A;                        // Enable compare match interrupt
}

// Timer 0 interrupt - multiplexes display and does ADC conversion
ISR(TIM0_COMPA_vect) {
  DisplayNextDigit();
}

// Timer/Counter1 counts seconds **********************************************

void SetupTimer1 () {
  TCCR1A = 0<<WGM10;
  TCCR1B = 1<<WGM12 | 3<<CS10;               // Divide by 64
  OCR1A = 15624;                             // Divide by 15625 for 1Hz interrupt
  TIMSK1 = 1<<OCIE1A;                        // Enable compare match interrupt
}

// Timer 1 interrupt - counts mAh
ISR(TIM1_COMPA_vect) {
  Charge = Charge + Current;
  if (Current != 0) DP = DP ^ 0xFF;          // Flash decimal point
}

// Do ADC conversions **********************************************

void SetupADC () {
  // Set up ADC on ADC7 (PA7)
  ADMUX = 2<<REFS0 | 7<<MUX0;                // 1.1V ref, ADC7 (PA7)
  ADCSRA = 1<<ADEN | 4<<ADPS0;               // Enable ADC, 62.5kHz clock
}

int ReadADC () {
  unsigned char low, high;
  ADCSRA = ADCSRA | 1<<ADSC;                 // Start
  while (ADCSRA & 1<<ADSC);                  // Wait while conversion in progress
  return ADC;
}

// Setup **********************************************

void setup () {
  OSCCAL = 130;                              // Adjust for 78.125Hz at PB1
  SetupTimer0();
  SetupTimer1();
  SetupADC();
}

// Everything done under interrupt
void loop () {
}
