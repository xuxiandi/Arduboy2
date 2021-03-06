/**
 * @file Arduboy2Core.cpp
 * \brief
 * The Arduboy2Core class for Arduboy hardware initilization and control.
 */

#include "Arduboy2Core.h"

#ifdef __AVR_ATmega32U4__
#include <Wire.h>
#endif

#ifdef LIMIT_BUTTON_CALLS

#endif

#ifdef __AVR_ATmega32U4__
const uint8_t PROGMEM lcdBootProgram[] = {
    // boot defaults are commented out but left here in case they
    // might prove useful for reference
    //
    // Further reading: https://www.adafruit.com/datasheets/SSD1306.pdf
    //
    // Display Off
    // 0xAE,

    // Set Display Clock Divisor v = 0xF0
    // default is 0x80
    0xD5, 0xF0,

    // Set Multiplex Ratio v = 0x3F
    // 0xA8, 0x3F,

    // Set Display Offset v = 0
    // 0xD3, 0x00,

    // Set Start Line (0)
    // 0x40,

    // Charge Pump Setting v = enable (0x14)
    // default is disabled
    0x8D, 0x14,

    // Set Segment Re-map (A0) | (b0001)
    // default is (b0000)
    0xA1,

    // Set COM Output Scan Direction
    0xC8,

    // Set COM Pins v
    // 0xDA, 0x12,

    // Set Contrast v = 0xCF
    0x81, 0xCF,

    // Set Precharge = 0xF1
    0xD9, 0xF1,

    // Set VCom Detect
    // 0xDB, 0x40,

    // Entire Display ON
    // 0xA4,

    // Set normal/inverse display
    // 0xA6,

    // Display On
    0xAF,

    // set display mode = horizontal addressing mode (0x00)
    0x20, 0x00,

    // set col address range
    // 0x21, 0x00, WIDTH-1,

    // set page address range
    // 0x22, 0x00, 0x07,
};
#endif

Arduboy2Core::Arduboy2Core() {}

void Arduboy2Core::boot()
{
#ifdef __AVR_ATmega32U4__
  // Select the ADC input here so a delay isn't required in initRandomSeed()
  ADMUX = RAND_SEED_IN_ADMUX;
#endif

  bootPins();
  bootSPI();
  bootOLED();
  bootPowerSaving();
}

// Pins are set to the proper modes and levels for the specific hardware.
// This routine must be modified if any pins are moved to a different port
void Arduboy2Core::bootPins()
{
#ifdef __AVR_ATmega32U4__
  // Port B INPUT_PULLUP or HIGH
  PORTB |= _BV(RED_LED_BIT) | _BV(GREEN_LED_BIT) | _BV(BLUE_LED_BIT) |
           _BV(B_BUTTON_BIT);
  // Port B INPUT or LOW (none)
  // Port B inputs
  DDRB &= ~(_BV(B_BUTTON_BIT) | _BV(SPI_MISO_BIT));
  // Port B outputs
  DDRB |= _BV(RED_LED_BIT) | _BV(GREEN_LED_BIT) | _BV(BLUE_LED_BIT) |
          _BV(SPI_MOSI_BIT) | _BV(SPI_SCK_BIT) | _BV(SPI_SS_BIT);

  // Port C
  // Speaker: Not set here. Controlled by audio class

  // Port D INPUT_PULLUP or HIGH
  PORTD |= _BV(CS_BIT);
  // Port D INPUT or LOW
  PORTD &= ~(_BV(RST_BIT));
  // Port D inputs (none)
  // Port D outputs
  DDRD |= _BV(RST_BIT) | _BV(CS_BIT) | _BV(DC_BIT);

  // Port E INPUT_PULLUP or HIGH
  PORTE |= _BV(A_BUTTON_BIT);
  // Port E INPUT or LOW (none)
  // Port E inputs
  DDRE &= ~(_BV(A_BUTTON_BIT));
  // Port E outputs (none)

  // Port F INPUT_PULLUP or HIGH
  PORTF |= _BV(LEFT_BUTTON_BIT) | _BV(RIGHT_BUTTON_BIT) |
           _BV(UP_BUTTON_BIT) | _BV(DOWN_BUTTON_BIT);
  // Port F INPUT or LOW
  PORTF &= ~(_BV(RAND_SEED_IN_BIT));
  // Port F inputs
  DDRF &= ~(_BV(LEFT_BUTTON_BIT) | _BV(RIGHT_BUTTON_BIT) |
            _BV(UP_BUTTON_BIT) | _BV(DOWN_BUTTON_BIT) |
            _BV(RAND_SEED_IN_BIT));
  // Port F outputs (none)
#endif
}

