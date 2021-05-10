 /* This library is an extension of the code discussed at the following link:
  *  
  *  http://forum.arduino.cc/index.php?topic=156812.0
  *  
  *  The purpose of this code is to provide a test and configure interface for the Avago AEAT-6600-T16 magnetic encoder.
  *  It is configured to read the encoder with default settings. i.e. 10bit at an SSI rate of approx. 400kHz which is limited by the arduino.
  *  
  *  Datasheet Sheet: https://docs.broadcom.com/doc/AV02-2792EN
  *  Application Notes: https://docs.broadcom.com/wcs-public/products/application-notes/application-note/696/604/av02-2791en_an_5501_aeat-6600_2014-04-21.pdf
  *  
  *  Written By : Andrew Becker
  *  Full repository available at https://github.com/4nswer/AEAT-6600-T16
  */

const int MAG_HI = 8,
            MAG_LO = 9,
            ALIGN = 2,
            PWR_DN = 3,
            PROG = 4,
            NCS = 5,
            CLOCK_PIN = 6, //The clock pin is also accessed directly as PORTD{7} in the readPosition() function. Changing the code only here will not work.
            DATA_PIN = 7, //The data pin is also accessed directly as PINE{6} in the readPosition() function. Changing the code only here will not work.
            BIT_COUNT = 10; // this's the precision of rotary encoder.

void setup() {
  pinMode(MAG_HI, INPUT);
  pinMode(MAG_LO, INPUT);
  pinMode(ALIGN,OUTPUT);
  pinMode(PWR_DN,OUTPUT);
  pinMode(PROG,OUTPUT);
  pinMode(NCS,OUTPUT);
  pinMode(DATA_PIN, INPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  digitalWrite(CLOCK_PIN, HIGH);
  Serial.begin(115200);
  Serial.println("Options: a - position, b - magnetic field strength, c - alignment test, d - programming, e - exit current mode");
}

void loop() {
  if (Serial.available())
  {
    switch (Serial.read())
    {
    case 'a':
      while (Serial.read()!='e')
      {
        Serial.println(readPosition(),2);
        delay(10);
      }
      break;

    case 'b':
      while (Serial.read()!='e')
      {
        if (digitalRead(MAG_HI))
        {
          Serial.println("Magnetic field intensity is too high");
        }
        if (digitalRead(MAG_LO))
        {
          Serial.println("Magnetic field intensity is too low");
        }
        if (!digitalRead(MAG_HI)&!digitalRead(MAG_LO))
        {
          Serial.print("Magnetic field intensity is correct. ");
          Serial.print("Position Reading is: ");
          Serial.println(readPosition(),2);
        }
        if (digitalRead(MAG_HI)&digitalRead(MAG_LO))
        {
          Serial.println("Fault, both pins reading high which should not be possible");
        }
        delay(200);
      }
      break;

      case 'c':
        while (Serial.read()!='e')
        {
          digitalWrite(ALIGN,HIGH);
          Serial.print("The alignment value is: ");
          Serial.println(SSI_Shift_In(DATA_PIN, CLOCK_PIN, 16));
          delay(100);
        }
        digitalWrite(ALIGN,LOW);
      break;

      case 'd':
      int32_t out1= 0b00000000000000000100101100000111;
      Serial.print(out1);
      SSI_Shift_Out(32,out1);
      break;

    default:
      break;
    }
  }
}

//read the current angular position
float readPosition() {
  unsigned long rawData = SSI_Shift_In(DATA_PIN, CLOCK_PIN, BIT_COUNT);
  delayMicroseconds(25);  // Clock must be high for at least 20 microseconds before a new sample can be taken
  return ((rawData & 0x03FF) * 360UL) / 1024.0; // Return reading scaled to 360deg with two point precision
}

//read in a byte of data from the digital input of the board.
unsigned long SSI_Shift_In(const int data_pin, const int clock_pin, const int bit_count)
{
  unsigned long data = 0;
  // First tick is to initialise the transfer, no data is read.
  PORTD &= ~(1 << 7); // clock pin low
  delayMicroseconds(2);
  PORTD |= (1 << 7); // clock pin high
  delayMicroseconds(2);
  for (int i=0; i<bit_count; i++) {
    data <<= 1; // shift all read data left one bit.
    PORTD &= ~(1 << 7); // clock pin low
    delayMicroseconds(2);
    PORTD |= (1 << 7); // clock pin high
    delayMicroseconds(1); // This is shorter than for the low time to account for the time read the port below. As it stands the time taken to read the data is about 1.5us so the delay is mostly for stability.
    data |= (bitRead(PINE,6)); // Append the new read bit to data.
    /* bitRead(port,bit) is defined in arduino.h and takes approximately 1us.
     * data |= digitalRead(data_pin); <- takes about 4us and limits the SSI rate to less than 100kHz. It is also increases the jitter.
     */
    }
  return data;
}

void SSI_Shift_Out(const int bit_count, int progData) {
  pinMode(DATA_PIN, OUTPUT);
  PORTD &= ~(1 << 7); // clock pin low
  bitWrite (PORTE,6,1);
  delayMicroseconds(1);
  PORTD |= (1 << 7); // clock pin high
  for (int i=0; i<bit_count+2; i++) {
    PORTD &= ~(1 << 7); // clock pin low
    delayMicroseconds(1);
    if (i==0)
    {
      bitWrite (PORTE,6,1);
    }
    if (i==bit_count+2)
    {
      bitWrite (PORTE,6,1);
    }
    if ((i > 0) && i<bit_count+2)
    {
      bitWrite (PORTE,6, bitRead(progData,i));
    }
    PORTD |= (1 << 7); // clock pin high
    delayMicroseconds(2); // This is shorter than for the low time to account for the time write the port below.
    //data |= (bitRead(PINE,6)); // append the new read bit to the whole read data.
    /*bitRead(port,bit) is defined in arduino.h and takes approximately 1us.
     * data |= digitalRead(data_pin); <- takes about 4us and limits the SSI rate to 100kHz. It is also increases the jitter.
     */
  }
  pinMode(DATA_PIN, INPUT);
}


