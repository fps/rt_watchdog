#include <boost/program_options.hpp>

#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include <cstdlib>

#include <iostream>

uint32_t sentinel;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

timespec waker_period_timespec{0, 0};
uint32_t waker_priority;

void *waker(void *)
{
  if (0 != waker_priority)
  {
    sched_param param{(int)waker_priority};
    if (0 != pthread_setschedparam(pthread_self(), SCHED_FIFO, &param))
    {
      std::cout << "Failed to set waker priority. Exiting.\n";
      exit(1);
    }
  }

  while(true)
  {
    __atomic_add_fetch(&sentinel, 1, __ATOMIC_SEQ_CST);
    if (-1 == nanosleep(&waker_period_timespec, 0))
    {
      std::cout << "Waker failed to sleep: " << strerror(errno) << ". Exiting.\n"; 
      exit(1);
    }  
  }

  return 0;
}

int main(int argc, char *argv[])
{
  std::string command;
  uint32_t waker_period;
  uint32_t waiter_period;
  uint32_t waiter_priority;

  try
  {
    namespace po = boost::program_options;
    po::options_description options_description("Allowed options");
    options_description.add_options()
      ("help,h", "Show this help text")
      ("command", po::value<std::string>(&command)->default_value("bash -c $'echo rt_watchdog timed out. Changing thread scheduling classes to SCHED_OTHER | wall; for n in $(ps -eL -o pid=,rtprio= | grep -v - | awk \\'$2 >= 55\\' | awk \\'$2 <= 85\\' | awk \\'{print $1}\\'); do chrt -o -p 0 $n; done'"), "The command to run in case of a timeout")
      ("waker-period", po::value<uint32_t>(&waker_period)->default_value(1), "The waker period (seconds)")
      ("waiter-period", po::value<uint32_t>(&waiter_period)->default_value(5), "The waiter timeout (seconds)")
      ("waker-priority", po::value<uint32_t>(&waker_priority)->default_value(0), "The waker priority (SCHED_FIFO). If set to 0 SCHED_OTHER is used")
      ("waiter-priority", po::value<uint32_t>(&waiter_priority)->default_value(95), "The waiter priority (SCHED_FIFO)")
    ;
  
    po::variables_map variables_map;
    po::store(po::parse_command_line(argc, argv, options_description), variables_map);
  
    if (variables_map.count("help"))
    {
      std::cout << options_description << "\n";
      exit(0);
    }
    
    po::notify(variables_map);
  }
  catch(const std::exception &e)
  {
    std::cout << "Error: " << e.what() << "\n";
    exit(1);
  }
  
  timespec waiter_period_timespec{0, 0};

  waker_period_timespec.tv_sec = waker_period;
  waiter_period_timespec.tv_sec = waiter_period;

  __atomic_store_n(&sentinel, 0, __ATOMIC_SEQ_CST);


  pthread_t waker_thread;
  if (0 != pthread_create(&waker_thread, 0, waker, 0))
  {
    std::cout << "Failed to create waker thread. Exiting.\n";
    exit(1);
  }

  sched_param param{(int)waiter_priority};
  if (0 != pthread_setschedparam(pthread_self(), SCHED_FIFO, &param))
  {
    std::cout << "Failed to set waker priority. Exiting.\n";
    exit(1);
  }

  while(true)
  {
    __atomic_store_n(&sentinel, 0, __ATOMIC_SEQ_CST);
    if (-1 == nanosleep(&waiter_period_timespec, 0))
    {
      std::cout << "Waiter failed to sleep: " << strerror(errno) << "\n";
      exit(1);
    }
    
    uint32_t value = __atomic_load_n(&sentinel, __ATOMIC_SEQ_CST);
    if (value == 0)
    {
      /*
      timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      std::cout << ts.tv_sec << " " << ts.tv_nsec << "\n";
      */
      std::cout << "Waker seems to not have run in a while. Executing command: \"" << command << "\"\n";
      if(-1 == system(command.c_str()))
      {
        std::cout << "Failed to run command: " << strerror(errno) << ". Exiting.\n";
        exit(1);
      }
      /*
      clock_gettime(CLOCK_MONOTONIC, &ts);
      std::cout << ts.tv_sec << " " << ts.tv_nsec << "\n";
      */
    }
  }

  exit(0);
}
