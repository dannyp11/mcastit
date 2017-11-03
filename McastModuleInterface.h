#ifndef MCAST_TOOL_MCASTMODULEINTERFACE_H_
#define MCAST_TOOL_MCASTMODULEINTERFACE_H_

#include "Common.h"

class McastModuleInterface
{
public:
  McastModuleInterface(const vector<IfaceData>& ifaces, const string& mcastAddress, int mcastPort);
  virtual ~McastModuleInterface();

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

inline McastModuleInterface::~McastModuleInterface()
{
  for (unsigned i = 0; i < mIfaces.size(); ++i)
  {
    if (mIfaces[i].sockFd >= 0)
    {
      close(mIfaces[i].sockFd);
    }
  }
}

#endif /* MCAST_TOOL_MCASTMODULEINTERFACE_H_ */
