#ifndef MCAST_TOOL_MCASTSENDERMODULE_H_
#define MCAST_TOOL_MCASTSENDERMODULE_H_

#include "McastModuleInterface.h"

/**
 * Send multicast
 */
class SenderModule: public McastModuleInterface
{
public:
  SenderModule(const vector<IfaceData>& ifaces,
      const string& mcastAddress, int mcastPort, bool loopbackOn, bool useIpV6);
  virtual ~SenderModule() {}
  bool run();

private:
  /**
   * Associate ifaceName to multicast address
   * If ifacename is empty, bind with kernel determined interface
   *
   * @param fd        - udp socket that needs binding
   * @param ifaceName - name of interface
   */
  bool setMcastIface(int fd, const char* ifaceName);
  bool setMcastV6Iface(int fd, const char* ifaceName);

private:
  bool mIsLoopBackOn;
};

#endif /* MCAST_TOOL_MCASTSENDERMODULE_H_ */
