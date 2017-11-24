#ifndef MCAST_TOOL_MCASTMODULEINTERFACE_H_
#define MCAST_TOOL_MCASTMODULEINTERFACE_H_

#include "Common.h"

/**
 * The interface that handles both reader/writer child classes
 */
class McastModuleInterface
{
public:
  McastModuleInterface(const vector<IfaceData>& ifaces, const string& mcastAddress,
                       int mcastPort, bool useIpV6 = false);
  virtual ~McastModuleInterface() {}

  /**
   * Main runner function, should not be inside a loop
   * @return true if run successfully
   */
  virtual bool run() = 0;

  bool isIpV6() const;

protected:
  /**
   * Associate ifaceName to multicast address
   * If ifacename is empty, bind with kernel determined interface
   *
   * @param fd        - udp socket that needs binding
   * @param ifaceName - name of interface
   * @param isLoopBack - turn on loopback sending
   */
  bool associateMcastWithIfaceName(int fd, const char* ifaceName, bool isLoopBack = false);
  bool associateMcastV6WithIfaceName(int fd, const char* ifaceName, bool isLoopBack = false);

  /**
   * Join multicast on socket with interface ifaceName
   * If ifaceName is empty, join to generic interface determined by kernel
   *
   * @param sock       - sock fd to join
   * @param ifaceName  - interface name
   * @return 0 on success
   */
  int joinMcastIface(int sock, const char* ifaceName = "");
  int joinMcastIfaceV6(int sock, const char* ifaceName = "");

protected:
  vector<IfaceData> mIfaces; // all interfaces to be listened/sent to
  string mMcastAddress;
  int mMcastPort, mAckPort;

private:
  bool mIsIpV6;
};
#endif /* MCAST_TOOL_MCASTMODULEINTERFACE_H_ */
