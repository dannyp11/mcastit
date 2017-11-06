#include "Common.h"

#define DEFAULT_IP_ADDRESS  "0.0.0.0"
#define DEFAULT_IFACE       "default"

static struct ifaddrs *g_ifap;

int createSocketFromIfaceName(const string& ifaceName, string& ifaceIpAdress, bool isIpV6)
{
  ifaceIpAdress = DEFAULT_IP_ADDRESS;
  int sockFamily = (isIpV6) ? AF_INET6 : AF_INET;

  if (!ifaceName.size())
  {
    return -1;
  }

  int fd = socket(sockFamily, SOCK_DGRAM, 0);
  if (-1 == fd)
  {
    perror("creating fd");
    return -1;
  }

  /*
   * Now get interface ip address from interface name
   */
  static bool alreadyHasIfap = false;
  if (!alreadyHasIfap)
    getifaddrs(&g_ifap);
  alreadyHasIfap = true;
  char addr[INET6_ADDRSTRLEN];
  int getnameErrCode;

  for (struct ifaddrs* ifa = g_ifap; ifa; ifa = ifa->ifa_next)
  {
    if (isIpV6 && ifa->ifa_addr->sa_family == AF_INET6)
    {
      if (ifaceName == ifa->ifa_name)
      {
        if (0
            == (getnameErrCode = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), addr,
                sizeof(addr), NULL, 0, NI_NUMERICHOST)))
        {
          ifaceIpAdress = addr;
          break;
        }
        else
        {
          cout << "Error getting IPv6 address for " << ifaceName << ": "
              << gai_strerror(getnameErrCode) << endl;
          close(fd);
          return -1;
        }
      }
    }
  }

  return fd;
}

const string& IfaceData::toString() const
{
  static string result;
  std::stringstream stm;

  stm << getReadableName() << " (" << getReadableAddress() << ")";
  result = stm.str();
  return result;
}

const string& IfaceData::getReadableName() const
{
  static string result = DEFAULT_IFACE;
  if (ifaceName.size())
    result = ifaceName;
  return result;
}

const string& IfaceData::getReadableAddress() const
{
  static string result = DEFAULT_IP_ADDRESS;
  if (ifaceAddress.size())
    result = ifaceAddress;
  return result;
}

std::ostream & operator<<(std::ostream &os, const IfaceData& iface)
{
  return os << iface.toString();
}

void cleanupCommon()
{
  freeifaddrs(g_ifap);
}
