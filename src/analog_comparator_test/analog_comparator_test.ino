#include <avr/io.h> // AVR/IO.H header file
#include <avr/wdt.h>

//#define F_CPU 16000000UL     // Setting CPU Frequency 16MHz
#define COMPARATOR_REG ACSR
#define DATAIN ACO
#define INPUT_IS_SET (bit_is_set(COMPARATOR_REG, DATAIN))
#define INPUT_IS_CLEAR (bit_is_clear(COMPARATOR_REG, DATAIN))
#define BIT_1_HOLD_ON_LENGTH 40 // 20 us * 2 counts / us
#define BIT_0_HOLD_ON_LENGTH 64 // 33 us * 2 counts / us
#define START_HOLD_ON_LENGTH 80 // 150 us * 2 counts / us
#define BUFFER_SIZE 25
#define DATA_SIZE_MAX 15

static bool ParityBit;
static bool Broadcast;
static word MasterAddress;
static word SlaveAddress;
static byte Control;
static byte DataSize;
static byte Data[DATA_SIZE_MAX];

struct AVCMessage
{
    bool ParityBit;
    bool Broadcast;
    word MasterAddress;
    word SlaveAddress;
    byte Control;
    byte DataSize;
    byte Data[DATA_SIZE_MAX];
};

byte buffer_ptr = 0;
AVCMessage buffer[BUFFER_SIZE];

void setup()
{
    DDRB = 0x21;   // Bit 0 as output
    PORTB = 0x00;  // Clear all bits
    ACSR = 0x00;   // Clear bits of ACSR
    ADCSRB = 0x00; // Clear bits of ADCSRB

    // Watchdog timer enable
    wdt_enable(WDTO_2S);
    // Timer 0 prescaler = 8
    // (16e6 cycles/second) / (8 cycles/count)  / (1e6 us/second)
    //   = ( 2 count / us )
    TCCR0B = _BV(CS01);

    // Logging
    Serial.begin(115200);
    Serial.println("Setup started:");
    delay(250);
    Serial.println("Finished setup. Collecting data...");
}

AVCMessage msg;
void loop()
{
    wdt_reset();
    bool valid = AvcReadMessage();
    // bool valid = true;
    bool condition = (MasterAddress == 272 && SlaveAddress == 454) ||
                     (MasterAddress == 454 && SlaveAddress == 272);
    if (valid & condition)
    {
        // make a new AVC message
        buffer[buffer_ptr].MasterAddress = MasterAddress;
        buffer[buffer_ptr].SlaveAddress = SlaveAddress;
        buffer[buffer_ptr].Control = Control;
        buffer[buffer_ptr].DataSize = DataSize;
        for (int i = 0; i < DataSize; i++)
        {
            buffer[buffer_ptr].Data[i] = Data[i];
        }
        buffer_ptr += 1;
    }

    if (buffer_ptr == BUFFER_SIZE)
    {
        Serial.println("----------Data dump----------");
        for (int j = 0; j < BUFFER_SIZE; j++)
        {
            AVCMessage message = buffer[j];
            Serial.print(message.MasterAddress);
            Serial.print(' ');
            Serial.print(message.SlaveAddress);
            Serial.print(' ');
            Serial.print(message.Control);
            Serial.print('{');
            for (int i = 0; i < message.DataSize; i++)
            {
                Serial.print(message.Data[i]);
                Serial.print(' ');
            }
            Serial.println('}');
        }
        Serial.println("-----------------------------");
        buffer_ptr = 0;
    }
}

inline void LedOn(void)
{
    PORTB |= 0x20;
}
inline void LedOff(void)
{
    PORTB &= ~0x20;
}

// Waits until a valid start bit
bool getStart()
{
    // wait until the signal goes high
    while (INPUT_IS_CLEAR)
    {
        wdt_reset();
    }
    TCNT0 = 0; // start the count
    while (INPUT_IS_SET)
        ; // wait for the signal to go low
    // check how long the level was held high
    if (TCNT0 > START_HOLD_ON_LENGTH)
    {
        // found valid start, continue
        return true;
    }
    return false;
}

