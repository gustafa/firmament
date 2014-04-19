

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>

#include "command.h"

#define RESPACKLEN 32

#define NUMMETERS 32




static int openrawtty(char device[64])
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
  

    tios.c_cc[VMIN] = RESPACKLEN;
    tios.c_cc[VTIME] = 4;

    tcflush(sock, TCIFLUSH);
    if (tcsetattr(sock, TCSANOW, &tios) < 0) {
        return -1;
    }


    return sock;
}


int command (int sock, unsigned char *buff)
{
  int got, i;

  tcflush(sock, TCIFLUSH);


  got = write(sock, buff, RESPACKLEN);  
  if (got != RESPACKLEN)printf("wrote %d\n", got);
  got = read (sock, buff, RESPACKLEN);
  if (got == -1)
    {
      fprintf(stderr, "ERROR Can't read from device (%s)\n",
	      strerror(errno));
      return -1;
    }
  if (got !=RESPACKLEN)
    {
      printf ("got %d\n", got);
      if (got > 0) for (i=0;i<got;i++) printf("%d: 0x%x \'%c\'\n",
					      i, buff[i], buff[i]);
      printf ("\n");
      return -1;
    }
  return 0;
}

int SetUp (int sock)
{
	unsigned char buff[32];
	int i, seconds;
	setComTrackCmd_t *tcmd = (setComTrackCmd_t *) buff;
	setTransformerCmd_t *scmd = (setTransformerCmd_t *) buff;
	
	
	tcmd->cmd = CMDSETCOMMONTRACKINGMODE;
	tcmd->transid = 128;
	tcmd->mode = TRACKMODEINFER;
	command (sock, buff);
	
	for(i=0;i<NUMMETERS;i++){
		scmd->cmd = CMDSETTRANSFORMER;
		scmd->transformer = 25;
		scmd->transid = i+64;
		scmd->meter = i;
		command (sock, buff);
	}
	
	
}

int main(int argc, char *argv[])
{
	int sock;
	int i;
	sock = openrawtty("/dev/ttyACM0");

	if (sock == -1) return -1;

	SetUp (sock);
			
	
	//	for (i=0;i<12;i++) printf ("0x%x\n", buff[i]);
	close (sock);   
}


