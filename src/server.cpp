#include <functional>
#include <thread>
#include <iostream>
#include <iomanip>

#include "server.h"
#include "inputparser.h"
#include "constants.h"

// For PROGRAM_OPTIONS and ambientChunks(), respectively, if we precompute chunks.
#include "cmdlineoptions.h"
#include "types.h"

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
  m_map(12500 /* evening */),
  m_gsm(std::bind(&Server::sleepMilli, this, std::placeholders::_1), m_connection_manager, m_map),
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

  if (PROGRAM_OPTIONS["testfile"].as<std::string>().empty())
  {
    std::vector<ChunkCoords> ac = ambientChunks(ChunkCoords(0, 0), PLAYER_CHUNK_HORIZON);
    std::cout << "Precomputing map (" << std::dec << ac.size() << " chunks):" << std::endl << "  Generating terrain: ";

    for (auto i = ac.begin(); i != ac.end(); ++i)
    {
      m_map.ensureChunkIsLoaded(*i);
      std::cout << "*"; std::cout.flush();
    }
    std::cout << std::endl << "Done!" << std::endl;
  }
}

void Server::runIO()
{
  m_io_service.run();
}

void Server::runInputProcessing()
{
  while (!m_server_should_stop)
  {
    // Be extra nice.
    sleepMilli(100);

    {
      std::unique_lock<std::mutex> lock(m_connection_manager.m_input_ready_mutex);
      while (!m_connection_manager.m_input_ready)
      {
        m_connection_manager.m_input_ready_cond.wait(lock, Identity<const bool &>(m_connection_manager.m_input_ready));
      }

      // std::cout << "runInputProcessor() has work to do." << std::endl;

      while (!m_connection_manager.m_pending_eids.empty())
      {
        m_connection_manager.m_cd_mutex.lock();

        ConnectionManager::ClientData::iterator it = m_connection_manager.clientData().find(m_connection_manager.m_pending_eids.front());

        if (it != m_connection_manager.clientData().end())
        {
          const int32_t eid = it->first;
          auto & cd = it->second;

          m_connection_manager.m_cd_mutex.unlock();

          // processIngress() only returns true if all data has been processed.
          if (processIngress(eid, cd))
          {
            // We only remove an EID from the list if its queue is empty.
            std::lock_guard<std::recursive_mutex> lock(m_connection_manager.m_cd_mutex);
            m_connection_manager.m_pending_eids.pop_front();
          }
        }
        else
        {
          m_connection_manager.m_pending_eids.pop_front();
          m_connection_manager.m_cd_mutex.unlock();
        }
      }

    } // scope of the lock
    m_connection_manager.m_input_ready = false;

  } // while (server is running)
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
  static unsigned long long int game_seconds = (m_map.tick_counter % 24000) / 20;

  //std::cout << "Tick-200ms: Actual time was " << std::dec << dt << "ms." << std::endl;

  if (dt < int(m_sleep_next)) sleepMilli(m_sleep_next - dt);
  timer = clockTick();

  /* do stuff */

  m_map.tick_counter += dt / 50; // 20 ticks/s, i.e. one tick every 50ms

  if ((m_map.tick_counter % 24000) / 20 != game_seconds)
  {
    game_seconds = (m_map.tick_counter % 24000) / 20;
    std::cout << "The game time is " << std::dec << std::setw(2) << std::setfill('0')
              << game_seconds / 60 << ":" << std::setw(2) << std::setfill('0') << game_seconds % 60
              << "." << std::endl;
  }

  const long long int work_time = clockTick() - timer;

  // Try to get the next call back in rhythm.
  m_sleep_next = work_time > 200 ? 0 : 200 - work_time;
}

void Server::processSchedule1s()
{
  static long long int timer = clockTick();

  long long int now = clockTick();

  //std::cout << "Tick-1s. Actual time since last call is " << std::dec << now - timer << "ms." << std::endl;

  /* do stuff */
  (void)now;


  // We just gather the active eids quickly and don't hang on to the mutex...
  std::list<int32_t> todo;
  {
    std::unique_lock<std::recursive_mutex> lock(m_connection_manager.m_cd_mutex);
    for (ConnectionManager::ClientData::const_iterator it = m_connection_manager.clientData().begin(); it != m_connection_manager.clientData().end(); ++it)
    {
      todo.push_back(it->first);
    }
  }

  // Now we get to work. Update game state, send keepalives, send map time. (At the moment "update" only cleans up dead connections.)
  for (std::list<int32_t>::const_iterator it = todo.begin(); it != todo.end(); ++it)
  {
    m_gsm.packetSCKeepAlive(*it);
    m_gsm.packetSCTime(*it, m_map.tick_counter % 24000);
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
  (void)now;


  timer = clockTick();
}

bool Server::processIngress(int32_t eid, std::shared_ptr<SyncQueue> d)
{
  while (!d->empty())
  {
    const unsigned char first_byte(d->front());
    const auto pit = std::find(PACKET_INFO.begin(), PACKET_INFO.end(), first_byte);

    if (pit != PACKET_INFO.end())
    {
      const size_t psize = pit->size; // excludes initial type byte!

      if (psize != size_t(PACKET_VARIABLE_LEN) && d->size() >= psize + 1)
      {
        std::vector<unsigned char> x(psize + 1);
        for (size_t k = 0; k < psize + 1; ++k)  x[k] = d->pop();

        // Here we are guaranteed to process a whole packet.
        m_input_parser.immediateDispatch(eid, x);
      }
      else if (psize == size_t(PACKET_VARIABLE_LEN))
      {
        if (!m_input_parser.dispatchIfEnoughData(eid, d)) 
        {
          // At this stage, the queue didn't have enough data...
          return false;
        }
        // ... while at this stage we managed to extract a whole packet.

      }
      else
      {
        return false;
      }
    }

    else // pit == end()
    {
      if (pit == PACKET_INFO.end()) std::cout << "[Packet ID unkown] ";
      std::cout << "Unintellegible data! Clearing buffer for client #" << eid << ". First byte was "
                << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)(first_byte) << std::endl;
      d->clear();
      return true;
    }
  }
  return true;
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
