#include "SenderModule.h"

SenderModule::SenderModule(const vector<IfaceData>& ifaces, const string& mcastAddress,
    int mcastPort, bool loopbackOn, bool useIpV6, float loopInterval) :
    McastModuleInterface(ifaces, mcastAddress, mcastPort, useIpV6),
    mIsLoopBackOn(loopbackOn), mLoopInterval(loopInterval)
{
  string loopMsg = (mIsLoopBackOn) ? "with" : "without";
  cout << "Sending " << loopMsg << " loopback";
  if (shouldLoop())
  {
    cout << " with interval " << mLoopInterval << " second(s)";
  }
  cout << endl;
}

bool SenderModule::run()
{
  /*
   * First, setup mcast for all interfaces
   */
  for (unsigned i = 0; i < mIfaces.size(); ++i)
  {
    bool setMcastOk;
    if (isIpV6())
    {
      setMcastOk = setMcastWithV6IfaceName(mIfaces[i].sockFd, mIfaces[i].ifaceName.c_str());
    }
    else
    {
      setMcastOk = setMcastWithIfaceName(mIfaces[i].sockFd, mIfaces[i].ifaceName.c_str());
    }

    if (!setMcastOk)
    {
      LOG_ERROR("Setting mcast for " << mIfaces[i]);
      return false;
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

  // setup mcast IP address
  if (isIpV6())
  {
    if (inet_pton(AF_INET6, mMcastAddress.c_str(), &addr6.sin6_addr) != 1)
    {
      LOG_ERROR("Error parsing address for " << mMcastAddress);
      return false;
    }
  }
  else
  {
    addr.sin_addr.s_addr = inet_addr(mMcastAddress.c_str());
  }

  const string dmsg = "<Sender info:";
  int msgSeqNumber = 1;

  do
  {
    for (unsigned i = 0; i < mIfaces.size(); ++i)
    {
      const IfaceData ifaceData = mIfaces[i];
      int fd = ifaceData.sockFd;

      // build message
      std::stringstream msg;
      msg << dmsg << " " << ifaceData << ">";
      if (shouldLoop())
      {
        msg << " seqId " << msgSeqNumber;
      }

      // send message
      int byteSent;
      if (!isIpV6())
      {
        byteSent = sendto(fd, msg.str().c_str(), 1 + msg.str().size(),
                              MSG_NOSIGNAL, (struct sockaddr *) &addr, sizeof(addr));
      }
      else
      {
        byteSent = sendto(fd, msg.str().c_str(), 1 + msg.str().size(),
                              MSG_NOSIGNAL, (struct sockaddr *) &addr6, sizeof(addr6));
      }

      // Error check for sending message
      if (0 > byteSent)
      {
        LOG_ERROR("sendto " << ifaceData << " :" << strerror(errno));
        return false;
      }
      else
      {
        cout << "[OK] sent to [" << ifaceData << "] bytes: " << byteSent << endl;
      }
    }

    // loop interval
    if (shouldLoop())
    {
      ++msgSeqNumber;
      (void) usleep( (int)(mLoopInterval*1e6) );
    }

  } while (shouldLoop());

  return true;
}

bool SenderModule::setMcastWithIfaceName(int fd, const char* ifaceName)
{
  // Set the TTL hop count
  int opt = 6;
  int res = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, (void*) &opt, sizeof(opt));
  if (0 > res)
  {
    LOG_ERROR("sockopt TTL: " << strerror(errno));
    return false;
  }

  // Allow broadcast on this socket.
  opt = 1;
  res = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (void*) &opt, sizeof(opt));
  if (0 > res)
  {
    LOG_ERROR("sockopt SO_BROADCAST: " << strerror(errno));
    return false;
  }

  // Enable loop back if set.
  opt = mIsLoopBackOn;
  res = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, (void*) &opt, sizeof(opt));
  if (0 > res)
  {
    LOG_ERROR("sockopt IP_MULTICAST_LOOP: " << strerror(errno));
    return false;
  }

  // If iface name specified, set iface name for the socket
  if (0 < strlen(ifaceName))
  {
    struct ip_mreqn ifaddrn;
    memset(&ifaddrn, 0, sizeof(ifaddrn));

    ifaddrn.imr_ifindex = if_nametoindex(ifaceName);
    if (0 != (res = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &ifaddrn, sizeof(ifaddrn))))
    {
      LOG_ERROR("sockopt IP_MULTICAST_IF: " << strerror(errno));
    }
  }

  return true;
}

bool SenderModule::setMcastWithV6IfaceName(int fd, const char* ifaceName)
{
  // Set the TTL hop count
  int opt = 6;
  int res = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (void*) &opt, sizeof(opt));
  if (0 > res)
  {
    LOG_ERROR("sockopt TTL: " << strerror(errno));
    return false;
  }

  // Allow broadcast on this socket.
  opt = 1;
  res = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (void*) &opt, sizeof(opt));
  if (0 > res)
  {
    LOG_ERROR("sockopt SO_BROADCAST: " << strerror(errno));
    return false;
  }

  // Enable loop back if set.
  opt = mIsLoopBackOn;
  res = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (void*) &opt, sizeof(opt));
  if (0 > res)
  {
    LOG_ERROR("sockopt IPV6_MULTICAST_LOOP: " << strerror(errno));
    return false;
  }

  // If iface name specified, set iface name for the socket
  if (0 < strlen(ifaceName))
  {
    unsigned ifIndex = if_nametoindex(ifaceName);
    if (0 != (res = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifIndex, sizeof(ifIndex))))
    {
      LOG_ERROR("sockopt IPV6_MULTICAST_IF: " << strerror(errno));
      return false;
    }
  }

  return true;
}

bool SenderModule::shouldLoop() const
{
  return mLoopInterval > 0.0;
}
