

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

int MeterStat (int sock, int meter)
{
	unsigned char buff[32];
	int i, seconds;
	getMeterStatsCmd_t *cmd = (getMeterStatsCmd_t *) buff;
	getMeterStatsRply_t *rply = (getMeterStatsRply_t *) buff;
	genericCmd_t *ecmd = (genericCmd_t *) buff;
	getEpochsRply_t *erply = (getEpochsRply_t *) buff;
	
	ecmd->cmd = CMDGETEPOCHS;
	ecmd->transid = meter+128;
	command (sock, buff);
	seconds = erply->epochs;
	
	cmd->cmd = CMDGETMETERSTATS;
	cmd->transid = meter+64;
	cmd->meter = meter;
	
	command(sock, buff);

	if ((rply->reply != 0) || (rply->transid != (meter + 64))){
		printf("Meter %02d: error\n");
		return;
	}
	
//	for (i=0;i<32; i++) printf ("%02d %02x\n", i, buff[i]);
	
	printf("Meter %02d: %d mA %d A-s at %d secs\n", meter, rply->instCur,
		   (uint32_t) (rply->accumCur/1000), seconds);
	
}

int main(int argc, char *argv[])
{
	int sock;
	int i;
	int meter;
	boolean all;
	
	if (argc == 1){
		all = true;
	}
	else {
		if (argc != 2){
			printf("usage (%d): %s [meter]\n", argc, argv[0]);
			return;
		}

		if (!atoi(argv[1])){
			printf("invalid meter number\n");
			return;
		}

		meter = atoi(argv[1]);
		if (meter >= NUMMETERS){
			printf("invalid meter number\n");
			return;
		}
		all = false;
	}
	sock = openrawtty("/dev/ttyACM0");

	if (sock == -1) return -1;

	if (all) {
		for (i=0;i<NUMMETERS;i++) MeterStat(sock, i);
	}
	else MeterStat (sock, meter);
			
	
	//	for (i=0;i<12;i++) printf ("0x%x\n", buff[i]);
	close (sock);   
}


