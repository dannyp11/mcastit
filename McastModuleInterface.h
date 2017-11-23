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
  vector<IfaceData> mIfaces; // all interfaces to be listened/sent to
  string mMcastAddress;
  int mMcastPort, mAckPort;

private:
  bool mIsIpV6;
};

inline McastModuleInterface::McastModuleInterface(const vector<IfaceData>& ifaces,
    const string& mcastAddress, int mcastPort, bool useIpV6) :
    mIfaces(ifaces), mMcastAddress(mcastAddress), mMcastPort(mcastPort), mIsIpV6(useIpV6)
{
  string ipVer = (useIpV6)? "IPV6" : "IPV4";
  cout << "MCAST with " << ipVer << " " << mcastAddress << " port " << mcastPort << endl;
  mAckPort = mMcastPort + 1;
}

inline bool McastModuleInterface::isIpV6() const
{
  return mIsIpV6;
}
#endif /* MCAST_TOOL_MCASTMODULEINTERFACE_H_ */