word ReadBits(byte nbBits)
{
    word data = 0;

    ParityBit = 0;

    while (nbBits-- > 0)
    {
        // Insert new bit
        data <<= 1;

        // Wait until rising edge of new bit.
        while (INPUT_IS_CLEAR)
        {
            // Reset watchdog.
            wdt_reset();
        }

        // Reset timer to measure bit length.
        TCNT0 = 0;

        // Wait until falling edge.
        while (INPUT_IS_SET)
            ;

        // Compare half way between a '1' (20 us) and a '0' (32 us ): 32 - (32 - 20) /2 = 26 us
        if (TCNT0 < BIT_0_HOLD_ON_LENGTH - (BIT_0_HOLD_ON_LENGTH - BIT_1_HOLD_ON_LENGTH) / 2)
        {
            // Set new bit.
            data |= 0x0001;

            // Adjust parity.
            ParityBit = !ParityBit;
        }
    }

    return data;
}

bool AvcReadMessage(void)
{
    while (!getStart())
        ; // wait for a valid start bit

    LedOn();

    Broadcast = ReadBits(1);

    MasterAddress = ReadBits(12);
    bool p = ParityBit;
    if (p != ReadBits(1))
    {
        // UsartPutCStr( PSTR("AvcReadMessage: Parity error @ MasterAddress!\r\n") );
        // Serial.println("Parity Error # MasterAddress");
        return false;
        // return (AvcActionID)FALSE;
    }

    SlaveAddress = ReadBits(12);
    p = ParityBit;
    if (p != ReadBits(1))
    {
        // UsartPutCStr( PSTR("AvcReadMessage: Parity error @ SlaveAddress!\r\n") );
        // Serial.println("Parity error @ SlaveAddress");
        return false;
        // return (AvcActionID)FALSE;
    }

    // bool forMe = ( SlaveAddress == MY_ADDRESS );
    bool forMe = false;

    // In point-to-point communication, sender issues an ack bit with value '1' (20us). Receiver
    // upon acking will extend the bit until it looks like a '0' (32us) on the bus. In broadcast
    // mode, receiver disregards the bit.

    if (forMe)
    {
        // Send ACK.
        // Send1BitWord( 0 );
    }
    else
    {
        ReadBits(1);
    }

    Control = ReadBits(4);
    p = ParityBit;
    if (p != ReadBits(1))
    {
        // UsartPutCStr( PSTR("AvcReadMessage: Parity error @ Control!\r\n") );
        // Serial.println("Parity error @ Control");
        return false;
        // return (AvcActionID)FALSE;
    }

    if (forMe)
    {
        // Send ACK.
        // Send1BitWord( 0 );
    }
    else
    {
        ReadBits(1);
    }

    DataSize = ReadBits(8);
    p = ParityBit;
    if (p != ReadBits(1) || DataSize > DATA_SIZE_MAX)
    {
        // UsartPutCStr( PSTR("AvcReadMessage: Parity error @ DataSize!\r\n") );
        // Serial.println("Parity error @ DataSize");
        return false;
        // return (AvcActionID)FALSE;
    }

    if (forMe)
    {
        // Send ACK.
        // Send1BitWord( 0 );
    }
    else
    {
        ReadBits(1);
    }

    byte i;

    for (i = 0; i < DataSize; i++)
    {
        Data[i] = ReadBits(8);
        p = ParityBit;
        if (p != ReadBits(1))
        {
            // sprintf( UsartMsgBuffer, "AvcReadMessage: Parity error @ Data[%d]\r\n", i );
            // Serial.println("Parity error @ Data");
            return false;
            // UsartPutStr( UsartMsgBuffer );
            // return (AvcActionID)FALSE;
        }

        if (forMe)
        {
            // Send ACK.
            // Send1BitWord( 0 );
        }
        else
        {
            ReadBits(1);
        }
    }

    // Dump message on terminal.
    // if ( forMe ) UsartPutCStr( PSTR("AvcReadMessage: This message is for me!\r\n") );

    // AvcActionID actionID = GetActionID();

    // switch ( actionID ) {
    //   case /* value */:
    // }
    // DumpRawMessage( FALSE );

    LedOff();
    return true;
    // return actionID;
}
