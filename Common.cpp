#include "Common.h"

#define DEFAULT_IP_ADDRESS  "0.0.0.0"
#define DEFAULT_IFACE       "default"

int createSocketFromIfaceName(const string& ifaceName, string& ifaceIpAdress)
{
  ifaceIpAdress = DEFAULT_IP_ADDRESS;

  if (!ifaceName.size())
  {
    return -1;
  }

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (-1 == fd)
  {
    perror("creating fd");
    return -1;
  }

  struct ifreq ifr;
  /* I want to get an IPv4 IP address */
  ifr.ifr_addr.sa_family = AF_INET;

  /* I want IP address attached to "ifaceName" */
  strncpy(ifr.ifr_name, ifaceName.c_str(), IFNAMSIZ - 1);
  if (0 != ioctl(fd, SIOCGIFADDR, &ifr))
  {
    close(fd);
    return -1;
  }

  /* display result */
  ifaceIpAdress = inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr);

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
  if (ifaceName.size()) result = ifaceName;
  return result;
}

const string& IfaceData::getReadableAddress() const
{
  static string result = DEFAULT_IP_ADDRESS;
  if (ifaceAddress.size()) result = ifaceAddress;
  return result;
}

std::ostream & operator<<(std::ostream &os, const IfaceData& iface)
{
  return os << iface.toString();
}
