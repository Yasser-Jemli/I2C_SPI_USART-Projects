
// button definitions
#define button1      RA5_bit   // button B1 is connected to RA5 pin
#define button2      RA4_bit   // button B2 is connected to RA4 pin

// include I2C LCD driver source file
#include "I2C_LCD.c"

// variables declaration
char  i, second, minute, hour, m_day, month, year;

// a small function for button1 (B1) debounce
char debounce ()
{
  char i, count = 0;
  for(i = 0; i < 5; i++)
  {
    if (button1 == 0)
      count++;
    delay_ms(10);
  }
  if(count > 2)  return 1;
  else           return 0;
}

/********************** RTC functions *********************/
// convert BCD to decimal function
uint8_t bcd_to_decimal(uint8_t number)
{
  return((number >> 4) * 10 + (number & 0x0F));
}

// convert decimal to BCD function
uint8_t decimal_to_bcd(uint8_t number)
{
  return(((number / 10) << 4) + (number % 10));
}

// display time and date function
void RTC_display()
{
  // convert data from BCD format to decimal format
  second = bcd_to_decimal(second);
  minute = bcd_to_decimal(minute);
  hour   = bcd_to_decimal(hour);
  m_day  = bcd_to_decimal(m_day);
  month  = bcd_to_decimal(month);
  year   = bcd_to_decimal(year);
  // end conversion

  // print seconds
  LCD_Goto(13, 1);
  LCD_PutC( (second / 10) % 10 + '0');
  LCD_PutC(  second       % 10 + '0');
  // print minutes
  LCD_Goto(10, 1);
  LCD_PutC( (minute / 10) % 10 + '0');
  LCD_PutC(  minute       % 10 + '0');
  // print hours
  LCD_Goto(7, 1);
  LCD_PutC( (hour / 10) % 10 + '0');
  LCD_PutC(  hour       % 10 + '0');
  // print day of the month
  LCD_Goto(7, 2);
  LCD_PutC( (m_day / 10) % 10 + '0');
  LCD_PutC(  m_day       % 10 + '0');
  // print month
  LCD_Goto(10, 2);
  LCD_PutC( (month / 10) % 10 + '0');
  LCD_PutC(  month       % 10 + '0');
  // print year
  LCD_Goto(15, 2);
  LCD_PutC( (year / 10) % 10 + '0');
  LCD_PutC(  year       % 10 + '0');

}

// make editing parameter blinks function
void delay()
{
  TMR1H = TMR1L = 0;   // reset Timer1
  TMR1ON_bit    = 1;   // enable Timer1 module
  // wait for 250ms or at least one button press
  while ( ((unsigned)(TMR1H << 8) | TMR1L) < 62500 && button1 && button2) ;
  T1CON.F0 = 0;         // disable Timer1 module
}

// edit time and date function
char edit(char x, char y, char parameter)
{
  while(debounce());  // call debounce function (wait for B1 to be released)

  while(1) {

    while(!button2)    // if button B2 is pressed
    {
      parameter++;
      if(i == 0 && parameter > 23)   // if hours > 23 ==> hours = 0
        parameter = 0;
      if(i == 1 && parameter > 59)   // if minutes > 59 ==> minutes = 0
        parameter = 0;
      if(i == 2 && parameter > 31)   // if date > 31 ==> date = 1
        parameter = 1;
      if(i == 3 && parameter > 12)   // if month > 12 ==> month = 1
        parameter = 1;
      if(i == 4 && parameter > 99)   // if year > 99 ==> year = 0
        parameter = 0;

      LCD_Goto(x, y);    // move cursor to column x, row y
      LCD_PutC(parameter / 10 + '0');
      LCD_PutC(parameter % 10 + '0');
      delay_ms(200);

    }

    LCD_Goto(x, y);   // move cursor to column x, row y
    LCD_Print("  ");  // print 2 spaces
    delay();

    LCD_Goto(x, y);
    LCD_PutC(parameter / 10 + '0');
    LCD_PutC(parameter % 10 + '0');
    delay();

    if(!button1)     // if button B1 is pressed
    if(debounce())   // call debounce function (make sure B1 is pressed)
    {
      i++;   // increment 'i' for the next parameter
      return parameter;     // return parameter value and exit
    }

  }

}
/********************** end RTC functions *****************/

// main function
void main()
{
  //OSCCON = 0x70;  // set internal oscillator to 8MHz

  ADCON1 = 0x0F;     // configure all PORTA pins as digital



  // Timer1 module configuration
  T1CON  = 0x30;   // set Timer1 clock source to internal with 1:8 prescaler
                   // Timer1 clock frequency = 250kHz
  TMR1H  = TMR1L = 0;  // reset Timer1

  I2C1_Init(100000);   // initialize I2C bus with clock frequency of 100kHz

  LCD_Begin(0x4E);    // initialize LCD module with I2C address = 0x4E

  LCD_Goto(1, 1);   // move cursor to column 1, row 1
  LCD_Print("TIME: 00:00:00");
  LCD_Goto(1, 2);   // move cursor to column 1, row 2
  LCD_Print("DATE: 00/00/2020");

  while(1) {

    if(!button1)     // if button B1 is pressed
    if(debounce())   // call debounce function (make sure B1 is pressed)
    {
      i = 0;
      hour   = edit(7,  1, hour);
      minute = edit(10, 1, minute);
      m_day  = edit(7,  2, m_day);
      month  = edit(10, 2, month);
      year   = edit(15, 2, year);

      while(debounce());  // call debounce function (wait for button B1 to be released)

      // convert decimal to BCD
      minute = decimal_to_bcd(minute);
      hour   = decimal_to_bcd(hour);
      m_day  = decimal_to_bcd(m_day);
      month  = decimal_to_bcd(month);
      year   = decimal_to_bcd(year);
      // end conversion

      // write data to RTC chip
      I2C1_Start();         // start I2C
      I2C1_Wr(0xD0);     // RTC chip address
      I2C1_Wr(0);        // send register address
      I2C1_Wr(0);        // reset seconds and start oscillator
      I2C1_Wr(minute);   // write minute value to RTC chip
      I2C1_Wr(hour);     // write hour value to RTC chip
      I2C1_Wr(1);        // write day value (not used)
      I2C1_Wr(m_day);    // write date value to RTC chip
      I2C1_Wr(month);    // write month value to RTC chip
      I2C1_Wr(year);     // write year value to RTC chip
      I2C1_Stop();          // stop I2C

      delay_ms(20);

    }

    // read current time and date from the RTC chip
    I2C1_Start();           // start I2C
    I2C1_Wr(0xD0);       // RTC chip address
    I2C1_Wr(0);          // send register address
    I2C1_Repeated_Start();         // restart I2C
    I2C1_Wr(0xD1);       // initialize data read
    second = I2C1_Rd(1);  // read seconds from register 0
    minute = I2C1_Rd(1);  // read minutes from register 1
    hour   = I2C1_Rd(1);  // read hour from register 2
    I2C1_Rd(1);           // read day from register 3 (not used)
    m_day  = I2C1_Rd(1);  // read date from register 4
    month  = I2C1_Rd(1);  // read month from register 5
    year   = I2C1_Rd(0);  // read year from register 6
    I2C1_Stop();            // stop I2C

    RTC_display();  // print time & date

    delay_ms(50);   // wait 50 ms

  }

}
// end of code.