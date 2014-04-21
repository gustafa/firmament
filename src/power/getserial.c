

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

int SerNum (int sock)
{
	unsigned char buff[32];
	int i, seconds, sernum;
	genericCmd_t *ecmd = (genericCmd_t *) buff;
	getEpochsRply_t *erply = (getEpochsRply_t *) buff;
	getSerNumRply_t *srply = (getSerNumRply_t *) buff;
	
	
	ecmd->cmd = CMDGETEPOCHS;
	ecmd->transid = 128;
	command (sock, buff);
	seconds = erply->epochs;
	
	ecmd->cmd = CMDGETSERNUM;
	ecmd->transid = 64;
	command(sock, buff);
		
	printf("sernum %c%c%c%c running %d secs\n",
		   srply->serNum[0], 
		   srply->serNum[1], 
		   srply->serNum[2], 
		   srply->serNum[3], 
		   seconds);
	
}

int main(int argc, char *argv[])
{
	int sock;
	int i;
	int meter;
	boolean all;
	
	sock = openrawtty("/dev/ttyACM0");

	if (sock == -1) return -1;

	SerNum (sock);
			
	
	//	for (i=0;i<12;i++) printf ("0x%x\n", buff[i]);
	close (sock);   
}


