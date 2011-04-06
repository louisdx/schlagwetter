#include <functional>
#include <thread>
#include "server.h"
#include "inputparser.h"
#include "packets.h"

#include <iostream>
#include <iomanip>

Server::Server(const std::string & bindaddr, unsigned short int port)
  :
  m_io_service(),
  m_acceptor(m_io_service),
  m_server_should_stop(false),
  m_connection_manager(),
  m_next_connection(new Connection(m_io_service, m_connection_manager)),
  m_gsm(m_connection_manager, m_map),
  m_input_parser(m_gsm),
  m_map()
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

void Server::runGame()
{
  int32_t current_eid = 0;

  while (!m_server_should_stop)
  {

    /*** Phase I: Process the IO queues ***/
    auto it = m_connection_manager.clientData().begin();
    while (it != m_connection_manager.clientData().end() && it->first <= current_eid) ++it;
    if (it == m_connection_manager.clientData().end()) it = m_connection_manager.clientData().begin();
    if (it == m_connection_manager.clientData().end()) continue;
    current_eid = it->first;

    process_ingress(current_eid, it->second.first, it->second.second);

    process_egress(current_eid);

    process_gamestate(current_eid);

    /*** Phase II: Run servery business, timed events, whatnot... ***/
    {

    }

    pthread_yield();

  } // while(...)
}

inline void Server::process_gamestate(int32_t eid)
{
  m_gsm.update(eid);
}

void Server::process_egress(int32_t eid)
{
  auto it = m_connection_manager.clientEgressQ().find(eid);

  if (it == m_connection_manager.clientEgressQ().end()) return;

  std::deque<std::string> & d = it->second;

  while(!d.empty())
  {
    m_connection_manager.sendDataToClient(eid, d.front());
    d.pop_front();
  }
}

void Server::process_ingress(int32_t eid, std::deque<char> & d, std::shared_ptr<std::recursive_mutex> ptr_mutex)
{
  if (!d.empty())
  {
    const unsigned char first_byte(d.front());
    const auto pit = PACKET_INFO.find(EPacketNames(first_byte));

    if (pit != PACKET_INFO.end())
    {
      const size_t psize = pit->second.size; // excludes initial type byte!

      if (psize != size_t(PACKET_VARIABLE_LEN) && d.size() >= psize)
      {
        std::vector<char> x(psize + 1);

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
        m_input_parser.dispatchIfEnoughData(eid, d, ptr_mutex.get());
      }
    }

    else // pit == end()
    {
      if (pit == PACKET_INFO.end()) std::cout <<"ZZZ";
      std::cout << "Unintellegible data! Clearing buffer for client #" << eid << ". First byte was "
                << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)(first_byte) << std::endl;
      d.clear();
    }
  }
}

void Server::stop()
{
  // Post a call to the stop function so that Server::stop() is safe to call from any thread.
  m_io_service.post(std::bind(&Server::handleStop, this));

  // Tell the game thread to stop.
  m_server_should_stop = true;
}

void Server::handleAccept(const boost::system::error_code & error)
{
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
