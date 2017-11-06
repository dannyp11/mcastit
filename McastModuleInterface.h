#ifndef MCAST_TOOL_MCASTMODULEINTERFACE_H_
#define MCAST_TOOL_MCASTMODULEINTERFACE_H_

#include "Common.h"

class McastModuleInterface
{
public:
  McastModuleInterface(const vector<IfaceData>& ifaces, const string& mcastAddress,
                       int mcastPort, bool useIpV6 = false);
  virtual ~McastModuleInterface() {}

  virtual bool run() = 0;

protected:
  bool isIpV6() const;

protected:
  vector<IfaceData> mIfaces;
  string mMcastAddress;
  int mMcastPort;
  int mSin_Family;
};

inline McastModuleInterface::McastModuleInterface(const vector<IfaceData>& ifaces,
    const string& mcastAddress, int mcastPort, bool useIpV6) :
    mIfaces(ifaces), mMcastAddress(mcastAddress), mMcastPort(mcastPort)
{
  string ipVer = (useIpV6)? "IPV6" : "IPV4";
  cout << "MCAST with " << ipVer << " " << mcastAddress << ":" << mcastPort << endl;
  mSin_Family = (useIpV6)? AF_INET6 : AF_INET;
}

inline bool McastModuleInterface::isIpV6() const
{
  return (mSin_Family == AF_INET6);
}
#endif /* MCAST_TOOL_MCASTMODULEINTERFACE_H_ */
