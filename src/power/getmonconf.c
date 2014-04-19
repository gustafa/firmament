#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "command.h"
#include "moncom.h"

#define NUMMETERS 32

// Generic GetCommand to handle errors

int GetCommand (int socket, char *reqBuff, char *replyBuff){

	genericRply_t *rply = (genericRply_t *) replyBuff;
	genericCmd_t *cmd = (genericCmd_t *) reqBuff;
	
	if (monCommand (socket, reqBuff, replyBuff) == 0){
		if ((rply->reply != 0)|| (rply->transid != cmd->transid)){
			printf ("Bad result from setting, cmd = %d, rc = %d\n", cmd->cmd, rply->reply);
			return -1;
		}
	}
	else {
		printf ("Communication failure on setting\n");
		return -1;
	}
	return 0;
}
	
int GetTracking (int socket, int *tracking){
	char reqBuff[USBPACKLEN], replyBuff[USBPACKLEN];
	genericCmd_t *tCmd = (genericCmd_t *) reqBuff;
	getComTrackRply_t *tRply = (getComTrackRply_t *) replyBuff;
	
	tCmd->cmd = CMDGETCOMMONTRACKINGMODE;
	tCmd->transid = rand() & 0xff;

	if (GetCommand (socket, reqBuff, replyBuff) == 0) *tracking = tRply->mode;
}
	
int GetPhase (int socket, int meter, int *phase){
	char reqBuff[USBPACKLEN], replyBuff[USBPACKLEN];
	getPhaseRply_t *pRply = (getPhaseRply_t *) replyBuff;
	getPhaseCmd_t *pCmd = (getPhaseCmd_t *) reqBuff;
	pCmd->cmd = CMDGETPHASE;
	pCmd->transid = rand() & 0xff;
	pCmd->meter = meter;
	
	if (GetCommand (socket, reqBuff, replyBuff) == 0) *phase = pRply->phase;
}

int GetTransformer (int socket, int meter, int *transformer){
	char reqBuff[USBPACKLEN], replyBuff[USBPACKLEN];
	getTransformerRply_t *tRply = (getTransformerRply_t *) replyBuff;
	getTransformerCmd_t *tCmd = (getTransformerCmd_t *) reqBuff;

	
	tCmd->cmd = CMDGETTRANSFORMER;
	tCmd->transid = rand() & 0xff;
	tCmd->meter = meter;
	if (GetCommand (socket, reqBuff, replyBuff) == 0) *transformer = tRply->transformer;
}

int main (int argc, char * const argv[]) {
	
	int i, socket;
	int tracking;
	int transformer[NUMMETERS];
	int phase[NUMMETERS];
	
	socket = openMonitor ("/dev/ttyACM0");
	
	if (socket == -1) exit(-1);
	
   
	GetTracking (socket, &tracking);
	switch (tracking) {
		case TRACKMODEINFER: printf("Infering common\n"); break;
		case TRACKMODELOWCHANADC: printf("Measuring common in ADCs\n"); break;
		case TRACKMODEPROC: printf("Measuring common on processor\n"); break;
		default: printf("Bad tracking mode\n"); break;
	}
	
	for (i=0;i<NUMMETERS;i++){
		GetPhase (socket, i, &phase[i]);
		GetTransformer (socket, i, &transformer[i]);
		printf ("%4d, %4d, %5d\n", i, phase[i], transformer[i]);
	}

}
