#include "SenderModule.h"

SenderModule::SenderModule(const vector<IfaceData>& ifaces, const string& mcastAddress,
    int mcastPort, bool loopbackOn) :
    McastModuleInterface(ifaces, mcastAddress, mcastPort), mIsLoopBackOn(loopbackOn)
{
  string loopMsg = (mIsLoopBackOn) ? "with" : "without";
  cout << "Sending " << loopMsg << " loopback" << endl;
}

SenderModule::~SenderModule()
{
}

void SenderModule::run()
{
  for (unsigned i = 0; i < mIfaces.size(); ++i)
  {
    setMcastIface(mIfaces[i].sockFd, mIfaces[i].ifaceName.c_str());
  }
  cout << "==============================================================" << endl;

  // build addr struct
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(mMcastAddress.c_str());
  addr.sin_port = htons(mMcastPort);

  const string dmsg = "<Sender info:";

  for (unsigned i = 0; i < mIfaces.size(); ++i)
  {
    const IfaceData ifaceData = mIfaces[i];
    int fd = ifaceData.sockFd;

    // build message
    std::stringstream msg;
    msg << dmsg << " " << ifaceData << "]>";

    int numBytes = sendto(fd, msg.str().c_str(), 1 + msg.str().size(),
                          MSG_NOSIGNAL, (struct sockaddr *) &addr, sizeof(addr));
    cout << "sent to [" << ifaceData << "] bytes: " << numBytes << endl;
    if (0 > numBytes)
    {
      perror("recv");
    }
  }
}

void SenderModule::setMcastIface(int fd, const char* ifaceName)
{
  struct ip_mreqn ifaddrn;
  memset(&ifaddrn, 0, sizeof(ifaddrn));

  // Set the TTL hop count so that the mcast cast doesn't go all over the place.
  int opt = 6;
  int res = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, (void*) &opt, sizeof(opt));
  if (0 > res)
  {
    perror("ERR> sockopt TTL.");
    return;
  }

  // Allow broadcast on this socket.
  opt = 1;
  res = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (void*) &opt, sizeof(opt));
  if (0 > res)
  {
    perror("ERR> sockopt allow broadcast.");
    return;
  }

  // Enable loop back as we may be sending to a client on our own host.
  opt = mIsLoopBackOn;
  res = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, (void*) &opt, sizeof(opt));
  if (0 > res)
  {
    perror("ERR> sockopt enable loopback.");
    return;
  }

  // if iface name specified, set iface name for the socket
  if (0 < strlen(ifaceName))
  {
    ifaddrn.imr_ifindex = if_nametoindex(ifaceName);
    res = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &ifaddrn, sizeof(ifaddrn));
    if (0 != res)
    {
      perror("setting fd");
    }
  }
}
