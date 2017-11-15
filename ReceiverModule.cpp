#include "ReceiverModule.h"

ReceiverModule::ReceiverModule(const vector<IfaceData>& ifaces,
    const string& mcastAddress, int mcastPort, bool useIpV6) :
    McastModuleInterface(ifaces, mcastAddress, mcastPort, useIpV6)
{
}

bool ReceiverModule::run()
{
  int maxSockD = -1;

  cout << "Listening ..."<< endl;
  for (unsigned i = 0; i < mIfaces.size(); ++i)
  {
    int fd = mIfaces[i].sockFd;
    int setOk;

    if (isIpV6())
    {
      setOk = joinMcastIfaceV6(fd, mIfaces[i].ifaceName.c_str());
    }
    else
    {
      if (mIfaces[i].mustUseIPAddress)
      {
        // join using IP address
        setOk = joinMcastIfaceAddress(fd, mIfaces[i].ifaceAddress.c_str());
      }
      else
      {
        // join using interface name
        setOk = joinMcastIface(fd, mIfaces[i].ifaceName.c_str());
      }
    }

    if (setOk != 0)
    {
      LOG_ERROR("Error " << setOk << " setting mcast for " << mIfaces[i]);
      return false;
    }
    else
    {
      cout << "Interface " << mIfaces[i] << " [OK]" << endl;
    }

    maxSockD = (maxSockD < fd) ? fd : maxSockD;
  }
  cout << "==============================================================" << endl;

  /**
   * Setup fdset
   */
  struct timeval timeout;
  fd_set rfds;
  while (1)
  {
    FD_ZERO(&rfds);
    for (unsigned i = 0; i < mIfaces.size(); ++i)
    {
      FD_SET(mIfaces[i].sockFd, &rfds);
    }

    timeout.tv_sec = 1;
    timeout.tv_usec = 1;
    int numReady = select(maxSockD + 1, &rfds, NULL, NULL, &timeout);
    if (numReady == 0)
    {
      continue;
    }

    // for each found sock fd that detected from select()
    for (int i = 0; i < numReady; ++i)
    {
      // Retrieve the fd that has data
      IfaceData ifaceData;
      int fd;

      bool foundIfaceData = false;
      for (unsigned ii = 0; ii < mIfaces.size(); ++ii)
      {
        fd = mIfaces[ii].sockFd;
        if (FD_ISSET(fd, &rfds))
        {
          ifaceData = mIfaces[ii];
          FD_CLR(fd, &rfds);
          foundIfaceData = true;
          break;
        }
      }

      if (!foundIfaceData)
      {
        LOG_ERROR("No socket fd found in member var, something is very wrong here");
        return false;
      }

      int8_t buffer[MCAST_BUFF_LEN];

      // get sender data
      struct sockaddr_storage sender;
      socklen_t sendsize = sizeof(sender);
      bzero(&sender, sizeof(sender));

      if (0 > recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*) &sender, &sendsize))
      {
        LOG_ERROR("recvfrom " << ifaceData << ": " << strerror(errno));
        continue;
      }

      // get the sender info
      char senderIp[INET6_ADDRSTRLEN];
      if (sender.ss_family == AF_INET)
      {
        struct sockaddr_in *sender_addr = (struct sockaddr_in*) &sender;
        inet_ntop(sender.ss_family, &sender_addr->sin_addr, senderIp, sizeof(senderIp));
      }
      else if (sender.ss_family == AF_INET6)
      {
        struct sockaddr_in6 *sender_addr = (struct sockaddr_in6*) &sender;
        inet_ntop(sender.ss_family, &sender_addr->sin6_addr, senderIp, sizeof(senderIp));
      }

      // print result message
      printf("%s - %s\n", senderIp, buffer);

      // rm fd so that it's not processed again
      FD_CLR(fd, &rfds);
      break;
    }
  }

  return true;
}

int ReceiverModule::joinMcastIface(int sock, const char* ifaceName)
{
  // Set the recv buffer size.
  int res;
  uint32_t buffSz = MCAST_BUFF_LEN;
  res = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buffSz, sizeof(buffSz));
  if (0 > res)
  {
    LOG_ERROR("joinMcastIface sockopt BuffSz: " << strerror(errno));
    return -1;
  }

  // Let's set reuse port to on to allow multiple binds per host.
  int opt = 1;
  res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (0 > res)
  {
    LOG_ERROR("joinMcastIface sockopt SO_REUSEADDR: " << strerror(errno));
    return -1;
  }

  // Bind the socket to the multicast port.
  struct sockaddr_in bindAddr;
  memset(&bindAddr, 0, sizeof(bindAddr));
  bindAddr.sin_family = AF_INET;
  bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bindAddr.sin_port = htons(mMcastPort);
  res = ::bind(sock, (struct sockaddr *) &bindAddr, sizeof(bindAddr));
  if (0 > res && errno != EINVAL)
  // ignore EINVAL for allowing multiple mcast interfaces in same socket
  {
    LOG_ERROR("joinMcastIface bind: " << strerror(errno));
    return -1;
  }

  // If ifaceName is specified, bind directly to that iface,
  // otherwise bind to general interface
  if (0 == strlen(ifaceName))
  {
    struct ip_mreq mcastReq;
    mcastReq.imr_multiaddr.s_addr = inet_addr(mMcastAddress.c_str());
    mcastReq.imr_interface.s_addr = htonl(INADDR_ANY);
    res = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*) &mcastReq, sizeof(mcastReq));
  }
  else
  {
    struct ip_mreqn mcastReqn;
    mcastReqn.imr_multiaddr.s_addr = inet_addr(mMcastAddress.c_str());
    mcastReqn.imr_ifindex = if_nametoindex(ifaceName);
    res = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*) &mcastReqn, sizeof(mcastReqn));
  }

  if (0 != res)
  {
    printf("Error: join mcast group<%s> interface<%s>: %s\n",
              mMcastAddress.c_str(), ifaceName, strerror(errno));
  }

  return res;
}

