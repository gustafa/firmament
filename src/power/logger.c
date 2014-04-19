#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>


double prev[32] = {0}; 

#include "command.h"
#include "moncom.h"
#include "metering32.h"

#define DEFAULTPOLLINTERVAL 3 //30
int scalingFactor = 3; //Extracted from niagara.json

int main (int argc, char **const argv) {
	
	int i, c;
	int socket;
	int pollInt;
	char fname[120];
	char timeStr[30];
	double energy;
	int first;
	struct tm *gmt;
	time_t nominalPollTime, t;
	int start = 8;
	int end = 16;


	while ((c = getopt(argc, argv, "p:")) != -1) {
		switch (c) {
			// Read only the specified port.
			case 'p':
				start = strtol(optarg, 0, 10);
				end = start + 1;
				break;
		}
	}
	

	
	char reqBuff[USBPACKLEN], rplyBuff[USBPACKLEN];
	getMeterStatsCmd_t *sCmd = (getMeterStatsCmd_t *) reqBuff;
	getMeterStatsRply_t *sRply = (getMeterStatsRply_t *) rplyBuff;
		
	socket = openMonitor ("/dev/ttyACM0");
	
	if (socket == -1) exit(-1);

	pollInt = DEFAULTPOLLINTERVAL;

	
	while(1){

		nominalPollTime = ((time(NULL) - 1) / pollInt + 1) * pollInt;
		sleep(2);

		// V, 
		double toWatts = 240 / 3000;

		printf("Running logger...\n");	   
		while (1) {
			t = time (NULL);
			gmt = gmtime(&t);
			strftime (timeStr, 30, "%Y-%m-%dT%H:%M:%SZ", gmt);

			printf("%s\t", timeStr);
			for (i=start; i<end; i++){
				// Get samples and write log file
				
				sCmd->cmd = CMDGETMETERSTATS;
				sCmd->transid = rand() & 0xff;
				sCmd->meter = i;
				
				if (monCommand (socket, reqBuff, rplyBuff) != 0){
					printf("Polling failure for meter %d\n", i);
					exit (-1);
				}

				// We are reporting accumulated mA-secs - convert to kWh assuming in-phase 240V
				// and divide by scaling factor 
				

				energy = ((float) (sRply->accumCur) / scalingFactor); //  * 240.0; // / 1000.0 / 1000.0 / 3600.0;
				double delta = energy - prev[i];
				printf("%.2f\t",delta * toWatts); // - prev[i]);
				prev[i] = energy; 
				
			}
			printf("\n");
			fflush(stdout);

			nominalPollTime += pollInt;

			// sleep to make up to next interval
			sleep (nominalPollTime - time(NULL));
			
		}
	}

}
