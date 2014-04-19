// Victoria, power extractor working with firmament.

#include "victoria.h"
#include <math.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>




#define DEFAULTPOLLINTERVAL 3 //30

extern "C" {
  #include "command.h"
  #include "metering32.h"
  #include "moncom.h"
}

int scalingFactor = 3;

// Java implementation copy, returns index of where the key would
// have been inserted.
int binarySearch(const std::vector<double> *a, double key) {
  unsigned int low = 0;
  unsigned int high = a->size() - 1;
  unsigned int mid;
  unsigned int midVal;

  while (low <= high) {
    mid = (low + high) >> 1;
    midVal = (*a)[mid];

    if (midVal < key)
      low = mid + 1;
    else if (midVal > key)
      high = mid - 1;
    else
      return mid; // key found
  }
  return (low + 1);  // key not found.
  }


Victoria::Victoria(std::string firmament_master, std::vector<double> *xs, std::vector<double> *ys) {
  this->xs = xs;
  this->ys = ys;
  m_adapter_ = new StreamSocketsAdapter<BaseMessage>();
  chan_ = new StreamSocketsChannel<BaseMessage>(StreamSocketsChannel<BaseMessage>::SS_TCP);
}

void Victoria::addMonitor(int port, std::string hostname) {
  portToHost[port] = hostname;
}


 

double Victoria::toRealWatts(double measuredWatt) {
  unsigned long insertIndex = binarySearch(xs, measuredWatt);
  // If it isn't the first or last element we can just pick the elements
  // on the side. Should be practically always.
  if (insertIndex != 0 && insertIndex != xs->size() -1) {
    int left_idx = insertIndex -1;
    int right_idx = insertIndex +1;
    // double left_x = (*xs)[insertIndex -1];
    // double right_x = (*xs)[insertIndex +1];

    double from_left = measuredWatt - (*xs)[left_idx];
    double range = (*xs)[right_idx] - (*xs)[left_idx];

    double frac_right = from_left / range;
    double frac_left = 1 - frac_right;

    return frac_left * (*ys)[left_idx] + frac_right * (*ys)[right_idx];

  } else {
    // TODO really should warn.
      return (measuredWatt / (*ys)[insertIndex]) * (*ys)[insertIndex];
  }
}


void Victoria::run() {
  int socket;
  int pollInt;
  char timeStr[30];
  double energy;
  struct tm *gmt;
  time_t nominalPollTime, t;
  double toWatts = 240 / (double) 3000;
  double prev[32] = {0};


 unsigned char reqBuff[USBPACKLEN], rplyBuff[USBPACKLEN];
  getMeterStatsCmd_t *sCmd = (getMeterStatsCmd_t *) reqBuff;
  getMeterStatsRply_t *sRply = (getMeterStatsRply_t *) rplyBuff;

  std::string measurement_device = "/dev/ttyACM0";
  socket = openMonitor(measurement_device.c_str());

  if (socket == -1) {
    exit(-1);
  }

  pollInt = DEFAULTPOLLINTERVAL;

  std::unordered_map<int, std::string>::iterator iter;

  while (true) {
    // TODO verify this.
    nominalPollTime = ((time(NULL) - 1) / pollInt + 1) * pollInt;
    sleep(2);

    while (true) {
      firmament::BaseMessage bm;
      firmament::EnergyStatsMessage *energyStats = bm.mutable_energy_stats();



      t = time (NULL);
      gmt = gmtime(&t);
      strftime (timeStr, 30, "%Y-%m-%dT%H:%M:%SZ", gmt);

      printf("%s\t", timeStr);
      for (iter = portToHost.begin(); iter != portToHost.end(); ++iter) {
 //                                // Get samples and write log file
        int port = iter->first;
        std::string hostname = iter->second;


        firmament::EnergyStatsMessage_EnergyMessage* energy_message = energyStats->add_energy_messages();
        energy_message->set_uuid(hostname);




        printf("Port-%d: ", port);
        sCmd->cmd = CMDGETMETERSTATS;
        sCmd->transid = rand() & 0xff;
        sCmd->meter = port;

        if (monCommand (socket, reqBuff, rplyBuff) != 0){
          printf("Polling failure for meter %d\n", port);
          exit (-1);
        }

        // We are reporting accumulated mA-secs - convert to kWh assuming
        // in-phase 240V and divide by scaling factor 

        energy = ((float) (sRply->accumCur) / scalingFactor); //  * 240.0; // / 1000.0 / 1000.0 / 3600.0;
        double delta = energy - prev[port];
        printf("%.2f\t",delta * toWatts); // - prev[i]);
        prev[port] = energy;


      }


      printf("\n");
      fflush(stdout);

      nominalPollTime += pollInt;

    // sleep to make up to next interval
    sleep (nominalPollTime - time(NULL));     
    }
  }
}
int Victoria::sendStats(char *hostname, firmament::BaseMessage *bm) {

  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  printf("%s\n", hostname);

  portno = 8088; // TODO verify
  char buffer[256];


  firmament::misc::Envelope<firmament::BaseMessage> envelope(bm);



  // if (argc < 3) {
  //     fprintf(stderr,"usage %s hostname port\n", argv[0]);
  //     exit(0);
  // }
  /* Create a socket point */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
  {
      perror("ERROR opening socket");
      exit(1);
  }
  server = gethostbyname(hostname);
  if (server == NULL) {
      fprintf(stderr,"ERROR, no such host\n");
      exit(0);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, 
          (char *)&serv_addr.sin_addr.s_addr,
              server->h_length);
  serv_addr.sin_port = htons(portno);



  /* Now connect to the server */
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
  {
        perror("ERROR connecting");
        exit(1);
  } 
  /* Now ask for a message from the user, this message
  * will be read by server
  */
  printf("Please enter the message: ");
  bzero(buffer,256);
  fgets(buffer,255,stdin);
  /* Send message to the server */
  n = write(sockfd,buffer,strlen(buffer));
  if (n < 0) 
  {
        perror("ERROR writing to socket");
        exit(1);
  }
  /* Now read server response */
  bzero(buffer,256);
  n = read(sockfd,buffer,255);
  if (n < 0) 
  {
        perror("ERROR reading from socket");
        exit(1);
  }
  printf("%s\n",buffer);

  return 0;
}



int main(int argc, char const *argv[]) {



  // Measured power values.
  std::vector<double> xs = {1.6, 5.2, 10.7, 22.2, 24.9, 25.9, 30.7, 31.5, 33.8, 41.5, 54.6, 84.0, 146.3, 203.9, 251.7, 285.6, 311.9, 352.0 };

  // Real power values.
  std::vector<double> ys = {0, 2.5, 6.3, 13.3, 14.5, 15.4, 24.8, 47.8, 50.1, 62.3, 71, 117, 210, 302, 397, 483, 574, 787};

  Victoria victoria("localhost", &xs, &ys);



  victoria.addMonitor(9, "pandaboard");
  victoria.addMonitor(11, "uriel");
  victoria.addMonitor(13, "michael");
  victoria.addMonitor(14, "titanic");

  victoria.run();

  return 0;
}
