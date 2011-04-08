#include <iostream>
#include "cmdlineoptions.h"

bool parseOptions(int argc, char * argv[], po::variables_map & options)
{
  po::options_description desc("LDX Minecraft Server");
  desc.add_options()
    ("help,h", "Print usage information")
    ("verbose,v", "Print lots of run-time information")
    ("bindaddr,a", po::value<std::string>()->default_value("0.0.0.0"), "Set IP address to bind to (default: all interfaces)")
    ("port,p", po::value<unsigned short int>()->default_value(25565), "Set port to listen on (default: 25565)")
    ("testfile,f", po::value<std::string>()->default_value(""), "Test a region file")
    ;

  try
  {
    po::store(po::parse_command_line(argc, argv, desc), options);
    po::notify(options);
  }
  catch (const std::exception & e)
  {
    std::cerr << "Error during command line parsing: " << e.what() << std::endl;
    return false;
  }
  catch (...)
  {
    std::cerr << "Unknown error during command line parsing." << std::endl;
    return false;
  }

  if (options.count("help"))
  {
    std::cout << desc << std::endl;
      return false;
  }

  return true;
}
