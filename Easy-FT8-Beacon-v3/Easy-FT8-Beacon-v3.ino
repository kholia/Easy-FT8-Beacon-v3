// Si5351_WSPR_AND_FT8
//
// May 2021 changes by Dhiru Kholia (VU3CER):
// - Added RTC support
// - Better "DT" timings in WSPR than many RPi(s) and PC(s) is possible!
// - Internal Arduino clock is not depended upon much ;)
//
// Borrowed from:
// - https://gist.github.com/NT7S/2b5555aa28622c1b3fcbc4d7c74ad926
// - https://github.com/etherkit/JTEncode/blob/master/examples/Si5351JTDemo/Si5351JTDemo.ino
// - https://physics.princeton.edu/pulsar/k1jt/FT4_FT8_QEX.pdf
// ...
//
// Simple WSPR beacon for Arduino Uno, with the Etherkit Si5351A Breakout
// Board, by Jason Milldrum NT7S.
//
// Original code based on Feld Hell beacon for Arduino by Mark Vandewettering
// K6HX, adapted for the Si5351A by Robert Liesenfeld AK6L <ak6l@ak6l.org>.
// Timer setup code by Thomas Knutsen LA3PNA.
//
// Time code adapted from the TimeSerial.ino example from the Time library.

// Hardware Requirements
// ---------------------
// This firmware must be run on an Arduino AVR microcontroller
//
// Required Libraries
// ------------------
// Etherkit Si5351 (Library Manager)
// Etherkit JTEncode (Library Manager)
// Time (Library Manager)
// Wire (Arduino Standard Library)
// RTClib
//
// https://www.arduinoslovakia.eu/application/timer-calculator
//
// License
// -------
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject
// to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

/*

  $ ./gen_ft8 "VU3FOE VU3CER MK68" hello.wav
  Packed data: e3 e7 74 57 1f 36 dc 96 23 08
  FSK tones: 3140652707426453641754447112052035223140652130064260456762740044355664143140652

  $ ./gen_ft8 "VU3CER VU3FOE MK68" hello.wav
  Packed data: e3 e6 db 97 1f 3b a2 96 23 08
  FSK tones: 3140652707422225641757260612052036213140652317643256157154225632575077733140652

  $ ipython
  In [1]: n = "3140652707426453641754447112052035223140652130064260456762740044355664143140652"

  In [2]: %pprint
  Pretty printing has been turned OFF

  In [3]: [int(d) for d in str(n)]
  Out[3]: [3, 1, 4, 0, 6, 5, 2, 7, 0, 7, 4, 2, 6, 4, 5, 3, 6, 4, 1, 7, 5, 4, 4, 4, 7, 1, 1, 2, 0, 5, 2, 0, 3, 5, 2, 2, 3, 1, 4, 0, 6, 5, 2, 1, 3, 0, 0, 6, 4, 2, 6, 0, 4, 5, 6, 7, 6, 2, 7, 4, 0, 0, 4, 4, 3, 5, 5, 6, 6, 4, 1, 4, 3, 1, 4, 0, 6, 5, 2]

*/

#include <si5351.h>
// #include <JTEncode.h>
#define FT8_SYMBOL_COUNT                    79
#include <int.h>
#include <TimeLib.h>
#include <RTClib.h>

#include "Wire.h"

// Mode defines
#define WSPR_TONE_SPACING       146          // ~1.46 Hz
#define FT8_TONE_SPACING        625          // ~6.25 Hz

// Note
#define WSPR_CTC                10672        // CTC value for WSPR -> CTC = WSPR_DELAY / 64uS
#define FT8_CTC                 2484         // CTC value for FT8  -> CTC = FT8_DELAY / 64uS

#define WSPR_DELAY              683          // Delay value for WSPR
#define FT8_DELAY               159          // Delay value for FT8

// #define WSPR_DEFAULT_FREQ    14097200UL
#define WSPR_DEFAULT_FREQ       14096750UL
#define FT8_DEFAULT_FREQ        14075000UL
// #define FT8_DEFAULT_FREQ     14074750UL

#define DEFAULT_MODE            MODE_FT8

// https://www.qsl.net/yo4hfu/SI5351.html says,
//
// If it is necessary, frequency correction must to be applied. My Si5351
// frequency was too high, correction used 1.787KHz at 10MHz. Open again
// Si5351Example sketch, set CLK_0 to 1000000000 (10MHz). Upload. Connect a
// accurate frequency counter to CLK_0 output pin 10. Correction factor:
// (Output frequency Hz - 10000000Hz) x 100. Example: (10001787Hz - 10000000Hz)
// x 100 = 178700 Note: If output frequency is less than 10MHz, use negative
// value of correction, example -178700.