void Arduboy2Core::bootOLED()
{
#ifdef __AVR_ATmega32U4__
  // reset the display
  delayShort(5);             // reset pin should be low here. let it stay low a while
  bitSet(RST_PORT, RST_BIT); // set high to come out of reset
  delayShort(5);             // wait a while

  // select the display (permanently, since nothing else is using SPI)
  bitClear(CS_PORT, CS_BIT);

  // run our customized boot-up command sequence against the
  // OLED to initialize it properly for Arduboy
  LCDCommandMode();
  for (uint8_t i = 0; i < sizeof(lcdBootProgram); i++)
  {
    SPItransfer(pgm_read_byte(lcdBootProgram + i));
  }
  LCDDataMode();
#endif
}

void Arduboy2Core::LCDDataMode()
{
#ifdef __AVR_ATmega32U4__
  bitSet(DC_PORT, DC_BIT);
#endif
}

void Arduboy2Core::LCDCommandMode()
{
#ifdef __AVR_ATmega32U4__
  bitClear(DC_PORT, DC_BIT);
#endif
}

// Initialize the SPI interface for the display
void Arduboy2Core::bootSPI()
{
#ifdef __AVR_ATmega32U4__
  // master, mode 0, MSB first, CPU clock / 2 (8MHz)
  SPCR = _BV(SPE) | _BV(MSTR);
  SPSR = _BV(SPI2X);
#endif
}

// Write to the SPI bus (MOSI pin)
void Arduboy2Core::SPItransfer(uint8_t data)
{
#ifdef __AVR_ATmega32U4__
  SPDR = data;
  /*
   * The following NOP introduces a small delay that can prevent the wait
   * loop form iterating when running at the maximum speed. This gives
   * about 10% more speed, even if it seems counter-intuitive. At lower
   * speeds it is unnoticed.
   */
  asm volatile("nop");
  while (!(SPSR & _BV(SPIF)))
  {
  } // wait
#endif
}

void Arduboy2Core::safeMode()
{
#ifdef __AVR_ATmega32U4__
  if (buttonsState() == UP_BUTTON)
  {
    digitalWriteRGB(RED_LED, RGB_ON);

#ifndef ARDUBOY_CORE // for Arduboy core timer 0 should remain enabled
    // prevent the bootloader magic number from being overwritten by timer 0
    // when a timer variable overlaps the magic number location
    power_timer0_disable();
#endif

    while (true)
    {
    }
  }
#endif
}

/* Power Management */

void Arduboy2Core::idle()
{
#ifdef __AVR_ATmega32U4__
  SMCR = _BV(SE); // select idle mode and enable sleeping
  sleep_cpu();
  SMCR = 0; // disable sleeping
#endif
}

void Arduboy2Core::bootPowerSaving()
{
#ifdef __AVR_ATmega32U4__
  // disable Two Wire Interface (I2C) and the ADC
  // All other bits will be written with 0 so will be enabled
  PRR0 = _BV(PRTWI) | _BV(PRADC);
  // disable USART1
  PRR1 |= _BV(PRUSART1);
#endif
}

// Shut down the display
void Arduboy2Core::displayOff()
{
#if defined(DISPLAY_SSD1306)
  sendLCDCommand(DISPLAYOFF);
#elif __AVR_ATmega32U4__
  LCDCommandMode();
  SPItransfer(0xAE); // display off
  SPItransfer(0x8D); // charge pump:
  SPItransfer(0x10); //   disable
  delayShort(250);
  bitClear(RST_PORT, RST_BIT); // set display reset pin low (reset state)
#endif
}

