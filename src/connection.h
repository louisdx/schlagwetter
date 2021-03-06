#ifndef H_CONNECTION
#define H_CONNECTION

#include <set>
#include <memory>
#include <array>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include "syncqueue.h"

class ConnectionManager;


/* Class Connection:  represents a single connection from a client, i.e. an active player. */

class Connection : private boost::noncopyable, public std::enable_shared_from_this<Connection>
{
public:

  Connection(boost::asio::io_service & io_service,  ConnectionManager & manager);

  ~Connection();


  /// Get the socket and peer associated with the connection.

  inline       boost::asio::ip::tcp::socket & socket()       { return m_socket; }
  inline const boost::asio::ip::tcp::socket & socket() const { return m_socket; }
  inline       boost::asio::ip::tcp::endpoint & peer()       { return m_peer; }
  inline const boost::asio::ip::tcp::endpoint & peer() const { return m_peer; }


  /// Entity ID and nickname.

  inline       int32_t EID()        const { return m_EID; }
  inline const std::string & nick() const { return m_nick; }
  inline       std::string & nick()       { return m_nick; }


  /// Start the first asynchronous operation for the connection.

  void start();


  /// Stop all asynchronous operations associated with the connection.

  void stop();


  /// Send data to client.

  inline void sendData(const std::string & data)
  {
    sendData(reinterpret_cast<const unsigned char *>(data.data()), data.length());
  }

  inline void sendData(const unsigned char * data, size_t len)
  {
    boost::asio::async_write(m_socket, boost::asio::buffer(data, len), std::bind(&Connection::handleWrite, shared_from_this(), std::placeholders::_1));
  }

private:

  /// Handle completion of a read operation.

  void handleRead(const boost::system::error_code & e, std::size_t bytes_transferred);


  /// Handle completion of a write operation.

  void handleWrite(const boost::system::error_code & e);

  /// Socket for the connection.

  boost::asio::ip::tcp::socket m_socket;


  /// The peer of the connection

  boost::asio::ip::tcp::endpoint m_peer;


  /// The manager for this connection.

  ConnectionManager & m_connection_manager;


  /// Buffer for incoming data.

  std::array<unsigned char, 8192> m_data;


  /// Local queue for incoming data.

  std::deque<unsigned char> m_local_queue;

  std::recursive_mutex m_local_queue_mutex;


  /// The client's Entity ID and nickname.

  const int32_t m_EID;

  std::string m_nick;
};

typedef std::shared_ptr<Connection> ConnectionPtr;



/*  Class ConnectionManager: manages open connections so that they may be cleanly
 *  stopped when the server needs to shut down; gathers incoming data and sends
 *  out data to clients.
 */

class ConnectionManager : private boost::noncopyable
{
  typedef std::map<int32_t, std::shared_ptr<SyncQueue>> ClientData;

  friend class Server;

public:

  ConnectionManager(boost::asio::io_service & io_service);


  /// The start() and stop() functions are only safe from with the IO thread!


  /// Add the specified connection to the manager and start it.

  void start(ConnectionPtr c);


  /// Stop the specified connection.

  void stop(ConnectionPtr c);

  inline void stop(int32_t eid)
  {
    auto it = findConnectionByEID(eid);
    if (it != m_connections.end()) stop(*it);
  }

  /// A thread-safe stop that posts to the io_service

  inline void safeStop(int32_t eid)
  {
    m_io_service.post(std::bind(static_cast<void(ConnectionManager::*)(int32_t)>(&ConnectionManager::stop), this, eid));
  }


  /// Accessors.

  inline       std::set<ConnectionPtr> & connections()       { return m_connections; }
  inline const std::set<ConnectionPtr> & connections() const { return m_connections; }
  inline const ClientData & clientData()               const { return m_client_data; }
  inline       ClientData & clientData()                     { return m_client_data; }
  inline const std::deque<int32_t> & pendingEIDs()     const { return m_pending_eids; }
  inline       std::deque<int32_t> & pendingEIDs()           { return m_pending_eids; }


  /// Incoming data. May or may not do anything, depending on whether it can lock access to m_client_data.
  /// (Since the only other threads that access m_client_data in Server::runInputProcessing() sleep most
  /// of the time, this should not happen very often.)

  void storeReceivedData(int32_t eid, std::deque<unsigned char> & local_queue);



  /// Outgoing data.

  void sendDataToClient(int32_t eid, const unsigned char * data, size_t len, const char * debug_message = NULL);

  inline void sendDataToClient(int32_t eid, const std::string & data, const char * debug_message = NULL)
  {
    sendDataToClient(eid, reinterpret_cast<const unsigned char *>(data.data()), data.length(), debug_message);
  }


  /// Thread-safe check if a connection exists, by EID.

  inline bool hasConnection(int32_t eid)
  {
    std::lock_guard<std::mutex> lock(m_connections_mutex);
    return std::find_if(m_connections.begin(), m_connections.end(), EIDFinder(eid)) != m_connections.end();
  }


  /// Thread-safe setting and getting of nicknames.

  inline void setNickname(int32_t eid, const std::string & name)
  {
    std::lock_guard<std::mutex> lock(m_connections_mutex);

    auto it = findConnectionByEID(eid);
    if (it != m_connections.end()) (*it)->nick() = name;
  }

  inline std::string getNickname(int32_t eid)
  {
    std::lock_guard<std::mutex> lock(m_connections_mutex);

    auto it = findConnectionByEID(eid);
    return it != m_connections.end() ? (*it)->nick() : "";
  }


  /// Synchronisation.

  std::recursive_mutex m_cd_mutex;
  std::recursive_mutex m_pending_mutex;

private:

  /// Stop all connections.

  void stopAll();


  /// Look up a connection pointer by EID.

  struct EIDFinder
  {
    EIDFinder(int32_t eid) : e(eid) {}

    int32_t e;

    inline bool operator()(const ConnectionPtr & c) const
    {
      return e == c->EID();
    }
  };

  inline std::set<ConnectionPtr>::const_iterator findConnectionByEID(int32_t eid) const
  {
    return std::find_if(m_connections.begin(), m_connections.end(), EIDFinder(eid));
  }


  /// The managed connections.
  /// IMPORTANT: This object must only be modified by the IO thread.
  /// All calls to start(), stop(), stopAll() must be inside ASIO callbacks.

  std::mutex              m_connections_mutex;
  std::set<ConnectionPtr> m_connections;


  /// All incoming data is queued up here for processing.

  ClientData m_client_data;


  /// Synchronisation.

  bool m_input_ready;
  std::condition_variable m_input_ready_cond;
  std::mutex m_input_ready_mutex;


  /// The pending queue allows us to process ingress data FIFO,
  /// rather than just naively iterating over m_client_date front-to-back
  /// looking for some queue with data.

  std::deque<int32_t> m_pending_eids;


  /// The server's io_service

  boost::asio::io_service & m_io_service;
};


#endif
