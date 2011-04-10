#include <functional>
#include <thread>
#include <iostream>
#include <iomanip>
#include "server.h"
#include "inputparser.h"
#include "constants.h"


template<class T>
struct Identity
{
  inline T operator()() { return _x; }
  Identity(T x) : _x(x) { }
private:
  T _x;
};

Server::Server(const std::string & bindaddr, unsigned short int port)
  :
  m_io_service(),
  m_acceptor(m_io_service),
  m_server_should_stop(false),
  m_connection_manager(),
  m_next_connection(new Connection(m_io_service, m_connection_manager)),
  m_map(),
  m_gsm(m_connection_manager, m_map),
  m_input_parser(m_gsm),
  m_deadline_timer(m_io_service),
  m_sleep_next(200)
{
  boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(bindaddr), port);

  m_acceptor.open(endpoint.protocol());
  m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  m_acceptor.bind(endpoint);
  m_acceptor.listen();
  m_acceptor.async_accept(m_next_connection->socket(), m_next_connection->peer(), std::bind(&Server::handleAccept, this, std::placeholders::_1));
}

void Server::runIO()
{
  m_io_service.run();
}

void Server::runInputProcessing()
{
  while (!m_server_should_stop)
  {
    sleepMilli(100);
    std::unique_lock<std::mutex> lock(m_connection_manager.m_input_ready_mutex);
    while (!m_connection_manager.m_input_ready)
    {
      m_connection_manager.m_input_ready_cond.wait(lock, Identity<const bool &>(m_connection_manager.m_input_ready));
    }

    while (!m_connection_manager.m_pending_eids.empty())
    {
      std::unique_lock<std::recursive_mutex> lock(m_connection_manager.m_cd_mutex);

      ConnectionManager::ClientData::iterator it = m_connection_manager.clientData().find(m_connection_manager.m_pending_eids.front());

      if (it !=  m_connection_manager.clientData().end())
      {
        processIngress(it);
      }

      m_connection_manager.m_pending_eids.pop_front();
    }
  }
}

void Server::runTimerProcessing()
{
  long long int timer = clockTick();

  unsigned int counter = 0;

  for ( ; !m_server_should_stop; ++counter)
  {
    sleepMilli(m_sleep_next);

    const long long int now = clockTick();
    const long long int dt = now - timer;
    timer = now;

    /* If the 200ms processor runs too long for the 1s and 10s tasks,
       we can make another thread; or we can make the 1s and 10s tasks
       launch worker threads if need be. */

    processSchedule200ms(dt);

    if (counter % 5 == 0) processSchedule1s();

    if (counter % 50 == 0) { counter = 0; processSchedule10s(); }
  }
}

void Server::processSchedule200ms(int dt)
{
  static long long int timer;

  //std::cout << "Tick-200ms: Actual time was " << std::dec << dt << "ms." << std::endl;

  if (dt < 200) sleepMilli(200 - dt);
  timer = clockTick();

  /* do stuff */

  const long long int work_time = clockTick() - timer;
  if (work_time < 0) m_sleep_next = 0;
  else m_sleep_next -= work_time;
}

void Server::processSchedule1s()
{
  static long long int timer = clockTick();

  long long int now = clockTick();

  //std::cout << "Tick-1s. Actual time since last call is " << std::dec << now - timer << "ms." << std::endl;

  /* do stuff */


  // We just gather the active eids quickly and don't hang on to the mutex...
  std::list<int32_t> todo;
  {
    std::unique_lock<std::recursive_mutex> lock(m_connection_manager.m_cd_mutex);
    for (ConnectionManager::ClientData::const_iterator it = m_connection_manager.clientData().begin(); it != m_connection_manager.clientData().end(); ++it)
    {
      todo.push_back(it->first);
    }
  }

  // Now we get to work. Update game state, send keepalives. (At the moment "update" only cleans up dead connections.)
  for (std::list<int32_t>::const_iterator it = todo.begin(); it != todo.end(); ++it)
  {
    m_gsm.packetSCKeepAlive(*it);
    m_gsm.update(*it);
  }

  timer = clockTick();
}


void Server::processSchedule10s()
{
  static long long int timer = clockTick();

  long long int now = clockTick();

  //std::cout << "Tick-10s. Actual time since last call is " << std::dec << now - timer << "ms." << std::endl;

  /* do stuff */

  timer = clockTick();
}

void Server::processIngress(int32_t eid, std::deque<unsigned char> & d, std::shared_ptr<std::recursive_mutex> ptr_mutex)
{
  while (!d.empty())
  {
    const unsigned char first_byte(d.front());
    const auto pit = std::find(PACKET_INFO.begin(), PACKET_INFO.end(), first_byte);

    if (pit != PACKET_INFO.end())
    {
      const size_t psize = pit->size; // excludes initial type byte!

      if (psize != size_t(PACKET_VARIABLE_LEN) && d.size() >= psize + 1)
      {
        std::vector<unsigned char> x(psize + 1);

        { // lock guard
          std::lock_guard<std::recursive_mutex> lock(*ptr_mutex);
          for (size_t k = 0; k < psize + 1; ++k)
          {
            x[k] = d.front();
            d.pop_front();
          }
        }

        m_input_parser.immediateDispatch(eid, x);
      }
      else if (psize == size_t(PACKET_VARIABLE_LEN))
      {
        if (!m_input_parser.dispatchIfEnoughData(eid, d, ptr_mutex.get()))
        {
          break;
        }
      }
      else
      {
        break;
      }
    }

    else // pit == end()
    {
      if (pit == PACKET_INFO.end()) std::cout <<"ZZZ";
      std::cout << "Unintellegible data! Clearing buffer for client #" << eid << ". First byte was "
                << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)(first_byte) << std::endl;
      d.clear();
      break;
    }
  }
}

void Server::stop()
{
  // Post a call to the stop function so that Server::stop() is safe to call from any thread.
  m_io_service.post(std::bind(&Server::handleStop, this));

  // Tell the game thread to stop.
  m_server_should_stop = true;

  // Release the input thread's lock.
  m_connection_manager.m_input_ready = true;
  m_connection_manager.m_input_ready_cond.notify_one();
}

void Server::handleAccept(const boost::system::error_code & error)
{
  std::cout << "Server says Hi." << std::endl;
  if (!error)
  {
    m_connection_manager.start(m_next_connection);
    m_next_connection.reset(new Connection(m_io_service, m_connection_manager));
    m_acceptor.async_accept(m_next_connection->socket(), m_next_connection->peer(), std::bind(&Server::handleAccept, this, std::placeholders::_1));
  }
}

void Server::handleStop()
{
  // The server is stopped by cancelling all outstanding asynchronous
  // operations. Once all operations have finished the io_service::run() call
  // will exit.
  m_acceptor.close();
  m_connection_manager.stopAll();
}
