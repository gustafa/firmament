

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

int SetSerNum (int sock, int sernum)
{
	unsigned char buff[32];
	int i, seconds;
	genericCmd_t *ecmd = (genericCmd_t *) buff;
	getEpochsRply_t *erply = (getEpochsRply_t *) buff;
	setSerNumCmd_t *scmd = (setSerNumCmd_t *) buff;
	
	
	ecmd->cmd = CMDGETEPOCHS;
	ecmd->transid = 128;
	command (sock, buff);
	seconds = erply->epochs;
	
	scmd->cmd = CMDSETSERNUM;
	scmd->transid = 64;
	
	scmd->serNum[0] = sernum / 1000 + '0';
	scmd->serNum[1] = (sernum / 100) % 10 + '0';
	scmd->serNum[2] = (sernum / 10) % 10 + '0';
	scmd->serNum[3] = sernum % 10 + '0';
	
	command(sock, buff);

	printf ("return code %d \n", erply->reply);
//	printf("sernum %d running %d secs\n", sernum, seconds);
	
}

int main(int argc, char *argv[])
{
	int sock;
	int i;
	int sernum;
	boolean all;
	
	if (argc != 2){
		printf("usage (%d): %s sernum\n", argc, argv[0]);
		return;
	}
	
	if (!atoi(argv[1])){
		printf("invalid serial number\n");
		return;
	}
	
	sernum = atoi(argv[1]);
	if (sernum >= 9999){
		printf("invalid serial number\n");
		return;
	}
	
	sock = openrawtty("/dev/ttyACM0");

	if (sock == -1) return -1;

	SetSerNum (sock, sernum);
			
	
	//	for (i=0;i<12;i++) printf ("0x%x\n", buff[i]);
	close (sock);   
}


