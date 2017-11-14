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
  bool setMcastWithIfaceName(int fd, const char* ifaceName);
  bool setMcastWithV6IfaceName(int fd, const char* ifaceName);

  /**
   * Similar to setMcastWithIfaceName but using interface IP address instead
   *
   * @param fd            - udp socket that needs binding
   * @param ifaceAddress  - IP address of interface
   */
  bool setMcastWithIfaceAddress(int fd, const char* ifaceAddress);

private:
  bool mIsLoopBackOn;
};

#endif /* MCAST_TOOL_MCASTSENDERMODULE_H_ */
