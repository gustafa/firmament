

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>


#include "moncom.h"

// Routine to open a socket to the device whose name is specified in device
// Simply open the device in raw mode and flush.

int openMonitor(const char device[64])
{

    struct termios tios;
    int sock;

    sock = open(device, O_RDWR | O_NOCTTY);
    if (sock == -1) {
        fprintf(stderr, "ERROR Can't open the device %s (%s)\n",
                device, strerror(errno));
        return -1;
    }

    tcgetattr(sock, &tios);
    cfmakeraw(&tios);
  
    tios.c_cc[VMIN] = USBPACKLEN;
    tios.c_cc[VTIME] = 4;

    tcflush(sock, TCIFLUSH);
    if (tcsetattr(sock, TCSANOW, &tios) < 0) {
		fprintf(stderr, "ERROR Can't set attribute on socket\n");
        return -1;
    }

    return sock;
}

// Exchange a request and reply with the device

int monCommand (int sock, unsigned char *reqBuff, unsigned char *replyBuff)
{
  int got, i;

  tcflush(sock, TCIFLUSH);


  got = write(sock, reqBuff, USBPACKLEN);  
  if (got != USBPACKLEN)fprintf(stderr, "wrote %d\n", got);
  got = read (sock, replyBuff, USBPACKLEN);
  if (got == -1)
    {
      fprintf(stderr, "ERROR Can't read from device (%s)\n",
	      strerror(errno));
      return -1;
    }
  if (got !=USBPACKLEN)
    {
      fprintf (stderr, "got %d\n", got);
		if (got > 0){ 
			for (i=0;i<got;i++) fprintf(stderr, "%d: 0x%x \'%c\'\n",i, replyBuff[i], replyBuff[i]);
			fprintf (stderr, "\n");
		}
      return -1;
    }
  return 0;
}

