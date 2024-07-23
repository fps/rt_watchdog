#include <boost/program_options.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
  uint64_t burn_cycles;

  try
  {
    namespace po = boost::program_options;
    po::options_description options_description("Allowed options");
    options_description.add_options()
      ("help,h", "Show this help text")
      ("burn-cycles", po::value<uint64_t>(&burn_cycles)->default_value(10000), "A number representing some work. The bigger the number the longer this process will run wasting energy and cpu cycles")
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

  volatile long long n;
  for (uint64_t index = 0; index < burn_cycles; ++index)
  {
    ++n;
  }

  exit(0);
}

