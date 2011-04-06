#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <functional>
#include "connection.h"

int32_t Connection::EID_POOL = 0;

Connection::Connection(boost::asio::io_service & io_service, ConnectionManager & manager)
  : m_socket(io_service), m_connection_manager(manager), m_EID(GenerateEID()), m_nick()
{
  std::cout << "Connection created." << std::endl;
}

Connection::~Connection()
{
  //std::lock_guard<std::recursive_mutex> lock(io_mutex);

  std::cout << "Connection destroyed." << std::endl;
}

void Connection::start()
{
  m_socket.async_read_some(boost::asio::buffer(m_data, read_buf_size), std::bind(&Connection::handleRead, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void Connection::stop()
{
  m_socket.close();
}

void Connection::handleRead(const boost::system::error_code & e, std::size_t bytes_transferred)
{
  if (!e)
  {
    std::cout << "Received data from client #" << EID() << " (" << std::dec << bytes_transferred << " bytes):";
    for (size_t i = 0; i < bytes_transferred; ++i) std::cout << " " << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)(unsigned char)(m_data[i]);
    std::cout << std::endl;

    {
      //std::lock_guard<std::recursive_mutex> lock(io_mutex);
      m_connection_manager.storeReceivedData(EID(), m_data, m_data + bytes_transferred);
      m_socket.async_read_some(boost::asio::buffer(m_data, read_buf_size), std::bind(&Connection::handleRead, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
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
  m_connections(),
  m_client_data(),
  m_egress_queue()
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

void ConnectionManager::storeReceivedData(int32_t eid, char * first, char * last)
{
  ClientData::iterator clit = m_client_data.find(eid);
  if (clit != m_client_data.end())
  {
    std::lock_guard<std::recursive_mutex> lock(*clit->second.second);
    clit->second.first.insert(clit->second.first.end(), first, last);
  }
  else
  {
    auto ret = m_client_data.insert(ClientData::value_type(eid, ClientData::mapped_type(std::deque<char>(), std::shared_ptr<std::recursive_mutex>(new std::recursive_mutex))));
    if (!ret.second)
    {
      std::cerr << "Panic: Could not create comm queue!" << std::endl;
    }
    else
    {
      clit = ret.first;
      std::lock_guard<std::recursive_mutex> lock(*clit->second.second);
      clit->second.first.insert(clit->second.first.end(), first, last);
    }
  }
}

void ConnectionManager::sendDataToClient(int32_t eid, const std::string & data) const
{
  auto it = findConnectionByEID(eid);
  if (it == m_connections.end())
  {
    std::cout << "Client #" << eid << " not available, discarding data." << std::endl;
  }
  else
  {
    std::cout << "Sending data to client #" << eid << ":";
    for (size_t i = 0; i < data.length(); ++i)
      std::cout << " " << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)(unsigned char)(data[i]);
    std::cout << std::endl;

    (*it)->sendData(data);
  }
}
