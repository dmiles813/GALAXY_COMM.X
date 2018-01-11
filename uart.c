#include <xc.h>
#include "app.h"
#include "uart.h"

//===============================================================================
//	Description:	This function initializes the internal UART.
//
//	Params:			BAUD				ULONG		Desired baud rate
//						MODE_9BIT		UCHAR		Cofnigure 8 or 9 bit mode
//						INTERRUPT_C...	UCHAR		Configure interrupt control
//
//	Returns:			NONE
//===============================================================================
void UART_Initialize (	unsigned char uart_index,
                        unsigned long baud, 
                        unsigned char mode_9bit, 
                        unsigned char interrupt_control )
{
	unsigned long temp = 0;
	unsigned long clock_freq = _XTAL_FREQ;

	/*
	RECEPTION

	1.		Initialize the SPBRGH:SPBRG registers for the appropriate baud rate.
			Set or clear the BRGH and BRG16 bits, as required, to achieve the desired baud rate.
	2.		Enable the asynchronous serial port by clearing bit SYNC and setting bit SPEN.
	3.		If interrupts are desired, set enable bit RCIE.
	4.		If 9-bit reception is desired, set bit RX9.
	5.		Enable the reception by setting bit CREN.
	6.		Flag bit, RCIF, will be set when reception is complete and an interrupt will be
			generated if enable bit, RCIE, was set.
	7.		Read the RCSTA register to get the 9th bit (if enabled) and determine if any error
			occurred during reception.
	8.		Read the 8-bit received data by reading the RCREG register.
	9.		If any error occurred, clear the error by clearing enable bit CREN.
	10.	If using interrupts, ensure that the GIE and PEIE bits in the INTCON register
			(INTCON<7:6>) are set.

	TRANSMISSION

	1.		Initialize the SPBRGH:SPBRG registers for the appropriate baud rate.
			Set or clear the BRGH and BRG16 bits, as required, to achieve the desired baud rate.
	2.		Enable the asynchronous serial port by clearing bit SYNC and setting bit SPEN.
	3. 	If interrupts are desired, set enable bit TXIE.
	4.		If 9-bit transmission is desired, set transmit bit TX9. Can be used as address/data bit.
	5. 	Enable the transmission by setting bit TXEN which will also set bit TXIF.
	6. 	If 9-bit transmission is selected, the ninth bit should be loaded in bit TX9D.
	7.		Load data to the TXREG register (starts transmission).
	8. 	If using interrupts, ensure that the GIE and PEIE bits in the INTCON register (INTCON<7:6>)
			are set.
	*/

	// Set BRG Control
    if (uart_index == UART1_INDEX) {
        BAUDCON1bits.BRG16 = 1;
        TXSTA1bits.BRGH = 1;
    }

	// Calculate value for baud register, given the states of BRG16 and BRGH
	temp = clock_freq / baud;
    temp = (temp / 4);
	temp = temp - 1;
    if (uart_index == UART1_INDEX) {
        SPBRGH1 = temp >> 8;
        SPBRG1 = (char)temp;
    }
    
	// RX & TX must be configured as inputs for UART to work
    if (uart_index == UART1_INDEX) {
        TRISCbits.TRISC6 = 1;
        TRISCbits.TRISC7 = 1;
        TXSTA1bits.SYNC1 = 0;
    }
    
	// 9 bit reception & transmission (Enabled => 1)
	if (mode_9bit == UART_9BIT_MODE)
	{
        if (uart_index == UART1_INDEX) {
            RCSTA1bits.RX9 = 1;
            TXSTA1bits.TX9 = 1;
        }
	}
	else
	{
        if (uart_index == UART1_INDEX) {
            RCSTA1bits.RX9 = 0;
            TXSTA1bits.TX9 = 0;
        }
	}

    //BAUDCON1bits.DTRXP1 = 1;
    
		// Enable Receiver Hardware Circuitry
    if (uart_index == UART1_INDEX) {
        RCSTA1bits.CREN = 1;
    }

    // Enable Asynch Serial Port
    if (uart_index == UART1_INDEX) {
        RCSTA1bits.SPEN = 1;
    }
    
	// Clear RCIF, if set
    if (uart_index == UART1_INDEX) {
        if (PIR1bits.RC1IF)
            temp = RC1REG;
    }
    
	// Configure Interrupts for UART
	if (interrupt_control == UART_INTERRUPTS_LOW_PRI || interrupt_control == UART_INTERRUPTS_HIGH_PRI)
	{
		// Configure Low or High Priority for UART
		if (interrupt_control == UART_INTERRUPTS_LOW_PRI)
		{
            if (uart_index == UART1_INDEX) {
            	IPR1bits.RC1IP = 0;
                IPR1bits.TX1IP = 0;
            }
		}
		else if (interrupt_control == UART_INTERRUPTS_HIGH_PRI)
		{
            if (uart_index == UART1_INDEX) {
                IPR1bits.RC1IP = 1;
                IPR1bits.TX1IP = 1;
            }
		}

		// Enable Receive/Transmit Interrupts
		PIE1bits.RC1IE = 1;
		//PIE1bits.TX1IE = 1;

		// Enable Peripheral Device Interrupts
		INTCONbits.PEIE = 1;

		// Enable Global Interrupts
		INTCONbits.GIE = 1;
	}
	// Else, just disable receive and transmit interrupts
	else
	{
		// Disable Interrupts
		PIE1bits.RC1IE = 0;
		PIE1bits.TX1IE = 0;
        
        EnableTransmitter(uart_index);
	}

}

