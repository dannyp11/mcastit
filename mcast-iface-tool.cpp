#include "Common.h"
#include "SenderModule.h"
#include "ReceiverModule.h"

// Global vars
#define DEFAULT_MCAST_ADDRESS_V4  "239.192.0.123"
#define DEFAULT_MCAST_ADDRESS_V6  "FFFE::1:FF47:0"
#define DEFAULT_MCAST_PORT        (12321)

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
                                                        << " or " << DEFAULT_MCAST_ADDRESS_V6 << endl
      << "    -p {port}          multicast port, default: " << DEFAULT_MCAST_PORT << endl << endl

      << "    -i {interval}      interval in seconds if send in loop" << endl
      << "    -l                 listen mode" << endl
      << "    -o                 turn off loop back on sender" << endl
      << "    -h                 This message, (version " __DATE__ << " " << __TIME__ << ")" << endl << endl;

  exit(1);
}

static void cleanup()
{
  delete g_McastModule;
  set<int> uniqueFdSet;
  for (unsigned i = 0; i < g_ifaces.size(); ++i)
  {
    uniqueFdSet.insert(g_ifaces[i].sockFd);
  }
  
  for (set<int>::const_iterator setIt = uniqueFdSet.begin(); 
            setIt != uniqueFdSet.end(); ++setIt)
  {
    if (-1 == close(*setIt))
    {
      LOG_ERROR("Close socket " << *setIt << ": " << strerror(errno));
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
  string mcastAddress = DEFAULT_MCAST_ADDRESS_V4;
  int mcastPort = DEFAULT_MCAST_PORT;
  int exitVal = 0;
  float sendInterval = -1;

  g_ifaces.clear();

  int command = -1;
  while ((command = getopt(argc, argv, "D6lom:p:i:h")) != -1)
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
      mcastAddress = optarg;
      useDefaultIp = false;
      break;
    case 'p':
      mcastPort = atoi(optarg);
      break;
    case 'l':
      listenMode = true;
      break;
    case 'i':
      sendInterval = atof(optarg);
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
    mcastAddress = DEFAULT_MCAST_ADDRESS_V6;
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
    const string ifaceName = argv[i];
    vector<string> ifaceAddresses;
    if (!listenMode || i == optind)
    {
      fd = createSocket(useIPv6);
    }

    if (-1 == fd)
    {
      LOG_ERROR("Can't create socket for " << ifaceName);
      safeExit(1);
    }

    // use interface name
    if (0 != getIfaceIPFromIfaceName(ifaceName, ifaceAddresses, useIPv6))
    {
      LOG_ERROR("Can't find interface IP address for " << ifaceName);
      safeExit(2);
    }

    g_ifaces.push_back((IfaceData(ifaceName, ifaceAddresses, fd)));
  }

  if (0 == g_ifaces.size())
  {
    int sock = createSocket(useIPv6);
    if (sock < 0)
    {
      LOG_ERROR("Creating socket: " << strerror(errno));
      safeExit(1);
    }

    g_ifaces.push_back(IfaceData("", vector<string>(), sock));
  }

  /*
   * Now we can run the mode
   */
  if (listenMode)
  {
    g_McastModule = new ReceiverModule(g_ifaces, mcastAddress, mcastPort, useIPv6);
  }
  else
  {
    g_McastModule = new SenderModule(g_ifaces, mcastAddress, mcastPort,
                                                  loopBackOn, useIPv6, sendInterval);
  }

  if (g_McastModule)
  {
    if (!g_McastModule->run())
    {
      cout << "Error running module, exiting..." << endl;
      exitVal = 1;
    }
  }
  else
  {
    cout << "Error spawning mcast module" << endl;
    exitVal = 2;
  }

  cleanup();
  return exitVal;
}
