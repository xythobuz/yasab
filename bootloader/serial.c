/*
 * serial.c
 *
 * Copyright (c) 2012, Thomas Buck <xythobuz@me.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "serial.h"

#define XON 0x11
#define XOFF 0x13

#if (RX_BUFFER_SIZE < 8) || (TX_BUFFER_SIZE < 8)
#error SERIAL BUFFER TOO SMALL!
#endif

#if (RX_BUFFER_SIZE + TX_BUFFER_SIZE) >= (RAMEND - 0x60)
#error SERIAL BUFFER TOO LARGE!
#endif

#define FLOWMARK 5

uint8_t volatile rxBuffer[RX_BUFFER_SIZE];
uint8_t volatile txBuffer[TX_BUFFER_SIZE];
uint16_t volatile rxRead = 0;
uint16_t volatile rxWrite = 0;
uint16_t volatile txRead = 0;
uint16_t volatile txWrite = 0;
uint8_t volatile shouldStartTransmission = 1;

uint8_t volatile sendThisNext = 0;
uint8_t volatile flow = 1;
uint8_t volatile rxBufferElements = 0;

#ifdef AUTOMAGICDETECTION
#define UNKNOWN 2
#define FIRST 0
#define SECOND 1
uint8_t detectedStatus = UNKNOWN;
#endif

ISR(SERIALRECIEVEINTERRUPT) { // Receive complete
#ifdef AUTOMAGICDETECTION
    if (detectedStatus == UNKNOWN) {
        detectedStatus = FIRST;
    }
    if (detectedStatus == FIRST) {
#endif
        rxBuffer[rxWrite] = SERIALDATA;
        if (rxWrite < (RX_BUFFER_SIZE - 1)) {
            rxWrite++;
        } else {
            rxWrite = 0;
        }

        rxBufferElements++;
        if ((flow == 1) && (rxBufferElements >= (RX_BUFFER_SIZE - FLOWMARK))) {
            sendThisNext = XOFF;
            flow = 0;
            if (shouldStartTransmission) {
                shouldStartTransmission = 0;
                SERIALB |= (1 << SERIALUDRIE);
                SERIALA |= (1 << SERIALUDRE); // Trigger Interrupt
            }
        }
#ifdef AUTOMAGICDETECTION
    }
#endif
}

#ifdef AUTOMAGICDETECTION
ISR(SERIALRECIEVEINTERRUPTB) { // Receive complete
    if (detectedStatus == UNKNOWN) {
        detectedStatus = SECOND;
    }
    if (detectedStatus == SECOND) {
        rxBuffer[rxWrite] = SERIALDATAB;
        if (rxWrite < (RX_BUFFER_SIZE - 1)) {
            rxWrite++;
        } else {
            rxWrite = 0;
        }

        rxBufferElements++;
        if ((flow == 1) && (rxBufferElements >= (RX_BUFFER_SIZE - FLOWMARK))) {
            sendThisNext = XOFF;
            flow = 0;
            if (shouldStartTransmission) {
                shouldStartTransmission = 0;
                SERIALBB |= (1 << SERIALUDRIEB);
                SERIALAB |= (1 << SERIALUDREB); // Trigger Interrupt
            }
        }
    }
}
#endif

ISR(SERIALTRANSMITINTERRUPT) { // Data register empty
#ifdef AUTOMAGICDETECTION
    if (detectedStatus == FIRST) {
#endif
        if (sendThisNext) {
            SERIALDATA = sendThisNext;
            sendThisNext = 0;
        } else {
            if (txRead != txWrite) {
                SERIALDATA = txBuffer[txRead];
                if (txRead < (TX_BUFFER_SIZE -1)) {
                    txRead++;
                } else {
                    txRead = 0;
                }
            } else {
                shouldStartTransmission = 1;
                SERIALB &= ~(1 << SERIALUDRIE); // Disable Interrupt
            }
        }
#ifdef AUTOMAGICDETECTION
    }
#endif
}

#ifdef AUTOMAGICDETECTION
ISR(SERIALTRANSMITINTERRUPTB) { // Data register empty
    if (detectedStatus == SECOND) {
        if (sendThisNext) {
            SERIALDATAB = sendThisNext;
            sendThisNext = 0;
        } else {
            if (txRead != txWrite) {
                SERIALDATAB = txBuffer[txRead];
                if (txRead < (TX_BUFFER_SIZE -1)) {
                    txRead++;
                } else {
                    txRead = 0;
                }
            } else {
                shouldStartTransmission = 1;
                SERIALBB &= ~(1 << SERIALUDRIEB); // Disable Interrupt
            }
        }
    }
}
#endif

void serialInit(uint16_t baud) {
    // Default: 8N1
    SERIALC = (1 << SERIALUCSZ0) | (1 << SERIALUCSZ1);

    // Set baudrate
#ifdef SERIALBAUD8
    SERIALUBRRH = (baud >> 8);
    SERIALUBRRL = baud;
#else
    SERIALUBRR = baud;
#endif

    SERIALB = (1 << SERIALRXCIE); // Enable Interrupts
    SERIALB |= (1 << SERIALRXEN) | (1 << SERIALTXEN); // Enable Receiver/Transmitter

#ifdef AUTOMAGICDETECTION
    // Also initialize second UART
    SERIALCB = (1 << SERIALUCSZ0B) | (1 << SERIALUCSZ1B);
#ifdef SERIALBAUD8
    SERIALUBRRHB = (baud >> 8);
    SERIALUBRRLB = baud;
#else
    SERIALUBRRB = baud;
#endif
    SERIALBB = (1 << SERIALRXCIEB) | (1 << SERIALRXENB) | (1 << SERIALTXENB);
#endif
}

void serialClose(void) {
    SERIALB = 0;
    SERIALC = 0;
#ifdef AUTOMAGICDETECTION
    SERIALBB = 0;
    SERIALCB = 0;
#endif
}

void setFlow(uint8_t on) {
    if (flow != on) {
        if (on == 1) {
            // Send XON
            while (sendThisNext != 0);
            sendThisNext = XON;
            flow = 1;
            if (shouldStartTransmission) {
                shouldStartTransmission = 0;
#ifdef AUTOMAGICDETECTION
                if (detectedStatus == FIRST) {
#endif
                    SERIALB |= (1 << SERIALUDRIE);
                    SERIALA |= (1 << SERIALUDRE); // Trigger Interrupt
#ifdef AUTOMAGICDETECTION
                } else if (detectedStatus == SECOND) {
                    SERIALBB |= (1 << SERIALUDRIEB);
                    SERIALAB |= (1 << SERIALUDREB);
                }
#endif
            }
        } else {
            // Send XOFF
            sendThisNext = XOFF;
            flow = 0;
            if (shouldStartTransmission) {
                shouldStartTransmission = 0;
#ifdef AUTOMAGICDETECTION
                if (detectedStatus == FIRST) {
#endif
                    SERIALB |= (1 << SERIALUDRIE);
                    SERIALA |= (1 << SERIALUDRE); // Trigger Interrupt
#ifdef AUTOMAGICDETECTION
                } else if (detectedStatus == SECOND) {
                    SERIALBB |= (1 << SERIALUDRIEB);
                    SERIALAB |= (1 << SERIALUDREB);
                }
#endif
            }
        }
#ifdef AUTOMAGICDETECTION
        if (detectedStatus == FIRST) {
#endif
            while (SERIALB & (1 << SERIALUDRIE)); // Wait till it's transmitted
#ifdef AUTOMAGICDETECTION
        } else if (detectedStatus == SECOND) {
            while (SERIALBB & (1 << SERIALUDRIEB));
        }
#endif
    }
}

// ---------------------
// |     Reception     |
// ---------------------

uint8_t serialHasChar(void) {
    if (rxRead != rxWrite) { // True if char available
        return 1;
    } else {
        return 0;
    }
}

uint8_t serialGetBlocking(void) {
    while(!serialHasChar());
    return serialGet();
}

uint8_t serialGet(void) {
    uint8_t c;

    rxBufferElements--;
    if ((flow == 0) && (rxBufferElements <= FLOWMARK)) {
        while (sendThisNext != 0);
        sendThisNext = XON;
        flow = 1;
        if (shouldStartTransmission) {
            shouldStartTransmission = 0;
#ifdef AUTOMAGICDETECTION
            if (detectedStatus == FIRST) {
#endif
                SERIALB |= (1 << SERIALUDRIE);
                SERIALA |= (1 << SERIALUDRE); // Trigger Interrupt
#ifdef AUTOMAGICDETECTION
            } else if (detectedStatus == SECOND) {
                SERIALBB |= (1 << SERIALUDRIE);
                SERIALAB |= (1 << SERIALUDRE);
            }
#endif
        }
    }

    if (rxRead != rxWrite) {
        c = rxBuffer[rxRead];
        rxBuffer[rxRead] = 0;
        if (rxRead < (RX_BUFFER_SIZE - 1)) {
            rxRead++;
        } else {
            rxRead = 0;
        }
        return c;
    } else {
        return 0;
    }
}

// ----------------------
// |    Transmission    |
// ----------------------

void serialWrite(uint8_t data) {
    if (data == '\n') {
        serialWrite('\r');
    }
    while (serialTxBufferFull());

    uint8_t sreg = SREG;
    cli();
    txBuffer[txWrite] = data;
    if (txWrite < (TX_BUFFER_SIZE - 1)) {
        txWrite++;
    } else {
        txWrite = 0;
    }
    if (shouldStartTransmission) {
        shouldStartTransmission = 0;
#ifdef AUTOMAGICDETECTION
        if (detectedStatus != SECOND) { // Sane default for debugging
#endif
            SERIALB |= (1 << SERIALUDRIE); // Enable Interrupt
            SERIALA |= (1 << SERIALUDRE); // Trigger Interrupt
#ifdef AUTOMAGICDETECTION
        } else {
            SERIALBB |= (1 << SERIALUDRIE); // Enable Interrupt
            SERIALAB |= (1 << SERIALUDRE); // Trigger Interrupt
        }
#endif
    }
    SREG = sreg;
}

void serialWriteString(const char *data) {
    while (*data != '\0') {
        serialWrite(*data++);
    }
}

uint8_t serialTxBufferFull(void) {
    return (((txWrite + 1) == txRead) || ((txRead == 0) && ((txWrite + 1) == TX_BUFFER_SIZE)));
}

uint8_t serialTxBufferEmpty(void) {
    if (txRead != txWrite) {
        return 0;
    } else {
        return 1;
    }
}
