#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <vector>
#include <set>
#include <map>
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
#include <ifaddrs.h>

using std::string;
using std::cout;
using std::endl;
using std::vector;
using std::set;
using std::map;

#define MCAST_BUFF_LEN    (1024)  // message length

// print out error message to stderr
#define LOG_ERROR(msg) \
    do {if (!isDebugMode()) std::cerr << "[ERROR] " << msg << endl;\
        else std::cerr << __FILE__ << ":" << __LINE__ << " [ERROR] " << msg << endl;\
    } while(0)

/**
 * Struct that contains interface name and its associated socket
 */
struct IfaceData
{
  int sockFd;
  string ifaceName;
  string ifaceAddress;

  IfaceData(): sockFd(-1), ifaceName(""), ifaceAddress("") {}
  IfaceData(const string& name, const string& address, int fd = -1):
    sockFd(fd), ifaceName(name), ifaceAddress(address) {}

  const string& getReadableName() const;
  const string& getReadableAddress() const;
  const string& toString() const;

private:
  friend std::ostream & operator<<(std::ostream &os, const IfaceData& iface);
};

/**
 * Get ip address for ifaceName
 *
 * @param ifaceName - [IN] has to be nonzero length
 * @param ifaceIpAdress - [OUT]
 * @param isIpV6 - true if create ipv6 socket
 * @return 0 on success, -1 if error
 */
int getIfaceIPFromIfaceName(const string& ifaceName, string& ifaceIpAdress, bool isIpV6 = false);

/**
 * Get ifaceName from IP address
 *
 * @param ifaceIpAddress  - [IN] has to be nonzero length
 * @param ifaceName       - [OUT] resolved interface name
 * @param isIpV6          - true if create ipv6 socket
 * @return 0 on success, -1 if error
 */
int getIfaceNameFromIfaceAddress(const string& ifaceIpAddress, string& ifaceName, bool isIpV6 = false);

/**
 * Create udp socket
 *
 * @param isIpV6
 * @return socket fd, -1 if error, check errno for more info
 */
int createSocket(bool isIpV6 = false);

/**
 * Clear up internal data from Common.cpp
 */
void cleanupCommon();

/**
 * Getter/setter for debug mode
 */
void setDebugMode(bool enable = true);
bool isDebugMode();

/**
 * Determine if string is an IP address
 */
bool isValidIpAddress(const char *ipAddress, bool useIpV6 = false);

#endif /*COMMON_H_*/