// Restart the display after a displayOff()
void Arduboy2Core::displayOn()
{
#if defined(DISPLAY_SSD1306)
  sendLCDCommand(DISPLAYON);
#elif __AVR_ATmega32U4__
  bootOLED();
#endif
}

uint8_t Arduboy2Core::width() { return WIDTH; }

uint8_t Arduboy2Core::height() { return HEIGHT; }

/* Drawing */

void Arduboy2Core::paint8Pixels(uint8_t pixels)
{
#if defined(DISPLAY_SSD1306)
  uint8_t command[2] = {0x40, pixels};
  brzo_i2c_start_transaction(OLED_I2C_ADRESS, BRZO_I2C_SPEED);
  brzo_i2c_write(command, 2, true);
  brzo_i2c_end_transaction();
#elif __AVR_ATmega32U4__
  SPItransfer(pixels);
#endif
}

void Arduboy2Core::paintScreen(const uint8_t *image)
{
#if defined(DISPLAY_SSD1306)
  for (uint8_t i = 0; i < (WIDTH * HEIGHT / (16 * 8));)
  {
    brzo_i2c_start_transaction(OLED_I2C_ADRESS, BRZO_I2C_SPEED);
    for (uint8_t x = 0; x < 16; x++, i++)
    {
      uint8_t command[2] = {0x40, pgm_read_byte(image + i + x)};
      brzo_i2c_write(command, 2, true);
    }
    brzo_i2c_end_transaction();
  }
#elif __AVR_ATmega32U4__
  for (int i = 0; i < (HEIGHT * WIDTH) / 8; i++)
  {
    SPI.transfer(pgm_read_byte(image + i));
  }
#endif
}

// paint from a memory buffer, this should be FAST as it's likely what
// will be used by any buffer based subclass
//
// The following assembly code runs "open loop". It relies on instruction
// execution times to allow time for each byte of data to be clocked out.
// It is specifically tuned for a 16MHz CPU clock and SPI clocking at 8MHz.
void Arduboy2Core::paintScreen(uint8_t image[], bool clear)
{
#if defined(DISPLAY_SSD1306)
  brzo_i2c_start_transaction(OLED_I2C_ADRESS, BRZO_I2C_SPEED);
  uint8_t command[1] = {0x40};
  brzo_i2c_write(command, 1, true);
  for (uint16_t i = 0; i < (WIDTH * HEIGHT / 8); i++)
  {
    uint8_t command[1] = {image[i]};
    brzo_i2c_write(command, 1, true);
  }
  brzo_i2c_end_transaction();
#elif __AVR_ATmega32U4__
  uint16_t count;

  asm volatile(
      "   ldi   %A[count], %[len_lsb]               \n\t" //for (len = WIDTH * HEIGHT / 8)
      "   ldi   %B[count], %[len_msb]               \n\t"
      "1: ld    __tmp_reg__, %a[ptr]      ;2        \n\t" //tmp = *(image)
      "   out   %[spdr], __tmp_reg__      ;1        \n\t" //SPDR = tmp
      "   cpse  %[clear], __zero_reg__    ;1/2      \n\t" //if (clear) tmp = 0;
      "   mov   __tmp_reg__, __zero_reg__ ;1        \n\t"
      "2: sbiw  %A[count], 1              ;2        \n\t" //len --
      "   sbrc  %A[count], 0              ;1/2      \n\t" //loop twice for cheap delay
      "   rjmp  2b                        ;2        \n\t"
      "   st    %a[ptr]+, __tmp_reg__     ;2        \n\t" //*(image++) = tmp
      "   brne  1b                        ;1/2 :18  \n\t" //len > 0
      "   in    __tmp_reg__, %[spsr]                \n\t" //read SPSR to clear SPIF
      : [ptr] "+&e"(image),
        [count] "=&w"(count)
      : [spdr] "I"(_SFR_IO_ADDR(SPDR)),
        [spsr] "I"(_SFR_IO_ADDR(SPSR)),
        [len_msb] "M"(WIDTH * (HEIGHT / 8 * 2) >> 8),   // 8: pixels per byte
        [len_lsb] "M"(WIDTH * (HEIGHT / 8 * 2) & 0xFF), // 2: for delay loop multiplier
        [clear] "r"(clear));
#endif
}

