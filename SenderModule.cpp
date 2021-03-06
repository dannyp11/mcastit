#include "SenderModule.h"

SenderModule::SenderModule(const vector<IfaceData>& ifaces,
    const vector<string>& mcastAddresses, int mcastPort,
    int nLoopbackIfaces, bool useIpV6, float loopInterval) :
    McastModuleInterface(ifaces, mcastAddresses, mcastPort, useIpV6),
    mLoopbackCount(nLoopbackIfaces), mLoopInterval(loopInterval)
{
  std::stringstream loopMsg;
  if (nLoopbackIfaces>=0)
  {
    loopMsg << nLoopbackIfaces;
  }
  else
  {
    loopMsg << "all";
  }
  cout << "Sending with " << loopMsg.str() << " interface loopbacks";

  if (shouldLoop())
  {
    cout << " with interval " << mLoopInterval << " second(s)";
  }
  cout << endl;

  mIsStopped = false;
  mSenderPort = mMcastPort+1;
}

bool SenderModule::run()
{
  bool retVal = true;
  if (!init())
  {
    LOG_ERROR("Error initing sender module");
    return false;
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

  if (!sendMcastMessages())
  {
    LOG_ERROR("There's problem sending in SenderModule");
    retVal = false;
  }

  // Allow ack listener for 3 seconds
  sleep(3);
  mIsStopped = true;
  return retVal;
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
    if (numReady <= 0)
    {
      if (numReady<0)
      {
        LOG_ERROR("select: " << strerror(errno));
      }
      continue;
    }

    while(numReady-- > 0)
    {
      int resultFd = -1;
      std::string recvIfaceName = "default";

      for (unsigned ii = 0 ; ii < mIfaces.size(); ++ii)
      {
        resultFd = mIfaces[ii].sockFd;
        if (FD_ISSET(resultFd, &listenSet))
        {
          FD_CLR(resultFd, &listenSet);
          recvIfaceName = mIfaces[ii].ifaceName;
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
      if (Common::decodeAckMessage(rxBuf, decodedMsg))
      {
        if (isIpV6())
        {
          printf("[ACK] %-45s -> %-10s (%s)\n", senderIp, recvIfaceName.c_str(), decodedMsg.c_str());
        }
        else
        {
          printf("[ACK] %-15s -> %-10s (%s)\n", senderIp, recvIfaceName.c_str(), decodedMsg.c_str());
        }
      }
      else
      {
        printf("[STRAY] %-15s -> %-10s (%s)\n", senderIp, recvIfaceName.c_str(), rxBuf);
      }
    }
  }

  return 0;
}

bool SenderModule::shouldLoop() const
{
  return mLoopInterval > 0.0;
}

bool SenderModule::sendMcastMessages(int port)
{
  // build addr struct vector
  // if port is -1, use mcast port
  port = (-1 == port)? mMcastPort: port;
  vector<struct sockaddr_in> addrVec;
  vector<struct sockaddr_in6> addr6Vec;

  for (unsigned i = 0; i < mMcastAddresses.size(); ++i)
  {
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(port);

    // setup mcast IP address
    if (isIpV6())
    {
      if (inet_pton(AF_INET6, mMcastAddresses[i].c_str(), &addr6.sin6_addr) != 1)
      {
        LOG_ERROR("Error parsing address for " << mMcastAddresses[i]);
        return false;
      }
      addr6Vec.push_back(addr6);
    }
    else
    {
      addr.sin_addr.s_addr = inet_addr(mMcastAddresses[i].c_str());
      addrVec.push_back(addr);
    }
  }

  // build message
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
      for (unsigned i = 0; i < addrVec.size() + addr6Vec.size(); ++i)
      {
        const struct sockaddr_in& addr = addrVec[i];
        const struct sockaddr_in6& addr6 = addr6Vec[i];
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

void* SenderModule::rxThreadHelper(void* context)
{
  return ((SenderModule*)context)->runUcastReceiver();
}

bool SenderModule::init()
{
  /*
   * First, setup mcast for all interfaces
   */
  for (unsigned i = 0; i < mIfaces.size(); ++i)
  {
    bool setMcastOk;
    if (isIpV6())
    {
      setMcastOk = associateMcastV6WithIfaceName(mIfaces[i].sockFd,
          mIfaces[i].ifaceName.c_str(), mLoopbackCount<0 || (int)i < mLoopbackCount);
    }
    else
    {
      setMcastOk = associateMcastWithIfaceName(mIfaces[i].sockFd,
          mIfaces[i].ifaceName.c_str(), mLoopbackCount<0 || (int)i < mLoopbackCount);
    }

    if (!setMcastOk)
    {
      LOG_ERROR("Setting mcast for " << mIfaces[i]);
      return false;
    }
  }

  return true;
}
