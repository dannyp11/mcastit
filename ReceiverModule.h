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
         const string& mcastAddress, int mcastPort, bool useIpV6);
   virtual ~ReceiverModule() {}
   bool run();

private:
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

   /**
    * Respond to the sender using udp unicast
    * @param sender   - sender data
    * @param ackMsg   - ack message
    * @return true on sent success
    */
   bool sendAck(struct sockaddr_storage& sender, const string& ackMsg) const;
};

#endif /* MCAST_TOOL_MCASTRECEIVERMODULE_H_ */
