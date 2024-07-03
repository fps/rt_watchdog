#include <boost/program_options.hpp>

#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include <cstdlib>

#include <iostream>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

timespec waker_timeout{5, 0};

void *waker(void *)
{
  while(true)
  {
    if (-1 == nanosleep(&waker_timeout, 0))
    {
      std::cout << "Failed to sleep: " << strerror(errno) << ". Exiting.\n";
      exit(1);
    }

    std::cout << "Waking.\n";

    if (0 != pthread_cond_signal(&cond))
    {
      std::cout << "Failed to signal condition variable. Exiting.\n";
      exit(1);
    }
  }

  return 0;
}

int main(int argc, char *argv[])
{
  std::string command;
  uint32_t waker_period;
  uint32_t waiter_timeout;

  namespace po = boost::program_options;
  po::options_description options_description("Allowed options");
  options_description.add_options()
    ("help", "Show this help text")
    ("command", po::value<std::string>(&command)->default_value("bash -c \"echo hello\""), "The command to run in case of a timeout")
    ("waker-period", po::value<uint32_t>(&waker_period)->default_value(1), "The waker period (seconds)")
    ("waiter-timeout", po::value<uint32_t>(&waiter_timeout)->default_value(5), "The waiter timeout (seconds)")
  ;

  po::variables_map variables_map;
  po::store(po::parse_command_line(argc, argv, options_description), variables_map);

  if (variables_map.count("help"))
  {
    std::cout << options_description << "\n";
    exit(0);
  }
  
  po::notify(variables_map);

  waker_timeout.tv_sec = waker_period;

  pthread_t waker_thread;
  if (0 != pthread_create(&waker_thread, 0, waker, 0))
  {
    std::cout << "Failed to create waker thread. Exiting.\n";
    exit(1);
  }

  bool first_wakeup = true;
  timespec last_wakeup;
  while(true)
  {
    if (0 != pthread_mutex_lock(&mutex))
    {
      std::cout << "Failed to lock mutex. Exiting.\n";
      exit(1);
    }

    timeval current_time;
    if (0 != gettimeofday(&current_time, 0))
    {
      std::cout << "Failed to get current time. Exiting.\n";
      exit(1);
    }

    timespec timeout{current_time.tv_sec + waiter_timeout, current_time.tv_usec * 1000};
    int wait_res = pthread_cond_timedwait(&cond, &mutex, &timeout);
    switch(wait_res)
    {
      case 0:
        std::cout << "Woken.\n";
        break;
      case ETIMEDOUT:
        std::cout << "Timeout.\n";
        if(-1 == system(command.c_str()))
        {
          std::cout << "Failed to run command: " << strerror(errno) << ". Exiting.\n";
          exit(1);
        }
        break;
      default:
        std::cout << "Failed to wait on condition variable. Exiting.\n";
        exit(1);
    }

    if (0 != pthread_mutex_unlock(&mutex))
    {
      std::cout << "Failed to unlock mutex. Exiting.\n";
      exit(1);
    }
  }

  exit(0);
}
