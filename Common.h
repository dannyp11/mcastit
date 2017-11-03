#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <vector>
#include <iostream>
#include <signal.h>
#include <execinfo.h>

#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <net/if.h>
#include <netdb.h>
#include <fcntl.h>
#include <strings.h>
#include <sstream>

using std::string;
using std::cout;
using std::endl;
using std::vector;

#define DEFAULT_IFACE   "default"
#define DEFAULT_IP_ADDRESS "0.0.0.0"

struct IfaceData
{
  int sockFd;
  string ifaceName;
  string ifaceAddress;

  IfaceData(const string& name, const string& address, int fd = -1):
    sockFd(fd), ifaceName(name), ifaceAddress(address) {}
};

/**
 *
 * @param ifaceName
 * @param ifaceIpAdress
 * @return the socket created by getting the info, -1 if error
 */
int createSocketFromIfaceName(const string& ifaceName, string& ifaceIpAdress);

#endif /*COMMON_H_*/
