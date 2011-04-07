#ifndef H_SERVER
#define H_SERVER


#include <string>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include "connection.h"
#include "gamestatemanager.h"
#include "inputparser.h"
#include "map.h"


class Server : private boost::noncopyable
{
public:
  explicit Server(const std::string & address, unsigned short int port);
  ~Server() { }

  /// Run the server's io_service loop.
  void runIO();

  /// Run the server's main game loop.
  void runGame();

  /// Stop the server.
  void stop();

  /// Processors.
  void process_ingress(int32_t eid,  std::deque<char> & d, std::shared_ptr<std::recursive_mutex> ptr_mutex);
  void process_egress(int32_t eid);
  void process_gamestate(int32_t eid);

  /// Accessors.
  inline const ConnectionManager & cm() const { return m_connection_manager; }
  inline boost::asio::io_service & ioService() { return m_io_service; }

  /// A millisecond clock tick.
  typedef std::chrono::high_resolution_clock::period clock_period;
  static inline long long int clockTick()
  {
    return (long long int)((std::chrono::high_resolution_clock::now().time_since_epoch().count() * clock_period::num * 1000) / clock_period::den);
  }

private:
  /// Handle completion of an asynchronous accept operation.
  void handleAccept(const boost::system::error_code & error);

  /// Handle a request to stop the server.
  void handleStop();

  /// The io_service used to perform asynchronous operations.
  boost::asio::io_service m_io_service;

  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor m_acceptor;

  /// Notify the main loop.
  bool m_server_should_stop;

  /// The connection manager which owns all live connections.
  ConnectionManager m_connection_manager;

  /// The next connection to be accepted.
  ConnectionPtr m_next_connection;

  /// The game state manager.
  GameStateManager m_gsm;

  /// The input parser.
  InputParser m_input_parser;

  /// The map. (Will eventually have many.)
  Map m_map;
};


#endif
