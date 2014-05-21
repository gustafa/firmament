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
#include <iostream>

#include <assert.h>

#define DEFAULTPOLLINTERVAL 3 //30

extern "C" {
  #include "command.h"
  #include "metering32.h"
  #include "moncom.h"
}

using firmament::misc::Envelope;

int scalingFactor = 3;

// BinarySearch returns the position if a match is found and the position the element
// would be inserted at otherwise.
int binarySearch(const std::vector<double> *a, double key) {
  unsigned int low = 0;
  unsigned int high = a->size();
  unsigned int mid;
  unsigned int midVal;

  while (low <= high) {
    mid = (low + high) >> 1;
    midVal = (*a)[mid];

    std::cout << "MIDVAL: " << midVal << "LOW " << low << "MID " << mid << " HIGH " << high << "\n";

    if (midVal < key)
      low = mid + 1;
    else if (midVal > key)
      high = mid - 1;
    else
      return mid; // key found
  }
  return (low);  // key not found.
}

int getLeftIndex(const std::vector<double> *a, double key) {
  int size = a->size();
  for (int i = 0; i < size; ++i) {
    if ((*a)[i] > key) {
      return i - 1;
    }
  }
  return a->size() - 1;
}


Victoria::Victoria(std::string firmament_master, std::vector<double> *xs, std::vector<double> *ys) {
  this->xs = xs;
  this->ys = ys;
  this->coordinator_uri = firmament_master;
  m_adapter_ = new StreamSocketsAdapter<BaseMessage>();
  chan_ = new StreamSocketsChannel<BaseMessage>(StreamSocketsChannel<BaseMessage>::SS_TCP);
}

void Victoria::addMonitor(int port, std::string hostname) {
  portToHost[port] = hostname;
}

bool Victoria::ConnectToCoordinator(const string& coordinator_uri) {
  return m_adapter_->EstablishChannel(coordinator_uri, chan_);
}

 void Victoria::HandleWrite(const boost::system::error_code& error,
        size_t bytes_transferred) {
  VLOG(1) << "In HandleWrite, thread is " << boost::this_thread::get_id();
  if (error)
    LOG(ERROR) << "Error returned from async write: " << error.message();
  else
    VLOG(1) << "bytes_transferred: " << bytes_transferred;
}

double Victoria::toRealWatts(double measuredWatt) {
  //unsigned long insertIndex = binarySearch(xs, measuredWatt);

  int left_dex = getLeftIndex(xs, measuredWatt);

  std::cout << "LEFT INDEX " << left_dex << "\n";
  // If it isn't the first or last element we can just pick the elements
  // on the side. Should be practically always.
  int size = xs->size();
  if (left_dex != -1 && left_dex != size - 1) {
    int left_idx = left_dex; //insertIndex - 1;
    int right_idx = left_dex + 1;

    printf("VAL: %f, LEFT % f, RIGHT: %f\n", measuredWatt, (*xs)[left_idx], (*xs)[right_idx]);
    assert(measuredWatt >= (*xs)[left_idx]);
    assert(measuredWatt <= (*xs)[right_idx]);
    // double left_x = (*xs)[insertIndex -1];
    // double right_x = (*xs)[insertIndex +1];

    double from_left = measuredWatt - (*xs)[left_idx];
    double range = (*xs)[right_idx] - (*xs)[left_idx];

    double frac_right = from_left / range;
    double frac_left = 1 - frac_right;

    return frac_left * (*ys)[left_idx] + frac_right * (*ys)[right_idx];

  } else {
     printf("WARNING VALUE OUT OF SAFE RANGE\n");
    // TODO really should warn.
     if (left_dex == -1) {
       return 0;
     }
     return (measuredWatt / (*xs)[left_dex]) * (*ys)[left_dex];
  }
}


void Victoria::run() {
  int socket;
  int pollInt; // nominalPollTime;
  double energy;
  time_t t, nominalPollTime; //, t;
  double toWatts = 240 / (double) 3000;
  double prev[32] = {0};
  double totals[32] = {0};


  while (true) {
    if (ConnectToCoordinator(coordinator_uri)) {
      printf("Successfully connected to coordinator: %s\n", coordinator_uri.c_str());
      break;
    }
      printf("Failed to connect to coordinator; is it reachable? Retrying");
      sleep(2);
  }

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

  bool first = true;

  while (true) {
    // TODO verify this.
    //    nominalPollTime = time(NULL);//((time(NULL) - 1) / pollInt + 1) * pollInt;
    nominalPollTime = ((time(NULL) - 1) / pollInt + 1) * pollInt;

    std::cout << "nominalPollTime " << nominalPollTime << "\n";

    sleep(2);

    while (true) {
      t = time (NULL);
      firmament::BaseMessage bm;
      firmament::EnergyStatsMessage *energyStats = bm.mutable_energy_stats();

      energyStats->set_update_interval(scalingFactor);

      for (iter = portToHost.begin(); iter != portToHost.end(); ++iter) {
 //                                // Get samples and write log file
        int port = iter->first;
        std::string hostname = iter->second;
        firmament::EnergyStatsMessage_EnergyMessage* energy_message =
            energyStats->add_energy_messages();
        energy_message->set_hostname(hostname);




        // printf("Port-%d: ", port);
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
        double delta = (energy - prev[port]) * toWatts;
        prev[port] = energy;
        // printf("%.2f\t",delta * toWatts); // - prev[i]);
        if (!first) {
          double real_delta = toRealWatts(delta);
          totals[port] = totals[port] + real_delta;
	  energy_message->set_deltaj(real_delta);
          energy_message->set_totalj(totals[port]);
        } else {
          std::cout << "SKIPPING\n";
        }

      }
      if (!first) {

        sendStats(&bm);
      } else {
        first = false;
      }

    nominalPollTime += pollInt;
    //long val = nominalPollTime - time(NULL);

    //std::cout << "VAL: " << val << "\n";
    //std::cout << "Nompolltime: " << nominalPollTime << "\n";


    // sleep to make up to next interval
    sleep(nominalPollTime - time(NULL));
    }
  }
}

bool Victoria::sendStats(firmament::BaseMessage *bm) {
  Envelope<BaseMessage> envelope(bm);
  return chan_->SendA(
          envelope, boost::bind(&Victoria::HandleWrite,
          this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));

}



int main(int argc, char const *argv[]) {



  // Measured power values.
  std::vector<double> xs = {1.6, 5.2, 10.7, 22.2, 24.9, 25.9, 30.7, 31.5, 33.8, 41.5, 54.6, 84.0, 146.3, 203.9, 251.7, 285.6, 311.9, 352.0 };

  // Real power values.
  std::vector<double> ys = {0, 2.5, 6.3, 13.3, 14.5, 15.4, 24.8, 47.8, 50.1, 62.3, 71, 117, 210, 302, 397, 483, 574, 787};

  Victoria victoria(argv[1], &xs, &ys);



  victoria.addMonitor(9, "pandaboard");
  victoria.addMonitor(11, "uriel");
  victoria.addMonitor(13, "michael");
  victoria.addMonitor(14, "titanic");

  victoria.run();

  return 0;
}
