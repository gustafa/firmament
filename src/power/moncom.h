
#define USBPACKLEN 32


int openMonitor(const char device[64]);
int monCommand (int sock, unsigned char *reqBuff, unsigned char *replyBuff);


