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
      int mcastPort, bool loopbackOn, bool useIpV6, float loopInterval);
  virtual ~SenderModule() {}
  bool run();

  /**
   * Listen for ACK messages from receiver modules
   */
  void* runUcastReceiver();

private:
  /**
   * Associate ifaceName to multicast address
   * If ifacename is empty, bind with kernel determined interface
   *
   * @param fd        - udp socket that needs binding
   * @param ifaceName - name of interface
   */
  bool setMcastWithIfaceName(int fd, const char* ifaceName);
  bool setMcastWithV6IfaceName(int fd, const char* ifaceName);

  // true if should send message in loop
  bool shouldLoop() const;

private:
  bool mIsLoopBackOn;
  float mLoopInterval; // loop micro seconds, -1 if send once
  string mMcastSingleAddress;

// multi thread area -----------------------------
private:
  bool mIsStopped;
public:
  static void* rxThreadHelper(void* context);
// -----------------------------------------------
};

#endif /* MCAST_TOOL_MCASTSENDERMODULE_H_ */
