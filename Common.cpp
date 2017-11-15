#include "Common.h"

#define DEFAULT_IP_ADDRESS  "0.0.0.0"
#define DEFAULT_IFACE       "default"

/**
 * Maps contain all interface data
 * These maps should only be populated once
 */
static map<string, string> g_if4_name2ip_map;
static map<string, string> g_if4_ip2name_map;
static map<string, string> g_if6_name2ip_map;
static map<string, string> g_if6_ip2name_map;

static bool g_debugMode = false;

/**
 * Init g_ifap
 * @return 0 on success
 */
static int common_init()
{
  bool alreadyRan = false;
  static int retVal = -1;
  if (!alreadyRan)
  {
    alreadyRan = true;
    struct ifaddrs *ifap;
    if (0 != (retVal = getifaddrs(&ifap)))
    {
      LOG_ERROR("Can't get system interfaces: " << strerror(errno));
    }

    // populate all the maps
    char ipAddr[INET6_ADDRSTRLEN];
    int getnameErrCode;
    for (struct ifaddrs* ifa = ifap; ifa; ifa = ifa->ifa_next)
    {
      // make sure ifa_addr is valid
      if (!ifa->ifa_addr)
      {
        continue;
      }

      // Check ipv6 interface
      if (ifa->ifa_addr->sa_family == AF_INET6)
      {
        if (0 == (getnameErrCode = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), ipAddr,
                                                sizeof(ipAddr), NULL, 0, NI_NUMERICHOST)))
        {
          g_if6_ip2name_map[ipAddr] = ifa->ifa_name;
          g_if6_name2ip_map[ifa->ifa_name] = ipAddr;
        }
      }
      // check ipv4 interface
      else if (ifa->ifa_addr->sa_family == AF_INET)
      {
        if (0 == (getnameErrCode = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), ipAddr,
                                                sizeof(ipAddr), NULL, 0, NI_NUMERICHOST)))
        {
          g_if4_ip2name_map[ipAddr] = ifa->ifa_name;
          g_if4_name2ip_map[ifa->ifa_name] = ipAddr;
        }
      }
    }

    freeifaddrs(ifap);
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
  if (0 != common_init())
  {
    return -1;
  }

  // retrieve ip address from global maps
  if (isIpV6)
  {
    if (g_if6_name2ip_map.end() != g_if6_name2ip_map.find(ifaceName))
    {
      ifaceIpAdress = g_if6_name2ip_map[ifaceName];
    }
  }
  else
  {
    if (g_if4_name2ip_map.end() != g_if4_name2ip_map.find(ifaceName))
    {
      ifaceIpAdress = g_if4_name2ip_map[ifaceName];
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
  if (0 != common_init())
  {
    return -1;
  }

  // retrieve iface name from global maps
  if (isIpV6)
  {
    if (g_if6_ip2name_map.end() != g_if6_ip2name_map.find(ifaceIpAddress))
    {
      ifaceName = g_if6_ip2name_map[ifaceIpAddress];
    }
  }
  else
  {
    if (g_if4_ip2name_map.end() != g_if4_ip2name_map.find(ifaceIpAddress))
    {
      ifaceName = g_if4_ip2name_map[ifaceIpAddress];
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
