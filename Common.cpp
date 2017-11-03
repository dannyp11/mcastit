#include "Common.h"

int createSocketFromIfaceName(const string& ifaceName, string& ifaceIpAdress)
{
  ifaceIpAdress = DEFAULT_IP_ADDRESS;

  if (!ifaceName.size() || ifaceName == DEFAULT_IFACE)
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

