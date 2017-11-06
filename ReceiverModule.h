#ifndef MCAST_TOOL_MCASTRECEIVERMODULE_H_
#define MCAST_TOOL_MCASTRECEIVERMODULE_H_

#include "McastModuleInterface.h"

class ReceiverModule: public McastModuleInterface
{
public:
   ReceiverModule(const vector<IfaceData>& ifaces,
         const string& mcastAddress, int mcastPort);
   virtual ~ReceiverModule();
   bool run();

private:
   int joinMcastIface(int sock, const char* ifaceName = "");
};

#endif /* MCAST_TOOL_MCASTRECEIVERMODULE_H_ */