void Arduboy2Core::blank()
{
#if defined(DISPLAY_SSD1306)
  sendLCDCommand(DISPLAYOFF);
#elif __AVR_ATmega32U4__
  for (int i = 0; i < (HEIGHT * WIDTH) / 8; i++)
    SPI.transfer(0x00);
#endif
}

void Arduboy2Core::sendLCDCommand(uint8_t command)
{
#if defined(DISPLAY_SSD1306)
  uint8_t com[2] = {0x80 /* command mode */, command};
  brzo_i2c_start_transaction(OLED_I2C_ADRESS, BRZO_I2C_SPEED);
  brzo_i2c_write(com, 2, true);
  brzo_i2c_end_transaction();
#elif __AVR_ATmega32U4__
  LCDCommandMode();
  SPI.transfer(command);
  LCDDataMode();
#endif
}

// invert the display or set to normal
// when inverted, a pixel set to 0 will be on
void Arduboy2Core::invert(bool inverse)
{
  sendLCDCommand(inverse ? OLED_PIXELS_INVERTED : OLED_PIXELS_NORMAL);
}

// turn all display pixels on, ignoring buffer contents
// or set to normal buffer display
void Arduboy2Core::allPixelsOn(bool on)
{
  sendLCDCommand(on ? OLED_ALL_PIXELS_ON : OLED_PIXELS_FROM_RAM);
}

// flip the display vertically or set to normal
void Arduboy2Core::flipVertical(bool flipped)
{
  sendLCDCommand(flipped ? OLED_VERTICAL_FLIPPED : OLED_VERTICAL_NORMAL);
}

// flip the display horizontally or set to normal
void Arduboy2Core::flipHorizontal(bool flipped)
{
  sendLCDCommand(flipped ? OLED_HORIZ_FLIPPED : OLED_HORIZ_NORMAL);
}

/* RGB LED */

void Arduboy2Core::setRGBled(uint8_t red, uint8_t green, uint8_t blue)
{
#if defined(__AVR_ATmega32U4__)
#if defined(ARDUBOY_10) // RGB, all the pretty colors
  // timer 0: Fast PWM, OC0A clear on compare / set at top
  // We must stay in Fast PWM mode because timer 0 is used for system timing.
  // We can't use "inverted" mode because it won't allow full shut off.
  TCCR0A = _BV(COM0A1) | _BV(WGM01) | _BV(WGM00);
  OCR0A = 255 - green;
  // timer 1: Phase correct PWM 8 bit
  // OC1A and OC1B set on up-counting / clear on down-counting (inverted). This
  // allows the value to be directly loaded into the OCR with common anode LED.
  TCCR1A = _BV(COM1A1) | _BV(COM1A0) | _BV(COM1B1) | _BV(COM1B0) | _BV(WGM10);
  OCR1AL = blue;
  OCR1BL = red;
#elif defined(AB_DEVKIT)
  // only blue on DevKit, which is not PWM capable
  (void)red;   // parameter unused
  (void)green; // parameter unused
  bitWrite(BLUE_LED_PORT, BLUE_LED_BIT, blue ? RGB_ON : RGB_OFF);
#endif
#endif
}

void Arduboy2Core::setRGBled(uint8_t color, uint8_t val)
{
#if defined(__AVR_ATmega32U4__)
#if defined(ARDUBOY_10)
  if (color == RED_LED)
  {
    OCR1BL = val;
  }
  else if (color == GREEN_LED)
  {
    OCR0A = 255 - val;
  }
  else if (color == BLUE_LED)
  {
    OCR1AL = val;
  }
#elif defined(AB_DEVKIT)
  // only blue on DevKit, which is not PWM capable
  if (color == BLUE_LED)
  {
    bitWrite(BLUE_LED_PORT, BLUE_LED_BIT, val ? RGB_ON : RGB_OFF);
  }
#endif
#endif
}

