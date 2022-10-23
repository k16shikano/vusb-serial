#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "usbdrv/usbdrv.h"
#define F_CPU 16000000
#include <util/delay.h>

#include <string.h>

static uchar rcvd[16];
//static uchar test[21] = "Hello from ATmega328\n";
static uchar rcvd_i = 0, rcvd_len = 0;
static int has_usb_message = 0;

volatile char buf[32];
volatile int head = 1;
volatile int buf_ready = 0;

ISR (USART_RX_vect) {
  uint8_t u8temp;
  u8temp = UDR0;
  if (head < 15) {
    buf[head++] = u8temp;
  } else {
    buf[head] = '\0';
    buf_ready = 1;
    head = 0;
  }
}

USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
  usbRequest_t *rq = (void *)data;
  rcvd_len = (uchar)rq->wLength.word;
  //  rcvd_i = 0;

  if((rq->bmRequestType & USBRQ_DIR_MASK) == USBRQ_DIR_DEVICE_TO_HOST) {
    uchar usart_rcvd[32] = "";
    if (buf_ready) {
      for (int i = 0; i < 16; i++) {
        if (buf[i] == '\n') {
          usart_rcvd[i++] = '\0';
          break;
        }
        usart_rcvd[i] = buf[i];
        buf_ready = 0;
        head = 1;
      }
    }
    usbMsgPtr = usart_rcvd;
    _delay_ms(1);
    return strlen((char*)usart_rcvd);
  } else {
    if (rcvd_len > sizeof(rcvd)) // limit to buffer size
      rcvd_len = sizeof(rcvd);
    return USB_NO_MSG;
  }

  return 0;
}

USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len) {

  for (uchar i = 0; rcvd_i < rcvd_len && i < len; i++, rcvd_i++) {
    rcvd[rcvd_i] = data[i];
  }

  if (rcvd_i == rcvd_len) {
    //send rcvd buffer with USART
    for (uchar j = 0; j < rcvd_len+1; j++) {
      while (!(UCSR0A & (1 << UDRE0)));
      UDR0 = rcvd[j];
    }
    has_usb_message = 1;
    return 1; // 1 if we received it all, 0 if not
  }

  return 0;
}

void uartInit() {
  UBRR0H = (uint8_t) (103 >> 8);
  UBRR0L = (uint8_t) (103 & 0xFF);
  UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
  UCSR0C = (0 << USBS0) | (3 << UCSZ00);
}

int main() {
  uartInit();

  usbInit();
  usbDeviceConnect();
  
  sei();
  
  while(1) {
    usbPoll();
  }

  return 0;
}

