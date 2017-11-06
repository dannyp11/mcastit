#include "ReceiverModule.h"

ReceiverModule::ReceiverModule(const vector<IfaceData>& ifaces,
    const string& mcastAddress, int mcastPort) :
    McastModuleInterface(ifaces, mcastAddress, mcastPort, true)
{
}

ReceiverModule::~ReceiverModule()
{
}

bool ReceiverModule::run()
{
  int maxSockD = -1;

  cout << "Listening ..."<< endl;
  for (unsigned i = 0; i < mIfaces.size(); ++i)
  {
    int fd = mIfaces[i].sockFd;
    int setOk = joinMcastIface(fd, mIfaces[i].ifaceName.c_str());
    if (setOk != 0)
    {
      cout << "Error " << setOk << " setting mcast for " << mIfaces[i] << endl;
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
      continue;

    // print message
    for (int i = 0; i < numReady; ++i)
    {
      for (unsigned ii = 0; ii < mIfaces.size(); ++ii)
      {
        int fd = mIfaces[ii].sockFd;
        if (FD_ISSET(fd, &rfds))
        {
          uint8_t buff_[1024];
          uint8_t *rbuff = buff_;
          uint32_t rbuffsz = sizeof(buff_);

          // get sender data
          struct sockaddr_storage sender;
          socklen_t sendsize = sizeof(sender);
          bzero(&sender, sizeof(sender));

          ssize_t dataSize = ::recvfrom(fd, rbuff, rbuffsz, 0, (struct sockaddr*) &sender,
              &sendsize);

          if (0 > dataSize)
          {
            perror("recvfrom");
          }

          /*
           * Get ifaceName from fd
           */
          string ifaceName = "not found";
          for (unsigned iii = 0; iii < mIfaces.size(); ++iii)
          {
            if (mIfaces[iii].sockFd == fd)
            {
              ifaceName = mIfaces[iii].getReadableName();
              break;
            }
          }

          struct sockaddr_in *sender_addr = (struct sockaddr_in*) &sender;
          printf("%s ->%10s: ", inet_ntoa(sender_addr->sin_addr), ifaceName.c_str());
          cout << rbuff << endl;

          FD_CLR(fd, &rfds);
          break;
        }
      }
    }
  }
}

int ReceiverModule::joinMcastIface(int sock, const char* ifaceName)
{
  // Set the recv buffer size.
  int res;
  uint32_t buffSz = 1024;
  res = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buffSz, sizeof(buffSz));
  if (0 > res)
  {
    perror("ERR sockopt BuffSz.");
  }

  // Let's set reuse port to on to allow multiple binds per host.
  int opt = 1;
  res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (0 > res)
  {
    perror("ERR sockopt REUSEADDR.");
    return -1;
  }

  // Bind the socket to the multicast port.
  struct sockaddr_in bindAddr;
  memset(&bindAddr, 0, sizeof(bindAddr));
  bindAddr.sin_family = AF_INET;
  bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bindAddr.sin_port = htons(mMcastPort);
  res = ::bind(sock, (struct sockaddr *) &bindAddr, sizeof(bindAddr));
  if (0 > res)
  {
    perror("ERR bind.");
    return -1;
  }

  // Now we need to see if they have specified which interface we need to
  // multicast out of.
  {
    struct ip_mreq mcastReq;
    struct ip_mreqn mcastReqn;
    res = -1;

    if (0 == strlen(ifaceName))
    {
      mcastReq.imr_multiaddr.s_addr = inet_addr(mMcastAddress.c_str());
      mcastReq.imr_interface.s_addr = htonl(INADDR_ANY);
      res = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*) &mcastReq, sizeof(mcastReq));
    }
    else
    {
      mcastReqn.imr_multiaddr.s_addr = inet_addr(mMcastAddress.c_str());
      mcastReqn.imr_ifindex = if_nametoindex(ifaceName);
      res = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*) &mcastReqn, sizeof(mcastReqn));
    }

    if (0 != res)
    {
      printf("ERR: join mcast GRP<%s> INF<%s> ERR<%s>\n", mMcastAddress.c_str(), ifaceName,
          strerror(errno));
    }
  }

  return res;
}
