#include "Common.h"
#include "SenderModule.h"
#include "ReceiverModule.h"

// Global vars
static string g_McastAddress = "239.192.0.9";
static int g_McastPort = 50428;

static vector<IfaceData> g_ifaces;
static McastModuleInterface* g_McastModule = NULL;

static void usage(int /*argc*/, char * argv[])
{
  cout << "Usage: " << argv[0] << " [options] [iface1 iface2 ...]" << endl << endl

      << "   Multicast tool to test sending & receiving test message over various interfaces" << endl
      << "     if [iface1 iface2 ...] is blank, use system default one" << endl << endl

      << "    Option:" << endl << "    -o                 turn off loop back on sender" << endl
      << "    -l                 listen mode" << endl
      << "    -m {mcast address} multicast address, default: " << g_McastAddress << endl
      << "    -p {port}          multicast port, default: " << g_McastPort << endl
      << "    -h                 This message" << std::endl << endl;

  exit(1);
}

static void cleanup()
{
  delete g_McastModule;
  for (unsigned i = 0; i < g_ifaces.size(); ++i)
  {
    int fd = g_ifaces[i].sockFd;
    if (fd >=0 && (0 != close(fd)))
    {
      cout << "Error closing fd " << fd << ": " << strerror(errno) << endl;
    }
  }
}

static void sigHandler(int signo)
{
  cout << "Caught signal " << signo << endl;
  cleanup();
  exit(0);
}

static void errorHandler(int signo)
{
  cout << "Caught error signal " << signo << endl;
  void *array[10];
  size_t size = backtrace(array, 10);

  // print out all the frames to stderr
  backtrace_symbols_fd(array, size, STDERR_FILENO);

  cleanup();
  exit(1);
}

int main(int argc, char** argv)
{
  bool listenMode = false;
  bool loopBackOn = true;
  g_ifaces.clear();

  int command = -1;
  while ((command = getopt(argc, argv, "lom:p:h")) != -1)
  {
    switch (command)
    {
    case 'o':
      loopBackOn = false;
      break;
    case 'm':
      g_McastAddress = optarg;
      break;
    case 'p':
      g_McastPort = atoi(optarg);
      break;
    case 'l':
      listenMode = true;
      break;
    case 'h':
    default:
      usage(argc, argv);
      break;
    }
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
  for (int i = optind; i < argc; ++i)
  {
    string ifaceName = argv[i];
    string ifaceAddress;
    int fd = createSocketFromIfaceName(ifaceName, ifaceAddress);
    if (-1 == fd)
    {
      cout << "Can't find ip address for " << ifaceName << endl;
      exit(1);
    }

    g_ifaces.push_back((IfaceData(ifaceName, ifaceAddress, fd)));
  }

  if (0 == g_ifaces.size())
  {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
      perror("socket");
      exit(1);
    }

    g_ifaces.push_back(IfaceData("", "", sock));
  }

  /*
   * Now we can run the mode
   */
  if (listenMode)
  {
    g_McastModule = new ReceiverModule(g_ifaces, g_McastAddress, g_McastPort);
  }
  else
  {
    g_McastModule = new SenderModule(g_ifaces, g_McastAddress, g_McastPort, loopBackOn);
  }

  if (g_McastModule)
  {
    g_McastModule->run();
  }
  else
  {
    cout << "Error spawning mcast module" << endl;
  }

  cleanup();
  return 0;
}
