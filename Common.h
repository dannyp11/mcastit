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
        else std::cerr << __FILE__ << ":" << __LINE__ << "-(" \
        <<__func__ << ") [ERROR] " << msg << endl;\
    } while(0)

#define LOG_DEBUG(msg) \
    do {if (isDebugMode()) std::cerr << __FILE__ << ":" \
        << __LINE__ << "-(" <<__func__ << ") [DEBUG] " << msg << endl;\
    } while(0)

// Set of all mcast addresses
typedef set<string> McastAddressSet;

/**
 * Struct that contains interface name and its associated socket
 */
struct IfaceData
{
  int sockFd;
  string ifaceName;
  vector<string> ifaceAddresses;

  IfaceData(): sockFd(-1), ifaceName(""), ifaceAddresses(vector<string>()) {}
  IfaceData(const string& name, const vector<string>& addresses, int fd = -1):
    sockFd(fd), ifaceName(name), ifaceAddresses(addresses) {}

  const string& getReadableName() const;
  const string& getReadableAddress() const;
  const string& toString() const;

private:
  friend std::ostream & operator<<(std::ostream &os, const IfaceData& iface);
};

/**
 * Get ip addresses for ifaceName
 *
 * @param ifaceName - [IN] has to be nonzero length
 * @param ifaceIpAdresses - [OUT] with primary address as the first entry
 * @param isIpV6 - true if create ipv6 socket
 * @return 0 on success, -1 if error
 */
int getIfaceIPFromIfaceName(const string& ifaceName, vector<string>& ifaceIpAdresses, bool isIpV6 = false);

/**
 * Create udp socket
 *
 * @param isIpV6
 * @return socket fd, -1 if error, check errno for more info
 */
int createSocket(bool isIpV6 = false);

/**
 * Enable reuse address and port on socket
 * @param sock   - input socket
 * @return 0 on success, <0 on error
 */
int setReuseSocket(int sock);

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
 * Encode/decode ack message
 * @param message     [IN]  message to be processed
 * @param resultMsg   [OUT] result message, = message if failed to de/encode
 * @return  true on success
 */
bool encodeAckMessage(const string& message, string& resultMsg);
bool decodeAckMessage(const string& message, string& resultMsg);

/**
 * Send unicast message to target
 * @param sock
 * @param target
 * @param msg
 * @return true if success
 */
bool unicastMessage(int sock, struct sockaddr_storage& target, const string& msg);

#endif /*COMMON_H_*/