#define CORRECTION              20500        // !!! ATTENTION !!! Change this for your reference oscillator

#define TX_LED_PIN              12
#define SYNC_LED_PIN            13

const int BUTTON_PIN = 7;

// Global variables
RTC_DS3231 rtc;
Si5351 si5351;
// JTEncode jtencode;
// http://wsprnet.org/drupal/node/218 (WSPR Frequencies)
// 40m band -> 7.038600 (dial frequency) 7.040000 - 7.040200 (tx frequency range)
// 30m band -> 10.138700 (dial frequency) 10.140100 - 10.140300 7(tx frequency range)
// 20m band -> 14.095600 (dial frequency) 14.097000 - 14.097200 (tx frequency range)
// unsigned long freq = 7040500UL;              // Change this
// unsigned long freq = 14097200UL;             // Change this
// unsigned long freq = 14097100UL;             // Change this
// unsigned long freq = 10140100UL;             // Change this
// unsigned long freq = 10140000UL;             // This works great for 30m band!
// unsigned long freq = 14097000UL;             // Change this
// unsigned long freq = 14096750UL;             // Change this
// unsigned long freq = 7040500UL;              // Change this

// Generate with WSPR or ft8_lib
//
// VU3FOE VU3CER MK68
// uint8_t fixed_buffer[] = {3, 1, 4, 0, 6, 5, 2, 7, 0, 7, 4, 2, 6, 4, 5, 3, 6, 4, 1, 7, 5, 4, 4, 4, 7, 1, 1, 2, 0, 5, 2, 0, 3, 5, 2, 2, 3, 1, 4, 0, 6, 5, 2, 1, 3, 0, 0, 6, 4, 2, 6, 0, 4, 5, 6, 7, 6, 2, 7, 4, 0, 0, 4, 4, 3, 5, 5, 6, 6, 4, 1, 4, 3, 1, 4, 0, 6, 5, 2};
// VU3CER VU3FOE MK68
uint8_t fixed_buffer[] = {3, 1, 4, 0, 6, 5, 2, 7, 0, 7, 4, 2, 2, 2, 2, 5, 6, 4, 1, 7, 5, 7, 2, 6, 0, 6, 1, 2, 0, 5, 2, 0, 3, 6, 2, 1, 3, 1, 4, 0, 6, 5, 2, 3, 1, 7, 6, 4, 3, 2, 5, 6, 1, 5, 7, 1, 5, 4, 2, 2, 5, 6, 3, 2, 5, 7, 5, 0, 7, 7, 7, 3, 3, 1, 4, 0, 6, 5, 2};

// Global variables
unsigned long freq;
// char message[] = "VU3CER VU3FOE MK68";
// char call[] = "VU3FOE";
// char loc[] = "MK68";
// uint8_t dbm = 27;
// uint8_t tx_buffer[255];
uint8_t symbol_count;
uint16_t tone_delay, tone_spacing;

// Global variables used in ISRs
volatile bool proceed = false;

// Timer interrupt vector.  This toggles the variable we use to gate
// each column of output to ensure accurate timing.  Called whenever
// Timer1 hits the count set below in setup().
ISR(TIMER1_COMPA_vect)
{
  proceed = true;
}

// Loop through the string, transmitting one character at a time.
void tx()
{
  uint8_t i;

  // Reset the tone to the base frequency and turn on the output
  si5351.set_clock_pwr(SI5351_CLK0, 1);
  digitalWrite(TX_LED_PIN, HIGH);

  // Now do the rest of the message
  for (i = 0; i < symbol_count; i++)
  {
    // si5351.set_freq((freq * 100) + (tx_buffer[i] * tone_spacing), SI5351_CLK0);
    si5351.set_freq((freq * 100) + (fixed_buffer[i] * tone_spacing), SI5351_CLK0);
    proceed = false;
    while (!proceed);
  }

  // Turn off the output
  si5351.set_clock_pwr(SI5351_CLK0, 0);
  digitalWrite(TX_LED_PIN, LOW);
}

#define TIME_HEADER             "T"          // Header tag for serial time sync message
unsigned long processSyncMessage()
{
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if (Serial.find((char *)TIME_HEADER)) {
    pctime = Serial.parseInt();
    return pctime;
    /* if( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
      pctime = 0L; // return 0 to indicate that the time is not valid
      } */
  }
  return pctime;
}

time_t requestSync()
{
  return rtc.now().unixtime();
}

int buttonState = 0;

