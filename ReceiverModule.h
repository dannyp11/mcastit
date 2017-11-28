#ifndef MCAST_TOOL_MCASTRECEIVERMODULE_H_
#define MCAST_TOOL_MCASTRECEIVERMODULE_H_

#include "McastModuleInterface.h"

/**
 * Listener for multicast messages
 */
class ReceiverModule: public McastModuleInterface
{
public:
   ReceiverModule(const vector<IfaceData>& ifaces,
       const vector<string>& mcastAddresses, int mcastPort, bool useIpV6);
   virtual ~ReceiverModule();
   bool run();

private:
   int mUnicastSenderSock;
};

#endif /* MCAST_TOOL_MCASTRECEIVERMODULE_H_ */
