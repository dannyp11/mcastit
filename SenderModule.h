#ifndef MCAST_TOOL_MCASTSENDERMODULE_H_
#define MCAST_TOOL_MCASTSENDERMODULE_H_

#include "McastModuleInterface.h"

/**
 * Send multicast
 */
class SenderModule: public McastModuleInterface
{
public:
  SenderModule(const vector<IfaceData>& ifaces, const string& mcastAddress,
      int mcastPort, int nLoopbackIfaces, bool useIpV6, float loopInterval);
  virtual ~SenderModule() {}
  bool run();

  /**
   * Listen for ACK messages from receiver modules
   */
  void* runUcastReceiver();

protected:
  /**
   * Init all interfaces
   * @return true on success
   */
  bool init();

  /**
   * Main sender function
   * @param port - the port to send to, set -1 if want to use mMcastPort
   * @return true on success
   */
  bool sendMcastMessages(int port = -1);

private:
  // true if should send message in loop
  bool shouldLoop() const;

private:
  int mLoopbackCount;
  float mLoopInterval; // loop micro seconds, -1 if send once
  string mMcastSingleAddress;
  int mSenderPort;

// multi thread area -----------------------------
private:
  bool mIsStopped;
public:
  static void* rxThreadHelper(void* context);
// -----------------------------------------------
};

#endif /* MCAST_TOOL_MCASTSENDERMODULE_H_ */
