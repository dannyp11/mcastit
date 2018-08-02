#include "ReceiverModule.h"

ReceiverModule::ReceiverModule(const vector<IfaceData>& ifaces,
    const vector<string>& mcastAddresses, int mcastPort, bool useIpV6) :
    McastModuleInterface(ifaces, mcastAddresses, mcastPort, useIpV6)
{
  mUnicastSenderSock = -1;
}

ReceiverModule::~ReceiverModule()
{
  ::close(mUnicastSenderSock);
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
      setOk = joinMcastIface(fd, mIfaces[i].ifaceName.c_str());
    }

    if (setOk != 0)
    {
      LOG_ERROR("Error " << setOk << " setting mcast for " << mIfaces[i]);
      continue;
    }
    else
    {
      cout << "Interface " << mIfaces[i] << " [OK]" << endl;
    }

    maxSockD = (maxSockD < fd) ? fd : maxSockD;
  }

  // Finally initialize unicast sender
  if (-1 == (mUnicastSenderSock = Common::createSocket(isIpV6())))
  {
    LOG_ERROR("Cannot create socket");
    return false;
  }

  if (-1 == Common::setReuseSocket(mUnicastSenderSock))
  {
    LOG_ERROR("Cannot set reuse socket");
    return false;
  }

  // Bind ucast sock port to mcast port
  int ret = -1;
  if (isIpV6())
  {
    struct sockaddr_in6 ucastSrcAddr;
    memset(&ucastSrcAddr, 0, sizeof(ucastSrcAddr));
    ucastSrcAddr.sin6_family = AF_INET6;
    ucastSrcAddr.sin6_port = htons(0);
    ret = ::bind(mUnicastSenderSock, (struct sockaddr *) &ucastSrcAddr, sizeof(ucastSrcAddr));
  }
  else
  {
    struct sockaddr_in ucastSrcAddr;
    memset(&ucastSrcAddr, 0, sizeof(ucastSrcAddr));
    ucastSrcAddr.sin_family = AF_INET;
    ucastSrcAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ucastSrcAddr.sin_port = htons(mMcastPort);
    ret = ::bind(mUnicastSenderSock, (struct sockaddr*) &ucastSrcAddr, sizeof(ucastSrcAddr));
  }

  if (ret < 0)
  {
    LOG_ERROR("Cannot bind ack source port " << strerror(errno));
    return false;
  }
  cout << "==============================================================" << endl;

  /**
   * Setup fdset
   */
  struct timeval timeout;
  fd_set rfds;
  char buffer[MCAST_BUFF_LEN];
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
    if (numReady <= 0)
    {
      continue;
    }

    // for each found sock fd that detected from select()
    for (int i = 0; i < numReady; ++i)
    {
      // Retrieve the fd that has data
      int fd = -1;
      std::string recvIface = "default";
      for (unsigned ii = 0; ii < mIfaces.size(); ++ii)
      {
        fd = mIfaces[ii].sockFd;
        if (FD_ISSET(fd, &rfds))
        {
          FD_CLR(fd, &rfds);
          recvIface = mIfaces[ii].ifaceName;
          break;
        }
      }

      // get sender data
      struct sockaddr_storage sender;
      socklen_t sendsize = sizeof(sender);
      bzero(&sender, sizeof(sender));

      memset(buffer, 0, sizeof(buffer));
      int recvLen = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*) &sender, &sendsize);
      if (0 > recvLen)
      {
        LOG_ERROR("recvfrom " << fd << ": " << strerror(errno));
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
      string decodedMsg;
      if (Common::decodeAckMessage(buffer, decodedMsg))
      {
        if (isIpV6())
        {
          printf("[ACK] %-40s (%s)\n", senderIp, decodedMsg.c_str());
        }
        else
        {
          printf("[ACK] %-15s (%s)\n", senderIp, decodedMsg.c_str());
        }
      }
      else
      {
        if (isIpV6())
        {
          printf("%-40s -> %-15s - %s\n", senderIp, recvIface.c_str(), decodedMsg.c_str());
        }
        else
        {
          printf("%-15s -> %-15s - %s\n", senderIp, recvIface.c_str(), decodedMsg.c_str());
        }
      }


      // Build response message
      string responseMsg;
      Common::encodeAckMessage(buffer, responseMsg);
      if (!Common::unicastMessage(mUnicastSenderSock, sender, responseMsg))
      {
        LOG_ERROR("sending ack message to " << senderIp);
      }

      // rm fd so that it's not processed again
      FD_CLR(fd, &rfds);
      break;
    }
  }

  return true;
}
