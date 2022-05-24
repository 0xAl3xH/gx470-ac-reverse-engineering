#include<avr/io.h>           // AVR/IO.H header file
#include <avr/wdt.h>

//#define F_CPU 16000000UL     // Setting CPU Frequency 16MHz
#define COMPARATOR_REG              ACSR
#define DATAIN                  ACO
#define INPUT_IS_SET            ( bit_is_set( COMPARATOR_REG, DATAIN ) )
#define INPUT_IS_CLEAR          ( bit_is_clear( COMPARATOR_REG, DATAIN ) )
#define BIT_1_HOLD_ON_LENGTH        40 //20*2
#define BIT_0_HOLD_ON_LENGTH        64 //33*2

static bool         ParityBit;
static bool         Broadcast;
static word         MasterAddress;
static word         SlaveAddress;
static byte         Control;
static byte         DataSize;
static byte         Data[ 256 ];

void setup() {
  // put your setup code here, to run once:
  DDRB = 0x21;               // Bit 0 as output
  PORTB = 0x00;              // Clear all bits
  ACSR = 0x00;               // Clear bits of ACSR 
  ADCSRB = 0x00;             // Clear bits of ADCSRB

  // Watchdog timer enable
  wdt_enable( WDTO_2S );
  // Timer 0 prescaler = 8 
  // (16e6 cycles/second) / (8 cycles/count)  / (1e6 us/second)
  //   = ( 2 count / us )
  TCCR0B = _BV(CS01);

  //Logging
  Serial.begin(115200);
  Serial.println("Setup started :");
  delay(250);
  Serial.println("Finished setup.");
}

void loop() {
  // put your main code here, to run repeatedly:
//  if( (ACSR & 0x20) == 0)    // Check ACO (output bit)
//  {
//    PORTB = 0x21;            // Turn on LED1
//  } 
//  else 
//  {
//    PORTB = 0x00;            // Turn off LED1
//  }
  wdt_reset();
  bool valid = AvcReadMessage();
  bool condition = (MasterAddress == 272 && SlaveAddress == 454) ||
                   (MasterAddress == 454 && SlaveAddress == 272);
  if (valid && condition) {
  //lif (valid) {
    Serial.print(MasterAddress);
    Serial.print(' ');
    Serial.print(SlaveAddress);
    Serial.print(' ');
    Serial.print(Control);
    Serial.print('{');
    for(int i = 0; i < DataSize; i++) {
      Serial.print(Data[i]);
      Serial.print(' ');
    }
    Serial.println('}');
    Serial.println("---------");
  }
}

inline void LedOn ( void )
{
    PORTB |= 0x20;
}
inline void LedOff ( void )
{
    PORTB &= ~0x20;
}

word ReadBits ( byte nbBits )
{
    word data = 0;

    ParityBit = 0;

    while ( nbBits-- > 0 )
    {
        // Insert new bit
        data <<= 1;

        // Wait until rising edge of new bit.
        while ( INPUT_IS_CLEAR )
        {
            // Reset watchdog.
            wdt_reset();
        }

        // Reset timer to measure bit length.
        TCNT0 = 0;

        // Wait until falling edge.
        while ( INPUT_IS_SET );

        // Compare half way between a '1' (20 us) and a '0' (32 us ): 32 - (32 - 20) /2 = 26 us
        if ( TCNT0 < BIT_0_HOLD_ON_LENGTH - (BIT_0_HOLD_ON_LENGTH - BIT_1_HOLD_ON_LENGTH) / 2 )
        {
            // Set new bit.
            data |= 0x0001;

            // Adjust parity.
            ParityBit = ! ParityBit;
        }
    }

    return data;
}

bool AvcReadMessage ( void )
{
    ReadBits( 1 ); // Start bit.

    LedOn();

    Broadcast = ReadBits( 1 );

    MasterAddress = ReadBits( 12 );
    bool p = ParityBit;
    if ( p != ReadBits( 1 ) )
    {
        //UsartPutCStr( PSTR("AvcReadMessage: Parity error @ MasterAddress!\r\n") );
        //Serial.println("Parity Error # MasterAddress");
        return false;
        //return (AvcActionID)FALSE;
    }

    SlaveAddress = ReadBits( 12 );
    p = ParityBit;
    if ( p != ReadBits( 1 ) )
    {
        //UsartPutCStr( PSTR("AvcReadMessage: Parity error @ SlaveAddress!\r\n") );
        //Serial.println("Parity error @ SlaveAddress");
        return false;
        //return (AvcActionID)FALSE;
    }

    //bool forMe = ( SlaveAddress == MY_ADDRESS );
    bool forMe = false;

    // In point-to-point communication, sender issues an ack bit with value '1' (20us). Receiver
    // upon acking will extend the bit until it looks like a '0' (32us) on the bus. In broadcast
    // mode, receiver disregards the bit.

    if ( forMe )
    {
        // Send ACK.
        //Send1BitWord( 0 );
    }
    else
    {
        ReadBits( 1 );
    }

    Control = ReadBits( 4 );
    p = ParityBit;
    if ( p != ReadBits( 1 ) )
    {
        //UsartPutCStr( PSTR("AvcReadMessage: Parity error @ Control!\r\n") );
        //Serial.println("Parity error @ Control");
        return false;
        //return (AvcActionID)FALSE;
    }

    if ( forMe )
    {
        // Send ACK.
        //Send1BitWord( 0 );
    }
    else
    {
        ReadBits( 1 );
    }

    DataSize = ReadBits( 8 );
    p = ParityBit;
    if ( p != ReadBits( 1 ) )
    {
        //UsartPutCStr( PSTR("AvcReadMessage: Parity error @ DataSize!\r\n") );
        //Serial.println("Parity error @ DataSize");
        return false;
        //return (AvcActionID)FALSE;
    }

    if ( forMe )
    {
        // Send ACK.
        //Send1BitWord( 0 );
    }
    else
    {
        ReadBits( 1 );
    }

    byte i;

    for ( i = 0; i < DataSize; i++ )
    {
        Data[i] = ReadBits( 8 );
        p = ParityBit;
        if ( p != ReadBits( 1 ) )
        {
            //sprintf( UsartMsgBuffer, "AvcReadMessage: Parity error @ Data[%d]\r\n", i );
            //Serial.println("Parity error @ Data");
            return false;
            //UsartPutStr( UsartMsgBuffer );
            //return (AvcActionID)FALSE;
        }

        if ( forMe )
        {
            // Send ACK.
            //Send1BitWord( 0 );
        }
        else
        {
            ReadBits( 1 );
        }
    }

    // Dump message on terminal.
    //if ( forMe ) UsartPutCStr( PSTR("AvcReadMessage: This message is for me!\r\n") );

    //AvcActionID actionID = GetActionID();

    // switch ( actionID ) {
    //   case /* value */:
    // }
    //DumpRawMessage( FALSE );

    LedOff();
    return true;
    //return actionID;
}
