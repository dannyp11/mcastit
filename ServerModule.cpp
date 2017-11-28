#include "ServerModule.h"

ServerModule::ServerModule(const vector<IfaceData>& ifaces,
    const vector<string>& mcastAddresses, int mcastPort,
    int nLoopbackIfaces, bool useIpV6, float loopInterval):
    SenderModule(ifaces, mcastAddresses, mcastPort, nLoopbackIfaces, useIpV6, loopInterval),
    mMcastListenSock(-1)
{
  mMcastSendPort = mMcastPort + 10;
  mUnicastSenderSock = -1;
  cout << "Periodical send to port " << mMcastSendPort << endl;
}

bool ServerModule::run()
{
  // Init RX socket
  mMcastListenSock = createSocket(isIpV6());
  if (-1 == mMcastListenSock)
  {
    LOG_ERROR("Cannot create listener socket");
    return false;
  }

  for (unsigned i = 0; i < mIfaces.size(); ++i)
  {
    int setOk;

    if (isIpV6())
    {
      setOk = joinMcastIfaceV6(mMcastListenSock, mIfaces[i].ifaceName.c_str());
    }
    else
    {
      setOk = joinMcastIface(mMcastListenSock, mIfaces[i].ifaceName.c_str());
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
  }

  // Init all sender interfaces
  if (!SenderModule::init())
  {
    return false;
  }

  // Spawn periodical sender
  pthread_t txThread;
  if (0 != pthread_create(&txThread, NULL, &ServerModule::txThreadHelper, this))
  {
    LOG_ERROR("Cannot spawn listener thread");
    return false;
  }
  else
  {
    cout << "Periodic sender thread started [OK]" << endl;
  }

  // Finally initialize unicast sender
  if (-1 == (mUnicastSenderSock = createSocket(isIpV6())))
  {
    LOG_ERROR("Cannot create socket");
    return false;
  }

  if (-1 == setReuseSocket(mUnicastSenderSock))
  {
    LOG_ERROR("Cannot set reuse socket");
    return false;
  }

  // Bind the unicast socket
  cout << "==============================================================" << endl;

  //-----------------------------------------------------------------------
  //
  // now prepare for select function
  //
  int maxSockFd = mMcastListenSock;
  for (unsigned i = 0; i < mIfaces.size(); ++i)
  {
    maxSockFd = (maxSockFd < mIfaces[i].sockFd)? mIfaces[i].sockFd : maxSockFd;
  }

  struct timeval timeout;
  fd_set rfds;
  char buffer[MCAST_BUFF_LEN];
  //-----------------------------------------------------------------------

  // Main select loop -----------------------------------------------------
  while (1)
  {
    FD_ZERO(&rfds);
    FD_SET(mMcastListenSock, &rfds);
    FD_SET(mUnicastSenderSock, &rfds);
    for (unsigned i = 0; i < mIfaces.size(); ++i)
    {
      FD_SET(mIfaces[i].sockFd, &rfds);
    }

    timeout.tv_sec = 1;
    timeout.tv_usec = 1;
    int numReady = select(maxSockFd + 1, &rfds, NULL, NULL, &timeout);
    if (numReady <= 0)
    {
      if (-1 == numReady)
      {
        LOG_ERROR("select: " << strerror(errno));
      }
      continue;
    }

    // for each found sock fd that detected from select()
    for (int i = 0; i < numReady; ++i)
    {
      // Retrieve the fd that has data
      IfaceData ifaceData;
      int fd = -1;
      if (FD_ISSET(mUnicastSenderSock, &rfds))
      {
        fd = mUnicastSenderSock;
      }
      else if (FD_ISSET(mMcastListenSock, &rfds))
      {
        fd = mMcastListenSock;
      }
      else
      {
        for (unsigned ii = 0; ii < mIfaces.size(); ++ii)
        {
          fd = mIfaces[ii].sockFd;
          if (FD_ISSET(fd, &rfds))
          {
            break;
          }
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
      if (decodeAckMessage(buffer, decodedMsg))
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
          printf("%-40s - %s\n", senderIp, buffer);
        }
        else
        {
          printf("%-15s - %s\n", senderIp, buffer);
        }
      }

      // Build response message
      string responseMsg;
      encodeAckMessage(buffer, responseMsg);
      if (!unicastMessage(mUnicastSenderSock, sender, responseMsg))
      {
        LOG_ERROR("sending ack message to " << senderIp);
      }

      // rm fd so that it's not processed again
      FD_CLR(fd, &rfds);
      break;
    }
  }
  // ----------------------------------------------------------------------

  return true;
}

ServerModule::~ServerModule()
{
  ::close(mMcastListenSock);
  ::close(mUnicastSenderSock);
}

void* ServerModule::txThreadHelper(void* context)
{
  static int randNum = 1;
  ServerModule* module = (ServerModule*)context;
  if (module->sendMcastMessages(module->mMcastSendPort))
  {
    return 0;
  }
  return &randNum;
}
