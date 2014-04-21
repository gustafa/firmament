 /*
 * command.h
 *
 *  Created on: Feb 12, 2013
 *      Author: iml1
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include "type.h"

#define COMMANDBUFFSIZE 32

#define CMDGETSERNUM 1
#define CMDSETSERNUM 2
#define CMDGETMETERSTATS 3
#define CMDRESETSTATS 4
#define CMDGETEPOCHS 5
#define CMDSETTRANSFORMER 6
#define CMDGETTRANSFORMER 7
#define CMDSETCOMMONTRACKINGMODE 8
#define CMDGETCOMMONTRACKINGMODE 9
#define CMDSETPHASE 10
#define CMDGETPHASE 11
#define CMDRESET 12

#define RCOK 0
#define RCSERNUMALREADYSET 50
#define RCBADMETER 51
#define RCBADCOMMAND 52
#define RCBADTRANSFORMER 53
#define RCBADSERNUM 54
#define RCBADPHASE 55


#define TRACKMODEINFER 0
#define TRACKMODELOWCHANADC 1
#define TRACKMODEPROC 2


typedef struct {
	uint8_t cmd;
	uint8_t transid;
} genericCmd_t;

typedef struct {
	uint8_t reply;
	uint8_t transid;
} genericRply_t;

typedef struct {
	uint8_t reply;
	uint8_t transid;
	uint8_t serNum[4];
} getSerNumRply_t;

typedef struct {
	uint8_t cmd;
	uint8_t transid;
	uint8_t serNum[4];
} setSerNumCmd_t;


typedef struct {
	uint8_t cmd;
	uint8_t transid;
	uint16_t meter;
} getMeterStatsCmd_t;

typedef struct {
	uint8_t reply;
	uint8_t transid;
	uint16_t meter;
	uint32_t instPwr;
	uint32_t instVoltage;
	uint32_t instCur;
	uint64_t accumCur;
	uint64_t accumEnergy;
} getMeterStatsRply_t;

typedef struct {
	uint8_t reply;
	uint8_t transid;
	uint16_t fill;
	uint32_t epochs;
} getEpochsRply_t;

typedef struct {
	uint8_t cmd;
	uint8_t transid;
	uint16_t meter;
	uint16_t transformer;
} setTransformerCmd_t;

typedef struct {
	uint8_t cmd;
	uint8_t transid;
	uint16_t meter;
} getTransformerCmd_t;

typedef struct {
	uint8_t reply;
	uint8_t transid;
	uint16_t meter;
	uint16_t transformer;
} getTransformerRply_t;

typedef struct {
	uint8_t cmd;
	uint8_t transid;
	uint8_t mode;
} setComTrackCmd_t;

typedef struct {
	uint8_t cmd;
	uint8_t transid;
	uint8_t mode;
} getComTrackRply_t;


typedef struct {
	uint8_t cmd;
	uint8_t transid;
	uint16_t meter;
	uint8_t phase;
} setPhaseCmd_t;


typedef struct {
	uint8_t cmd;
	uint8_t transid;
	uint16_t meter;
} getPhaseCmd_t;

typedef struct {
	uint8_t reply;
	uint8_t transid;
	uint16_t meter;
	uint16_t phase;
} getPhaseRply_t;

extern uint8_t commandBuff[COMMANDBUFFSIZE];
extern uint8_t replyBuff[COMMANDBUFFSIZE];

boolean replyAvail;

void CommandInit(void);
void ProcessCommand(uint32_t len);



#endif /* COMMAND_H_ */
