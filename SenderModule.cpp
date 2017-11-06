#include "SenderModule.h"

SenderModule::SenderModule(const vector<IfaceData>& ifaces, const string& mcastAddress,
    int mcastPort, bool loopbackOn, bool useIpV6) :
    McastModuleInterface(ifaces, mcastAddress, mcastPort, useIpV6), mIsLoopBackOn(loopbackOn)
{
  string loopMsg = (mIsLoopBackOn) ? "with" : "without";
  cout << "Sending " << loopMsg << " loopback" << endl;
}

SenderModule::~SenderModule()
{
}

bool SenderModule::run()
{
  for (unsigned i = 0; i < mIfaces.size(); ++i)
  {
    if (isIpV6())
    {
      setMcastV6Iface(mIfaces[i].sockFd, mIfaces[i].ifaceName.c_str());
    }
    else
    {
      setMcastIface(mIfaces[i].sockFd, mIfaces[i].ifaceName.c_str());
    }
  }
  cout << "==============================================================" << endl;

  // build addr struct
  struct sockaddr_in addr;
  int sinFamily = (isIpV6())? AF_INET6 : AF_INET;
  addr.sin_family = sinFamily;
  addr.sin_port = htons(mMcastPort);

  struct sockaddr_in6 addr6;
  addr6.sin6_family = sinFamily;
  addr6.sin6_port = htons(mMcastPort);

  if (isIpV6())
  {
    if (inet_pton(AF_INET6, mMcastAddress.c_str(), &addr6.sin6_addr) != 1)
    {
      cout << "Error parsing address for " << mMcastAddress << endl;
      return false;
    }
  }
  else
  {
    addr.sin_addr.s_addr = inet_addr(mMcastAddress.c_str());
  }

  const string dmsg = "<Sender info:";

  for (unsigned i = 0; i < mIfaces.size(); ++i)
  {
    const IfaceData ifaceData = mIfaces[i];
    int fd = ifaceData.sockFd;

    // build message
    std::stringstream msg;
    msg << dmsg << " " << ifaceData << ">";

    int numBytes = -1;
    if (!isIpV6())
    {
      numBytes = sendto(fd, msg.str().c_str(), 1 + msg.str().size(),
                            MSG_NOSIGNAL, (struct sockaddr *) &addr, sizeof(addr));
    }
    else
    {
      numBytes = sendto(fd, msg.str().c_str(), 1 + msg.str().size(),
                            MSG_NOSIGNAL, (struct sockaddr *) &addr6, sizeof(addr6));
    }

    if (0 > numBytes)
    {
      cout << "Error sending to " << ifaceData << " :" << strerror(errno) << endl;
      return false;
    }
    else
    {
      cout << "sent to [" << ifaceData << "] bytes: " << numBytes << endl;
    }
  }

  return true;
}

void SenderModule::setMcastIface(int fd, const char* ifaceName)
{
  // Set the TTL hop count
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

  // Enable loop back if set.
  opt = mIsLoopBackOn;
  res = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, (void*) &opt, sizeof(opt));
  if (0 > res)
  {
    perror("ERR> sockopt enable loopback.");
    return;
  }

  // If iface name specified, set iface name for the socket
  if (0 < strlen(ifaceName))
  {
    struct ip_mreqn ifaddrn;
    memset(&ifaddrn, 0, sizeof(ifaddrn));

    ifaddrn.imr_ifindex = if_nametoindex(ifaceName);
    if (0 != (res = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &ifaddrn, sizeof(ifaddrn))))
    {
      perror("setting fd");
    }
  }
}

void SenderModule::setMcastV6Iface(int fd, const char* ifaceName)
{
  // Set the TTL hop count
  int opt = 6;
  int res = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (void*) &opt, sizeof(opt));
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

  // Enable loop back if set.
  opt = mIsLoopBackOn;
  res = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (void*) &opt, sizeof(opt));
  if (0 > res)
  {
    perror("ERR> sockopt enable loopback.");
    return;
  }

  // If iface name specified, set iface name for the socket
  if (0 < strlen(ifaceName))
  {
    unsigned ifIndex = if_nametoindex(ifaceName);
    if (0 != (res = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifIndex, sizeof(ifIndex))))
    {
      perror("setting fd");
    }
  }
}
