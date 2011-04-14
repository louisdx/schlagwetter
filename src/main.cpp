#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include "server.h"
#include "cmdlineoptions.h"
#include "ui.h"
#include "filereader.h"

po::variables_map PROGRAM_OPTIONS;
namespace fs = boost::filesystem;


#define USE_SIGNALS 0
#if USE_SIGNALS > 0
#include <signal.h>
bool sig_flag = true;
void sigINTHandler(int)
{
  std::cout << "You pressed Ctrl-C. Please press Return to flush the input loop. "
    "We will try to shut down the server peacefully, please be patient." << std::endl;
  sig_flag = false;
}
void sigTERMHandler(int)
{
  std::cout << "SIGTERM received; we will try to shut down the server peacefully, please be patient." << std::endl;
  sig_flag = false;
}
#endif

int main(int argc, char* argv[])
{
  if (!parseOptions(argc, argv, PROGRAM_OPTIONS)) return 0;

  if (PROGRAM_OPTIONS.count("verbose"))
  {
    std::cout << "Server options:" << std::endl
              << "   Bind address: " << PROGRAM_OPTIONS["bindaddr"].as<std::string>() << std::endl
              << "   Port:         " << PROGRAM_OPTIONS["port"].as<unsigned short int>() << std::endl
              << std::endl;
  }

  const std::string filename = PROGRAM_OPTIONS["load"].as<std::string>();
  if (!filename.empty() && !(fs::exists(filename + ".idx") && fs::exists(filename + ".idx")))
  {
    std::cout << "The specified input files (.idx and .dat) do not exist." << std::endl;
    return 0;
  }

#if USE_SIGNALS > 0
  signal(SIGINT, sigINTHandler);
  signal(SIGTERM, sigTERMHandler);
#endif

  try
  {
    // Run server in background thread.
    Server server(PROGRAM_OPTIONS["bindaddr"].as<std::string>(), PROGRAM_OPTIONS["port"].as<unsigned short int>());
    std::thread thread_io(std::bind(&Server::runIO, &server));
    std::thread thread_input(std::bind(&Server::runInputProcessing, &server));
    std::thread thread_timer(std::bind(&Server::runTimerProcessing, &server));

#ifdef HAVE_GNUREADLINE
    // We must fix the path somehow.
    GNUReadlineUI ui("/tmp/.schlagwetter_history");
#else
    SimpleUI      ui;
#endif

    while (pump(server, ui)
#if USE_SIGNALS > 0
     && sig_flag
#endif
          ) { }

    server.stop();
    thread_io.join();
    thread_input.join();
    thread_timer.join();
  }
  catch (std::exception & e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  return 0;
}
