#include "Common.h"
#include "SenderModule.h"
#include "ReceiverModule.h"

// Global vars
#define DEFAULT_MCAST_ADDRESS_V4  "239.192.0.123"
#define DEFAULT_MCAST_ADDRESS_V6  "FFFE::1:FF47:0"

static int g_McastPort = 12321;
static string g_McastAddress = DEFAULT_MCAST_ADDRESS_V4;

static vector<IfaceData> g_ifaces;
static McastModuleInterface* g_McastModule = NULL;

static void usage(int /*argc*/, char * argv[])
{
  cout << "Usage: " << argv[0] << " [options] [iface1 iface2 ...]" << endl << endl

      << "   Multicast tool to test sending & receiving test message over various interfaces" << endl
      << "     if [iface1 iface2 ...] is blank, use system default one" << endl << endl

      << "    Option:" << endl
      << "    -6                 use IPv6" << endl
      << "    -m {mcast address} multicast address, default: " << DEFAULT_MCAST_ADDRESS_V4
                                                        << " and " << DEFAULT_MCAST_ADDRESS_V6 << "(v6)" << endl
      << "    -p {port}          multicast port, default: " << g_McastPort << endl << endl

      << "    -l                 listen mode" << endl
      << "    -o                 turn off loop back on sender" << endl
      << "    -h                 This message" << std::endl << endl;

  exit(1);
}

static void cleanup()
{
  delete g_McastModule;
  for (unsigned i = 0; i < g_ifaces.size(); ++i)
  {
    int& fd = g_ifaces[i].sockFd;
    if (fd >= 0 && (0 != close(fd)))
    {
      LOG_ERROR("Close socket " << fd << ": " << strerror(errno));
    }
    else
    {
      fd = -1;
    }
  }
  cleanupCommon();
}

static void safeExit(int errCode = 0)
{
  cleanup();
  exit(errCode);
}

static void sigHandler(int signo)
{
  cout << " Caught signal " << signo << endl;
  safeExit(0);
}

static void errorHandler(int signo)
{
  cout << " Caught error signal " << signo << endl;
  void *array[10];
  size_t size = backtrace(array, 10);

  // print out all the frames to stderr
  backtrace_symbols_fd(array, size, STDERR_FILENO);

  safeExit(signo);
}

int main(int argc, char** argv)
{
  bool listenMode = false;
  bool loopBackOn = true;
  bool useIPv6 = false;
  bool useDefaultIp = true;

  g_ifaces.clear();

  int command = -1;
  while ((command = getopt(argc, argv, "D6lom:p:h")) != -1)
  {
    switch (command)
    {
    case '6':
      useIPv6 = true;
      break;
    case 'o':
      loopBackOn = false;
      break;
    case 'm':
      g_McastAddress = optarg;
      useDefaultIp = false;
      break;
    case 'p':
      g_McastPort = atoi(optarg);
      break;
    case 'l':
      listenMode = true;
      break;
    case 'D':
      cout << "Debug mode ON" << endl;
      setDebugMode(true);
      break;
    case 'h':
    default:
      usage(argc, argv);
      break;
    }
  }

  /*
   * Update arguments based on getopt
   */
  if (useIPv6 && useDefaultIp)
  {
    g_McastAddress = DEFAULT_MCAST_ADDRESS_V6;
  }

  /*
   * Register sig handler
   */
  signal(SIGINT, sigHandler);
  signal(SIGHUP, sigHandler);
  signal(SIGQUIT, sigHandler);
  signal(SIGSEGV, errorHandler);
  signal(SIGABRT, errorHandler);
  signal(SIGILL, errorHandler);
  signal(SIGFPE, errorHandler);
  signal(SIGPIPE, errorHandler);

  /*
   * Now get iface info
   */
  int fd = -1;
  for (int i = optind; i < argc; ++i)
  {
    const string iface = argv[i];
    string ifaceAddress, ifaceName;
    bool useIPAddress = false;
    if (!listenMode || i == optind)
    {
      fd = createSocket(useIPv6);
    }

    if (-1 == fd)
    {
      LOG_ERROR("Can't create socket for " << iface);
      safeExit(1);
    }

    // force using IP address
    if (isValidIpAddress(iface.c_str(), useIPv6))
    {
      ifaceAddress = iface;
      useIPAddress = true;
      if (0 != getIfaceNameFromIfaceAddress(ifaceAddress, ifaceName, useIPv6))
      {
        LOG_ERROR("Can't find interface name for " << ifaceAddress);
        safeExit(2);
      }
    }
    else
    {
      // use interface name
      ifaceName = iface;
      useIPAddress = false;
      if (0 != getIfaceIPFromIfaceName(ifaceName, ifaceAddress, useIPv6))
      {
        LOG_ERROR("Can't find interface IP address for " << ifaceName);
        safeExit(2);
      }
    }

    g_ifaces.push_back((IfaceData(ifaceName, ifaceAddress, fd, useIPAddress)));
  }

  if (0 == g_ifaces.size())
  {
    int sock = createSocket(useIPv6);
    if (sock < 0)
    {
      LOG_ERROR("Creating socket: " << strerror(errno));
      safeExit(1);
    }

    g_ifaces.push_back(IfaceData("", "", sock));
  }

  /*
   * Now we can run the mode
   */
  if (listenMode)
  {
    g_McastModule = new ReceiverModule(g_ifaces, g_McastAddress, g_McastPort, useIPv6);
  }
  else
  {
    g_McastModule = new SenderModule(g_ifaces, g_McastAddress, g_McastPort, loopBackOn, useIPv6);
  }

  if (g_McastModule)
  {
    if (!g_McastModule->run())
    {
      cout << "Error running module, exiting..." << endl;
    }
  }
  else
  {
    cout << "Error spawning mcast module" << endl;
  }

  cleanup();
  return 0;
}
