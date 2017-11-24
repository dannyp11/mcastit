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

bool SenderModule::sendMcastMessages(int port)
{
  // build addr struct
  port = (-1 == port)? mMcastPort: port;
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  struct sockaddr_in6 addr6;
  addr6.sin6_family = AF_INET6;
  addr6.sin6_port = htons(port);

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
          mIfaces[i].ifaceName.c_str(), mIsLoopBackOn);
    }
    else
    {
      setMcastOk = associateMcastWithIfaceName(mIfaces[i].sockFd,
          mIfaces[i].ifaceName.c_str(), mIsLoopBackOn);
    }

    if (!setMcastOk)
    {
      LOG_ERROR("Setting mcast for " << mIfaces[i]);
      return false;
    }
  }

  return true;
}
