#ifndef H_SERVER
#define H_SERVER


#include <string>
#include <thread>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include "connection.h"
#include "gamestatemanager.h"
#include "inputparser.h"
#include "map.h"


class UI;

class Server : private boost::noncopyable
{
  friend bool pump(Server &, UI &);

public:
  explicit Server(const std::string & address, unsigned short int port);
  ~Server() { }

  /// Run the server's io_service loop.
  void runIO();

  /// Run the server's input processing loop.
  void runInputProcessing();

  /// Run the server's timer processing loop.
  void runTimerProcessing();

  /// Stop the server.
  void stop();

  /// Processors.
  bool processIngress(int32_t eid, std::shared_ptr<SyncQueue> d);
  void processSchedule200ms(int actual_time_interval);
  void processSchedule1s();
  void processSchedule10s();

  /// Accessors.
  inline const ConnectionManager & cm() const { return m_connection_manager; }
  inline       ConnectionManager & cm()       { return m_connection_manager; }
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

  /// Synchronisation.
  bool m_server_should_stop;

  /// The connection manager which owns all live connections.
  ConnectionManager m_connection_manager;

  /// The next connection to be accepted.
  ConnectionPtr m_next_connection;

  /// The map. (Will eventually have many.)
  Map m_map;

  /// The game state manager.
  GameStateManager m_gsm;

  /// The input parser.
  InputParser m_input_parser;

  /// An alarm clock.
  boost::asio::deadline_timer m_deadline_timer;
  unsigned int m_sleep_next;
  inline void sleepMilli(unsigned int t)
  {
    m_deadline_timer.expires_from_now(boost::posix_time::milliseconds(t));
    m_deadline_timer.wait();
  }

};


#endif
