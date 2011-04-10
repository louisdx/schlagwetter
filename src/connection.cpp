#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <functional>

#include "cmdlineoptions.h"
#include "connection.h"

int32_t ConnectionManager::EID_POOL = 0;

Connection::Connection(boost::asio::io_service & io_service, ConnectionManager & manager)
  :
  m_socket(io_service),
  m_connection_manager(manager),
  m_EID(ConnectionManager::GenerateEID()),
  m_nick()
{
  std::cout << "Connection created." << std::endl;
}

Connection::~Connection()
{
  std::cout << "Connection destroyed." << std::endl;
}

void Connection::start()
{
  m_socket.async_read_some(boost::asio::buffer(m_data.data(), m_data.size()), std::bind(&Connection::handleRead, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void Connection::stop()
{
  m_socket.close();
}

void Connection::handleRead(const boost::system::error_code & e, std::size_t bytes_transferred)
{
  if (!e)
  {
    if (PROGRAM_OPTIONS.count("verbose"))
    {
      std::cout << "Received data from client #" << EID() << " (" << std::dec << bytes_transferred << " bytes):";
      for (size_t i = 0; i < bytes_transferred; ++i) std::cout << " " << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)(unsigned char)(m_data[i]);
      std::cout << std::endl;
    }

    // We store the data in a local queue, which is very fast. Here we must wait for the lock.
    {
      std::unique_lock<std::recursive_mutex> lock(m_local_queue_mutex);
      m_local_queue.insert(m_local_queue.end(), m_data.data(), m_data.data() + bytes_transferred);
    }

    // Later, storeReceivedData() may or may not be able to process our queue, but we don't care.
    m_connection_manager.storeReceivedData(EID(), m_local_queue);

    // Set up the next read operation.
    m_socket.async_read_some(boost::asio::buffer(m_data.data(), m_data.size()), std::bind(&Connection::handleRead, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
  }
  else if (e != boost::asio::error::operation_aborted)
  {
    m_connection_manager.stop(shared_from_this());
  }
}

void Connection::handleWrite(const boost::system::error_code & e)
{
  if (!e)
  {
    // Initiate graceful connection closure.
    //boost::system::error_code ignored_ec;
    //m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
  }

  if (e && (e != boost::asio::error::operation_aborted))
  {
    std::cout << "Received error: " << e << std::endl;
    m_connection_manager.stop(shared_from_this());
  }
}

void Connection::sendData(const std::string & data)
{
  boost::asio::async_write(m_socket, boost::asio::buffer(data.data(), data.length()), std::bind(&Connection::handleWrite, shared_from_this(), std::placeholders::_1));
}


ConnectionManager::ConnectionManager()
  :
  m_cd_mutex(),
  m_connections(),
  m_client_data(),
  m_input_ready(false),
  m_input_ready_cond(),
  m_input_ready_mutex(),
  m_pending_eids()
{
}

void ConnectionManager::start(ConnectionPtr c)
{
  m_connections.insert(c);
  c->start();
}

void ConnectionManager::stop(ConnectionPtr c)
{
  m_connections.erase(c);
  c->stop();
}

void ConnectionManager::stopAll()
{
  std::for_each(m_connections.begin(), m_connections.end(), std::bind(&Connection::stop, std::placeholders::_1));
  m_connections.clear();
}

void ConnectionManager::storeReceivedData(int32_t eid, std::deque<unsigned char> & local_queue)
{
  // We have to lock access to m_client_data while using the iterator.
  // An upgradable r/w lock would be apt here, but STL doesn't have one.
  std::unique_lock<std::recursive_mutex> lock(m_cd_mutex);

  ClientData::iterator clit = m_client_data.find(eid);

  /* Two possibilities:
   * Either we already have a queue object for this EID,
   * or we must construct one.
   */

  // Case 1: The queue already exists.
  if (clit != m_client_data.end())
  {
    // If we can get the lock, we just store the data and are done. If not, no big deal.

    if (clit->second.second->try_lock())
    {
      try
      {
        clit->second.first.insert(clit->second.first.end(), local_queue.begin(), local_queue.end());
        local_queue.clear();
      }
      catch (...)
      {
        // OK, this should never happen, but we have to worry about exceptions while holding the lock.
        std::cerr << "Error while writing to m_client_data[" << std::dec << eid << "]." << std::endl;
      }
      clit->second.second->unlock();
    }
  }

  // Case 2: Queue doesn't exist, we create it. No need to lock m_client_data in this case.
  else
  {
    std::pair<ClientData::iterator, bool> ret;

    ret = m_client_data.insert(ClientData::value_type(eid, ClientData::mapped_type(std::deque<unsigned char>(), std::make_shared<std::recursive_mutex>())));

    if (!ret.second)
    {
      std::cerr << "Panic: Could not create comm queue!" << std::endl;
      return;
    }
    else
    {
      clit = ret.first;
      if (clit->second.second->try_lock())
      {
        clit->second.first.insert(clit->second.first.end(), local_queue.begin(), local_queue.end());
        local_queue.clear();

        clit->second.second->unlock();
      }
    }
  }

  m_pending_eids.push_back(eid);
  m_input_ready = true;
  m_input_ready_cond.notify_one();
}

void ConnectionManager::sendDataToClient(int32_t eid, const std::string & data, const char * debug_message) const
{
  auto it = findConnectionByEID(eid);
  if (it == m_connections.end())
  {
    std::cout << "Client #" << eid << " not available, discarding data." << std::endl;
  }
  else
  {
    if (PROGRAM_OPTIONS.count("verbose"))
    {
      std::cout << "Sending data to client #" << eid << ", " << data.length() << " bytes." << (debug_message ? debug_message : "") << std::endl;
    }
#ifdef DEBUG
    else if (debug_message) // No need for "verbose", just print annotated packets.
    {
      std::cout << "Sending data to client #" << eid << ", " << data.length() << " bytes." << debug_message << std::endl;
    }
#endif

    /*
    std::cout << "Sending data to client #" << eid << ":";
    for (size_t i = 0; i < data.length(); ++i)
      std::cout << " " << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)(unsigned char)(data[i]);
    std::cout << std::endl;
    */
    (*it)->sendData(data);
  }
}
