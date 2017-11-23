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

  mIsStopped = false;
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

  // Spawn listener thread
  pthread_t rxThread;
  if (0 != pthread_create(&rxThread, NULL, &SenderModule::rxThreadHelper, this))
  {
    LOG_ERROR("Cannot spawn listener thread");
    return false;
  }
  else
  {
    cout << "Listener thread started [OK]" << endl;
  }
  cout << "==============================================================" << endl;

  // build addr struct
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(mMcastPort);

  struct sockaddr_in6 addr6;
  addr6.sin6_family = AF_INET6;
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
    char msgBuf[MCAST_BUFF_LEN];
    for (unsigned i = 0; i < mIfaces.size(); ++i)
    {
      const IfaceData ifaceData = mIfaces[i];
      int fd = ifaceData.sockFd;

      // build message
      memset(msgBuf, 0 ,sizeof(msgBuf));
      if (shouldLoop())
      {
        sprintf(msgBuf, "%4d ", msgSeqNumber);
      }
      sprintf(msgBuf, "%s%s %s>", msgBuf, dmsg.c_str(), ifaceData.toString().c_str());

      // send message
      int byteSent;
      if (!isIpV6())
      {
        byteSent = sendto(fd, msgBuf, strlen(msgBuf) + 1,
                    MSG_NOSIGNAL|MSG_DONTWAIT, (struct sockaddr *) &addr, sizeof(addr));
      }
      else
      {
        byteSent = sendto(fd, msgBuf, strlen(msgBuf) + 1,
                    MSG_NOSIGNAL|MSG_DONTWAIT, (struct sockaddr *) &addr6, sizeof(addr6));
      }

      // Error check for sending message
      if (0 > byteSent)
      {
        LOG_ERROR("sendto " << ifaceData << " :" << strerror(errno));
        return false;
      }
      else
      {
        LOG_DEBUG("[SENT] " << ifaceData << " bytes: " << byteSent);
      }
    }

    // loop interval
    if (shouldLoop())
    {
      ++msgSeqNumber;
      (void) usleep( (int)(mLoopInterval*1e6) );
    }

  } while (shouldLoop());

  // Allow ack listener for 3 seconds
  sleep(3);
  mIsStopped = true;
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

  // Enable reuse ip/port
  if (-1 == setReuseSocket(fd))
  {
    LOG_ERROR("sockopt cannot reuse socket " << fd);
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

  // Next, bind socket to a fix port
  in_addr_t meSinAddr = htonl(INADDR_ANY);
  vector<string> meIps;
  if (0 == getIfaceIPFromIfaceName(ifaceName, meIps, false))
  {
    meSinAddr = inet_addr(meIps[0].c_str());
  }

  struct sockaddr_in me_addr;
  memset((char*) &me_addr, 0, sizeof(me_addr));
  me_addr.sin_family = AF_INET;
  me_addr.sin_port = htons(mMcastPort);
  me_addr.sin_addr.s_addr = meSinAddr;
  if (-1 == ::bind(fd, (struct sockaddr*)&me_addr, sizeof(me_addr)))
  {
    LOG_ERROR("sockopt bind: " << strerror(errno));
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

  // Enable reuse ip/port
  if (-1 == setReuseSocket(fd))
  {
    LOG_ERROR("sockopt cannot reuse socket " << fd);
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

  // Bind the socket to the multicast port.
  struct sockaddr_in6 bindAddr6;
  vector<string> meIps;
  if (0 == getIfaceIPFromIfaceName(ifaceName, meIps, true))
  {
    if (0 == inet_pton(AF_INET6, meIps[0].c_str(), &bindAddr6.sin6_addr))
    {
      LOG_ERROR("Error parsing address for " << mMcastAddress);
      return -1;
    }
  }

  memset(&bindAddr6, 0, sizeof(bindAddr6));
  bindAddr6.sin6_family = AF_INET6;
  bindAddr6.sin6_port = htons(mMcastPort);
  res = ::bind(fd, (struct sockaddr *) &bindAddr6, sizeof(bindAddr6));
  if (0 > res)
  // ignore EINVAL for allowing multiple mcast interfaces in same socket
  {
    LOG_ERROR("bind: " << strerror(errno));
    return -1;
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

void* SenderModule::runUcastReceiver()
{
  // Now listen to ack msgs
  char rxBuf[MCAST_BUFF_LEN];
  struct sockaddr_storage rmt;
  fd_set listenSet;
  struct timeval timeout;
  int maxFd = -1;
  for (unsigned i = 0; i < mIfaces.size(); ++i)
  {
    maxFd = (maxFd < mIfaces[i].sockFd)? mIfaces[i].sockFd : maxFd;
  }

  while (!mIsStopped)
  {
    LOG_DEBUG("listening...");
    FD_ZERO(&listenSet);
    for (unsigned i = 0; i < mIfaces.size(); ++i)
    {
      FD_SET(mIfaces[i].sockFd, &listenSet);
    }

    timeout.tv_sec = 1;
    timeout.tv_usec = 1;

    int numReady = select(maxFd + 1, &listenSet, NULL, NULL, &timeout);
    if (numReady == 0)
    {
      continue;
    }

    while(numReady-- > 0)
    {
      int resultFd = -1;
      for (unsigned ii = 0 ; ii < mIfaces.size(); ++ii)
      {
        resultFd = mIfaces[ii].sockFd;
        if (FD_ISSET(resultFd, &listenSet))
        {
          FD_CLR(resultFd, &listenSet);
          break;
        }
      }

      // Get respond data
      memset(rxBuf, 0, sizeof(rxBuf));
      socklen_t rmtLen = sizeof(rmt);
      int rxBytes = recvfrom(resultFd, rxBuf, sizeof(rxBuf), 0,
                                (struct sockaddr*) &rmt, &rmtLen);

      if (-1 == rxBytes)
      {
        LOG_ERROR("recvfrom: "<< strerror(errno));
        sleep(1);
        continue;
      }

      // get the sender info
      char senderIp[INET6_ADDRSTRLEN];
      if (rmt.ss_family == AF_INET)
      {
        struct sockaddr_in *sender_addr = (struct sockaddr_in*) &rmt;
        inet_ntop(rmt.ss_family, &sender_addr->sin_addr, senderIp, sizeof(senderIp));
      }
      else if (rmt.ss_family == AF_INET6)
      {
        struct sockaddr_in6 *sender_addr = (struct sockaddr_in6*) &rmt;
        inet_ntop(rmt.ss_family, &sender_addr->sin6_addr, senderIp, sizeof(senderIp));
      }

      string decodedMsg;
      if (decodeAckMessage(rxBuf, decodedMsg))
      {
        if (isIpV6())
        {
          printf("[ACK] %-45s (%s)\n", senderIp, decodedMsg.c_str());
        }
        else
        {
          printf("[ACK] %-15s (%s)\n", senderIp, decodedMsg.c_str());
        }
      }
      else
      {
        cout << "Stray message: " << rxBuf << endl;
      }
    }
  }

  return 0;
}

bool SenderModule::shouldLoop() const
{
  return mLoopInterval > 0.0;
}

void* SenderModule::rxThreadHelper(void* context)
{
  return ((SenderModule*)context)->runUcastReceiver();
}
