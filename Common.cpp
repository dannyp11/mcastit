#include "Common.h"

#define DEFAULT_IP_ADDRESS  "0.0.0.0"
#define DEFAULT_IFACE       "default"

/**
 * Maps contain all interface data
 * These maps should only be populated once
 */
static map<string, vector<string> > g_if4_name2ip_map;
static map<string, vector<string> > g_if6_name2ip_map;

static bool g_debugMode = false;

/**
 * Init g_ifap
 * @return 0 on success
 */
static int common_init()
{
  static bool alreadyRan = false;
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
    g_if4_name2ip_map.clear();
    g_if6_name2ip_map.clear();
    char ipAddr[INET6_ADDRSTRLEN];
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
        if (0 == getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), ipAddr,
                                                sizeof(ipAddr), NULL, 0, NI_NUMERICHOST))
        {
          g_if6_name2ip_map[ifa->ifa_name].push_back(ipAddr);
        }
      }
      // check ipv4 interface
      else if (ifa->ifa_addr->sa_family == AF_INET)
      {
        if (0 == getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), ipAddr,
                                                sizeof(ipAddr), NULL, 0, NI_NUMERICHOST))
        {
          g_if4_name2ip_map[ifa->ifa_name].push_back(ipAddr);
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

int getIfaceIPFromIfaceName(const string& ifaceName, vector<string>& ifaceIpAdresses, bool isIpV6)
{
  ifaceIpAdresses.clear();
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
      ifaceIpAdresses = g_if6_name2ip_map[ifaceName];
    }
  }
  else
  {
    if (g_if4_name2ip_map.end() != g_if4_name2ip_map.find(ifaceName))
    {
      ifaceIpAdresses = g_if4_name2ip_map[ifaceName];
    }
  }

  // Make sure iface ip address has to be found, which means iface name is valid
  if (!ifaceIpAdresses.size())
  {
    return -1;
  }

  return 0;
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

  if (ifaceAddresses.size() > 0)
  {
    result = "";
    for (unsigned i = 0; i < ifaceAddresses.size(); ++i)
    {
      if (i != ifaceAddresses.size() - 1)
      {
        result += ifaceAddresses[i] + ",";
      }
      else
      {
        result += ifaceAddresses[i];
      }
    }
  }
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

bool isDebugMode()
{
  return g_debugMode;
}