int ReceiverModule::joinMcastIfaceV6(int sock, const char* ifaceName)
{
  // Set the recv buffer size.
  int res;
  uint32_t buffSz = MCAST_BUFF_LEN;
  res = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buffSz, sizeof(buffSz));
  if (0 > res)
  {
    LOG_ERROR("joinMcastIfaceV6 sockopt BuffSz: " << strerror(errno));
    return -1;
  }

  // Let's set reuse port to on to allow multiple binds per host.
  int opt = 1;
  res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (0 > res)
  {
    LOG_ERROR("joinMcastIfaceV6 sockopt SO_REUSEADDR: " << strerror(errno));
    return -1;
  }

  // Bind the socket to the multicast port.
  struct sockaddr_in6 bindAddr;
  memset(&bindAddr, 0, sizeof(bindAddr));
  bindAddr.sin6_family = AF_INET6;
  bindAddr.sin6_port = htons(mMcastPort);
  res = ::bind(sock, (struct sockaddr *) &bindAddr, sizeof(bindAddr));
  if (0 > res && errno != EINVAL)
    // ignore EINVAL for allowing multiple mcast interfaces in same socket
  {
    LOG_ERROR("joinMcastIfaceV6 bind: " << strerror(errno));
    return -1;
  }

  // If ifaceName is specified, bind directly to that iface,
  // otherwise bind to general interface
  struct ipv6_mreq mcastReq;
  if (inet_pton(AF_INET6, mMcastAddress.c_str(), &mcastReq.ipv6mr_multiaddr) != 1)
  {
    LOG_ERROR("Error parsing address for " << mMcastAddress);
    return -1;
  }

  mcastReq.ipv6mr_interface = if_nametoindex(ifaceName); // no need to check since it will return 0 on error
  res = setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (void*) &mcastReq, sizeof(mcastReq));

  if (0 != res)
  {
    printf("ERR: join mcast GRP<%s> INF<%s> ERR<%s>\n", mMcastAddress.c_str(), ifaceName,
        strerror(errno));
  }

  return res;
}

int ReceiverModule::joinMcastIfaceAddress(int sock, const char* ifaceIpAddress)
{
  // Set the recv buffer size.
  int res;
  uint32_t buffSz = MCAST_BUFF_LEN;
  res = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buffSz, sizeof(buffSz));
  if (0 > res)
  {
    LOG_ERROR("joinMcastIfaceAddress sockopt BuffSz: " << strerror(errno));
    return -1;
  }

  // Let's set reuse port to on to allow multiple binds per host.
  int opt = 1;
  res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (0 > res)
  {
    LOG_ERROR("joinMcastIfaceAddress sockopt SO_REUSEADDR: " << strerror(errno));
    return -1;
  }

  // Bind the socket to the multicast port.
  struct sockaddr_in bindAddr;
  memset(&bindAddr, 0, sizeof(bindAddr));
  bindAddr.sin_family = AF_INET;
  bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bindAddr.sin_port = htons(mMcastPort);
  res = ::bind(sock, (struct sockaddr *) &bindAddr, sizeof(bindAddr));
  if (0 > res && errno != EINVAL)
    // ignore EINVAL for allowing multiple mcast interfaces in same socket
  {
    LOG_ERROR("joinMcastIfaceAddress bind: " << strerror(errno));
    return -1;
  }

  // If ifaceIpAddress is specified, bind directly to that IP,
  // otherwise bind to general interface
  struct ip_mreq mcastReq;
  if (0 == strlen(ifaceIpAddress))
  {
    mcastReq.imr_interface.s_addr = htonl(INADDR_ANY);
  }
  else
  {
    mcastReq.imr_interface.s_addr = inet_addr(ifaceIpAddress);
  }

  mcastReq.imr_multiaddr.s_addr = inet_addr(mMcastAddress.c_str());
  res = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*) &mcastReq, sizeof(mcastReq));
  if (0 != res)
  {
    printf("Error: join mcast group<%s> interface<%s>: %s\n",
              mMcastAddress.c_str(), ifaceIpAddress, strerror(errno));
  }

  return res;
}
