#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <boost/asio.hpp>

#include "server.h"
#include "cmdlineoptions.h"
#include "ui.h"

int main(int argc, char* argv[])
{
  po::variables_map options;
  if (!parseOptions(argc, argv, options)) return 0;

  const bool verbose = options.count("verbose");

  if (verbose)
  {
    std::cout << "Server options:" << std::endl
              << "   Bind address: " << options["bindaddr"].as<std::string>() << std::endl
              << "   Port:         " << options["port"].as<unsigned short int>() << std::endl
              << std::endl;
  }

  try
  {
    // Run server in background thread.
    Server server(options["bindaddr"].as<std::string>(), options["port"].as<unsigned short int>());
    std::thread thread_io(std::bind(&Server::runIO, &server));
    std::thread thread_input(std::bind(&Server::runInputProcessing, &server));
    std::thread thread_timer(std::bind(&Server::runTimerProcessing, &server));

    //GNUReadlineUI ui("/tmp/.minerd_history");
    SimpleUI      ui;

    while (pump(server, ui)) { }

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