void setup()
{
  int ret = 0;
  char date[10] = "hh:mm:ss";

  // Use the Arduino's on-board LED as a keying indicator.
  pinMode(TX_LED_PIN, OUTPUT);
  pinMode(SYNC_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(TX_LED_PIN, LOW);
  digitalWrite(SYNC_LED_PIN, LOW);
  Serial.begin(115200);

  Serial.println("..................");

  // Initialize the rtc
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    Serial.flush();
    abort();
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, abort!?");
    Serial.flush();
    // abort();  // NOTE
  }
  // We don't need the 32K Pin, so disable it
  rtc.disable32K();
  rtc.now().toString(date);
  Serial.print("Current time is = ");
  Serial.println(date);
  Serial.print("Temperature is: ");
  Serial.print(rtc.getTemperature());
  Serial.println(" C");

  // Initialize the Si5351
  // Change the 2nd parameter in init if using a ref osc other
  // than 25 MHz
  ret = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, CORRECTION);
  Serial.print("Si5351 init status (should be 1 always) = ");
  Serial.println(ret);
  if (ret != true) {
    Serial.flush();
    abort();
  }

  // Set CLK0 output
  si5351.set_freq(freq * 100, SI5351_CLK0);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA); // Set for minimum power
  // si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA); // Set for max power
  // delay(10000); // Keep TX on for 5 seconds for tunining purposes.
  si5351.set_clock_pwr(SI5351_CLK0, 0); // Disable the clock initially

  // Set up Timer1 for interrupts every symbol period.
  noInterrupts();          // Turn off interrupts.
  TCCR1A = 0;              // Set entire TCCR1A register to 0; disconnects
  //   interrupt output pins, sets normal waveform
  //   mode.  We're just using Timer1 as a counter.
  TCNT1  = 0;              // Initialize counter value to 0.
  TCCR1B = (1 << CS12) |   // Set CS12 and CS10 bit to set prescale
           (1 << CS10) |          //   to /1024
           (1 << WGM12);          //   turn on CTC
  //   which gives, 64 us ticks
  TIMSK1 = (1 << OCIE1A);  // Enable timer compare interrupt.
  // OCR1A = WSPR_CTC;        // Set up interrupt trigger count;
  OCR1A = FT8_CTC;         // Set up interrupt trigger count;
  interrupts();            // Re-enable interrupts.

  // Note: Set time sync provider
  setSyncProvider(requestSync);  // set function to call when sync required

  /* freq = WSPR_DEFAULT_FREQ;
    symbol_count = WSPR_SYMBOL_COUNT; // From the library defines
    tone_spacing = WSPR_TONE_SPACING;
    tone_delay = WSPR_DELAY;
    jtencode.wspr_encode(call, loc, dbm, tx_buffer); // one time ;) */

  // FT8
  freq = FT8_DEFAULT_FREQ;
  symbol_count = FT8_SYMBOL_COUNT; // From the library defines
  tone_spacing = FT8_TONE_SPACING;
  tone_delay = FT8_DELAY;
  // jtencode.ft8_encode(message, tx_buffer); // buggy/limited
}

DateTime dt;
time_t time;

int sync_mode = 0;

char date[10] = "hh:mm:ss";

void led_flash()
{
  digitalWrite(SYNC_LED_PIN, HIGH);
  delay(250);
  digitalWrite(SYNC_LED_PIN, LOW);
  delay(250);
  digitalWrite(SYNC_LED_PIN, HIGH);
  delay(250);
  digitalWrite(SYNC_LED_PIN, LOW);
  delay(250);
  digitalWrite(SYNC_LED_PIN, HIGH);
  delay(250);
}

void loop()
{
  buttonState = digitalRead(BUTTON_PIN);

  if ((buttonState == LOW || sync_mode == 1) && sync_mode != 3) {  // Sync mode
    sync_mode = 1;
    if (Serial.available()) {
      time_t t = processSyncMessage();
      if (t != 0) {
        rtc.adjust(DateTime(2021, 0, 0, 0, 0, t));
        setTime(t);
        Serial.println("RTC adjusted!");
        sync_mode = 2;
        led_flash();
      }
      rtc.now().toString(date);
      Serial.print("Current time is = ");
      Serial.println(date);
      led_flash();
    }
  } else {
    dt = rtc.now();

    if (timeStatus() == timeSet) {
      digitalWrite(SYNC_LED_PIN, HIGH); // LED on if synced
    }
    else {
      digitalWrite(SYNC_LED_PIN, LOW);  // LED off if needs refresh
    }

    // Trigger every 15 seconds! FT8
    if (timeStatus() == timeSet && dt.second() % 15 == 0) {
      tx();
      dt = rtc.now();
      time = dt.unixtime();
      setTime(time);
    }

    delay(100);
  }

  // Debug (keep this code disabled)
  // Serial.println(now());
  // delay(100);
}