void EnableTransmitter(unsigned char uart_index) {
    if (uart_index == UART1_INDEX) {
        TXSTA1bits.TXEN1 = 1;
    }
}

void DisableTransmitter(unsigned char uart_index) {
    if (uart_index == UART1_INDEX) {
        TXSTA1bits.TXEN1 = 0;
    }
}

unsigned char IsTransmitterEnabled(unsigned char uart_index) {
    if (uart_index == UART1_INDEX) {
        if (TXSTA1bits.TXEN1) {
            return TRUE;
        }
    }
    return FALSE;
}

unsigned char IsTransmitterReady(unsigned char uart_index) {
    if (IsTransmitterEnabled(uart_index)) {
        return _GetTxInterruptFlag(uart_index);
    }
    return FALSE;
}

void PutChar9 (unsigned char uart_index, unsigned int data) {
    // Enable the transceiver
    EnableTransceiverTX(UART1_INDEX);

    // Ensure we are not waiting for a byte to sent
    while (!_GetTxInterruptFlag(uart_index));
    
    EnableTransceiverTX(uart_index);

    // If word is 9 bits, configure TX9D bit, and must be done first.
    if (data > 0x00FF) {
        if (uart_index == UART1_INDEX) {
            TXSTA1bits.TX9D1 = 1;
        }
    } else {
        if (uart_index == UART1_INDEX) {
            TXSTA1bits.TX9D1 = 0;
        }
    }

    if (uart_index == UART1_INDEX) {
        TXREG1 = (unsigned char)(data & 0x00FF);
    }
    
    // Wait for byte to be shifted
    while (!TXSTA1bits.TRMT);
    while (!TXSTA1bits.TRMT);
    DisableTransceiverTX(uart_index);

/*    while (!IsRxDataAvailable(uart_index));
    
    unsigned char foo = 1;
    unsigned int data2;
    // Deal with errors
    if (RCSTA1bits.FERR) {
        RCSTA1bits.CREN = 0;    // Clear the flag
        RCSTA1bits.CREN = 1;
    }
    if (RCSTA1bits.OERR) {
        RCSTA1bits.CREN = 0;    // Clear the flag
        RCSTA1bits.CREN = 1;
    }

    // Get buffer contents
    if (RCSTA1bits.RX9D1 == 1)
        data2 = 0x0100;
    else
        data2 = 0x0000;
    data2 += RCREG1;
*/
}

void PutChar9Default (unsigned int data) {
    PutChar9(UART_INDEX_DEFAULT, data);
}

void EnableTransceiverTX(unsigned char uart_index) {
    if (uart_index == UART1_INDEX) {
        PIN_UART1_TX_ENABLE_TRIS = 0;
        PIN_UART1_TX_ENABLE_LATCH = UART1_TX_LATCH_ACTIVE;
    }
}

void DisableTransceiverTX(unsigned char uart_index) {
    if (uart_index == UART1_INDEX) {
        PIN_UART1_TX_ENABLE_TRIS = 0;
        PIN_UART1_TX_ENABLE_LATCH = UART1_TX_LATCH_INACTIVE;    
    }
}

unsigned char _GetTxInterruptFlag(unsigned char uart_index) {
    if (uart_index == UART1_INDEX) {
        if (PIR1bits.TX1IF)
            return TRUE;
    }
    return FALSE;
}

unsigned char _GetRxInterruptFlag(unsigned char uart_index) {
    if (uart_index == UART1_INDEX) {
        if (PIR1bits.RC1IF)
            return TRUE;
    }
    return FALSE;
}

unsigned char IsRxDataAvailable(unsigned char uart_index) {
    if (uart_index == UART1_INDEX) {
        if (RCSTA1bits.SPEN1 && PIR1bits.RC1IF)
            return TRUE;
    }
    return FALSE;
}

unsigned int GetChar9(unsigned char uart_index) {
    if (uart_index == UART1_INDEX) {        
        if (IsRxDataAvailable(uart_index)) {
            unsigned char foo = 1;
            unsigned int data;
            // Deal with errors
            if (RCSTA1bits.FERR) {
                RCSTA1bits.CREN = 0;    // Clear the flag
                RCSTA1bits.CREN = 1;
                return 0x1000;
            }
            if (RCSTA1bits.OERR) {
                RCSTA1bits.CREN = 0;    // Clear the flag
                RCSTA1bits.CREN = 1;
                return 0x2000;
            }
            
            // Get buffer contents
            if (RCSTA1bits.RX9D1 == 1)
                data = 0x0100;
            else
                data = 0x0000;
            data += RCREG1;
            return data;
        }
    }
    return 0x4000;
}

unsigned int GetChar9Default() {
    return GetChar9(UART_INDEX_DEFAULT);
}

