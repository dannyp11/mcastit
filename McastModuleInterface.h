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
  int mMcastPort;

private:
  int mSin_Family;
};

inline McastModuleInterface::McastModuleInterface(const vector<IfaceData>& ifaces,
    const string& mcastAddress, int mcastPort, bool useIpV6) :
    mIfaces(ifaces), mMcastAddress(mcastAddress), mMcastPort(mcastPort)
{
  string ipVer = (useIpV6)? "IPV6" : "IPV4";
  cout << "MCAST with " << ipVer << " " << mcastAddress << " port " << mcastPort << endl;
  mSin_Family = (useIpV6)? AF_INET6 : AF_INET;
}

inline bool McastModuleInterface::isIpV6() const
{
  return (mSin_Family == AF_INET6);
}
#endif /* MCAST_TOOL_MCASTMODULEINTERFACE_H_ */
