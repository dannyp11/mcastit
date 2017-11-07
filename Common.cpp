#include "Common.h"

#define DEFAULT_IP_ADDRESS  "0.0.0.0"
#define DEFAULT_IFACE       "default"

static struct ifaddrs *g_ifap;
static bool g_debugMode = false;

int createSocket(bool isIpV6)
{
  int sockFamily = (isIpV6) ? AF_INET6 : AF_INET;
  return socket(sockFamily, SOCK_DGRAM, 0);
}

int createSocketFromIfaceName(const string& ifaceName, string& ifaceIpAdress, bool isIpV6)
{
  ifaceIpAdress = "";
  if (!ifaceName.size())
  {
    return -1;
  }

  int fd = createSocket(isIpV6);
  if (-1 == fd)
  {
    return -1;
  }

  /*
   * Now get interface ip address from interface name
   */
  static bool alreadyHasIfap = false;
  if (!alreadyHasIfap)
  {
    if (0 != getifaddrs(&g_ifap))
    {
      LOG_ERROR("Can't get system interfaces: " << strerror(errno));
      return -1;
    }
    alreadyHasIfap = true;
  }

  char addr[INET6_ADDRSTRLEN];
  int getnameErrCode;

  for (struct ifaddrs* ifa = g_ifap; ifa; ifa = ifa->ifa_next)
  {
    // make sure ifa_addr is valid
    if (!ifa->ifa_addr)
    {
      continue;
    }

    // Check ipv6 interface
    if (isIpV6 && ifa->ifa_addr->sa_family == AF_INET6)
    {
      if (ifaceName == ifa->ifa_name)
      {
        if (0 == (getnameErrCode = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6),
                                          addr, sizeof(addr), NULL, 0, NI_NUMERICHOST)))
        {
          ifaceIpAdress = addr;
          break;
        }
        else
        {
          LOG_ERROR("Error getting IPv6 address for " << ifaceName << ": "
                      << gai_strerror(getnameErrCode));
          close(fd);
          return -1;
        }
      }
    }
    // check ipv4 interface
    else if (!isIpV6 && ifa->ifa_addr->sa_family == AF_INET)
    {
      if (ifaceName == ifa->ifa_name)
      {
        if (0 == (getnameErrCode = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), addr,
                sizeof(addr), NULL, 0, NI_NUMERICHOST)))
        {
          ifaceIpAdress = addr;
          break;
        }
        else
        {
          LOG_ERROR("Error getting IP address for " << ifaceName << ": "
              << gai_strerror(getnameErrCode));
          close(fd);
          return -1;
        }
      }
    }
  }

  // Make sure iface ip address has to be found, which means iface name is valid
  if (!ifaceIpAdress.length())
  {
    close(fd);
    return -1;
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

void setDebugMode(bool enable)
{
  g_debugMode = enable;
}

bool isDebugMode()
{
  return g_debugMode;
}
