#include "Common.h"

#define DEFAULT_IP_ADDRESS  "0.0.0.0"
#define DEFAULT_IFACE       "default"

static struct ifaddrs *g_ifap;
static bool g_debugMode = false;

/**
 * Init g_ifap
 * @return 0 on success
 */
static int init_Ifap()
{
  bool alreadyRan = false;
  static int retVal = -1;
  if (!alreadyRan)
  {
    alreadyRan = true;
    if (0 != (retVal = getifaddrs(&g_ifap)))
    {
      LOG_ERROR("Can't get system interfaces: " << strerror(errno));
    }
  }

  return retVal;
}

int createSocket(bool isIpV6)
{
  int sockFamily = (isIpV6) ? AF_INET6 : AF_INET;
  return socket(sockFamily, SOCK_DGRAM, 0);
}

int getIfaceIPFromIfaceName(const string& ifaceName, string& ifaceIpAdress, bool isIpV6)
{
  ifaceIpAdress = "";
  if (!ifaceName.size())
  {
    return -1;
  }

  /*
   * Now get interface ip address from interface name
   */
  if (0 != init_Ifap())
  {
    return -1;
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
          return -1;
        }
      }
    }
  }

  // Make sure iface ip address has to be found, which means iface name is valid
  if (!ifaceIpAdress.length())
  {
    return -1;
  }

  return 0;
}

const string& IfaceData::toString() const
{
  static string result;
  std::stringstream stm;

  if (mustUseIPAddress)
  {
    stm << getReadableAddress() << " (" << getReadableName() << ")";
  }
  else
  {
    stm << getReadableName() << " (" << getReadableAddress() << ")";
  }
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

int getIfaceNameFromIfaceAddress(const string& ifaceIpAddress, string& ifaceName, bool isIpV6)
{
  ifaceName = "";
  if (!ifaceIpAddress.length())
  {
    return -1;
  }

  /*
   * Now get interface ip address from interface name
   */
  if (0 != init_Ifap())
  {
    return -1;
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
      if (0 == (getnameErrCode = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6),
                                           addr, sizeof(addr), NULL, 0, NI_NUMERICHOST)))
      {
        if (ifaceIpAddress == addr)
        {
          ifaceName = ifa->ifa_name;
          break;
        }
      }
      else
      {
        LOG_ERROR( "Error getting interface name for "
                      << ifaceIpAddress << ": " << gai_strerror(getnameErrCode));
        return -1;
      }
    }
    // check ipv4 interface
    else if (!isIpV6 && ifa->ifa_addr->sa_family == AF_INET)
    {
      if (0 == (getnameErrCode = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                                            addr, sizeof(addr), NULL, 0, NI_NUMERICHOST)))
      {
        if (ifaceIpAddress == addr)
        {
          ifaceName = ifa->ifa_name;
          break;
        }
      }
      else
      {
        LOG_ERROR( "Error getting interface name for "
                      << ifaceIpAddress << ": " << gai_strerror(getnameErrCode));
        return -1;
      }
    }
  }

  // Make sure iface ip address has to be found, which means iface name is valid
  if (!ifaceName.length())
  {
    return -1;
  }

  return 0;
}

bool isDebugMode()
{
  return g_debugMode;
}

bool isValidIpAddress(const char *ipAddress, bool useIpV6)
{
  int result = 0;

  if (!useIpV6)
  {
    struct sockaddr_in sa;
    result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
  }
  else
  {
    struct sockaddr_in6 sa6;
    result = inet_pton(AF_INET6, ipAddress, &(sa6.sin6_addr));
  }

  return result != 0;
}