void Arduboy2Core::freeRGBled()
{
#if defined(__AVR_ATmega32U4__)
#ifdef ARDUBOY_10
  // clear the COM bits to return the pins to normal I/O mode
  TCCR0A = _BV(WGM01) | _BV(WGM00);
  TCCR1A = _BV(WGM10);
#endif
#endif
}

void Arduboy2Core::digitalWriteRGB(uint8_t red, uint8_t green, uint8_t blue)
{
#if defined(__AVR_ATmega32U4__)
#ifdef ARDUBOY_10
  bitWrite(RED_LED_PORT, RED_LED_BIT, red);
  bitWrite(GREEN_LED_PORT, GREEN_LED_BIT, green);
  bitWrite(BLUE_LED_PORT, BLUE_LED_BIT, blue);
#elif defined(AB_DEVKIT)
  // only blue on DevKit
  (void)red;   // parameter unused
  (void)green; // parameter unused
  bitWrite(BLUE_LED_PORT, BLUE_LED_BIT, blue);
#endif
#endif
}

void Arduboy2Core::digitalWriteRGB(uint8_t color, uint8_t val)
{
#if defined(__AVR_ATmega32U4__)
#ifdef ARDUBOY_10
  if (color == RED_LED)
  {
    bitWrite(RED_LED_PORT, RED_LED_BIT, val);
  }
  else if (color == GREEN_LED)
  {
    bitWrite(GREEN_LED_PORT, GREEN_LED_BIT, val);
  }
  else if (color == BLUE_LED)
  {
    bitWrite(BLUE_LED_PORT, BLUE_LED_BIT, val);
  }
#elif defined(AB_DEVKIT)
  // only blue on DevKit
  if (color == BLUE_LED)
  {
    bitWrite(BLUE_LED_PORT, BLUE_LED_BIT, val);
  }
#endif
#endif
}

/* Buttons */

// delay in ms with 16 bit duration
void Arduboy2Core::delayShort(uint16_t ms)
{
  delay((uint32_t)ms);
}

void Arduboy2Core::exitToBootloader()
{
#if defined(__AVR_ATmega32U4__)
  cli();
  // enable watchdog timer reset, with 16ms timeout
  wdt_reset();
  WDTCSR = (_BV(WDCE) | _BV(WDE));
  WDTCSR = _BV(WDE);
  while (true)
  {
  }
#endif
}

// Replacement main() that eliminates the USB stack code.
// Used by the ARDUBOY_NO_USB macro. This should not be called
// directly from a sketch.

void Arduboy2Core::mainNoUSB()
{
#if defined(__AVR_ATmega32U4__)
  // disable USB
  UDCON = _BV(DETACH);
  UDIEN = 0;
  UDINT = 0;
  USBCON = _BV(FRZCLK);
  UHWCON = 0;
  power_usb_disable();

  init();

  // This would normally be done in the USB code that uses the TX and RX LEDs
  TX_RX_LED_INIT;
  TXLED0;
  RXLED0;

  // Set the DOWN button pin for INPUT_PULLUP
  bitSet(DOWN_BUTTON_PORT, DOWN_BUTTON_BIT);
  bitClear(DOWN_BUTTON_DDR, DOWN_BUTTON_BIT);

  // Delay to give time for the pin to be pulled high if it was floating
  delayShort(10);

  // if the DOWN button is pressed
  if (bitRead(DOWN_BUTTON_PORTIN, DOWN_BUTTON_BIT) == 0)
  {
    exitToBootloader();
  }

  // The remainder is a copy of the Arduino main() function with the
  // USB code and other unneeded code commented out.
  // init() was called above.
  // The call to function initVariant() is commented out to fix compiler
  // error: "multiple definition of 'main'".
  // The return statement is removed since this function is type void.

  //  init();

  //  initVariant();

  //#if defined(USBCON)
  //  USBDevice.attach();
  //#endif

  setup();

  for (;;)
  {
    loop();
    //    if (serialEventRun) serialEventRun();
  }

//  return 0;
#endif
}
