#ifndef MCASTIT_SERVERMODULE_H_
#define MCASTIT_SERVERMODULE_H_

#include "SenderModule.h"

class ServerModule: public SenderModule
{
public:
  ServerModule(const vector<IfaceData>& ifaces, const string& mcastAddress,
      int mcastPort, int nLoopbackIfaces, bool useIpV6, float loopInterval);
  virtual ~ServerModule();
  bool run();

private:
  int mMcastListenSock, mUnicastSenderSock;
  int mMcastSendPort;

// Multithread area --------------------------
public:
  static void* txThreadHelper(void* context);
// -------------------------------------------
};

#endif /* MCASTIT_SERVERMODULE_H_ */
