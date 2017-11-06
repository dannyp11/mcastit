#ifndef MCAST_TOOL_MCASTSENDERMODULE_H_
#define MCAST_TOOL_MCASTSENDERMODULE_H_

#include "McastModuleInterface.h"

class SenderModule: public McastModuleInterface
{
public:
  SenderModule(const vector<IfaceData>& ifaces,
      const string& mcastAddress, int mcastPort, bool loopbackOn);
  virtual ~SenderModule();
  bool run();

private:
  void setMcastIface(int fd, const char* ifaceName);
  void setMcastV6Iface(int fd, const char* ifaceName);

private:
  bool mIsLoopBackOn;
};

#endif /* MCAST_TOOL_MCASTSENDERMODULE_H_ */
