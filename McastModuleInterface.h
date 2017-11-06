#ifndef MCAST_TOOL_MCASTMODULEINTERFACE_H_
#define MCAST_TOOL_MCASTMODULEINTERFACE_H_

#include "Common.h"

class McastModuleInterface
{
public:
  McastModuleInterface(const vector<IfaceData>& ifaces, const string& mcastAddress, int mcastPort);
  virtual ~McastModuleInterface() {}

  virtual void run() = 0;

protected:
  vector<IfaceData> mIfaces;
  string mMcastAddress;
  int mMcastPort;
};

inline McastModuleInterface::McastModuleInterface(const vector<IfaceData>& ifaces,
    const string& mcastAddress, int mcastPort) :
    mIfaces(ifaces), mMcastAddress(mcastAddress), mMcastPort(mcastPort)
{
  cout << "MCAST " << mcastAddress << ":" << mcastPort << endl;
}
#endif /* MCAST_TOOL_MCASTMODULEINTERFACE_H_ */
