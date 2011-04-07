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

class ConnectionManager;


/* Class Connection:  represents a single connection from a client, i.e. an active player. */

class Connection : private boost::noncopyable, public std::enable_shared_from_this<Connection>
{
public:
  Connection(boost::asio::io_service & io_service,  ConnectionManager & manager);
  ~Connection();

  static int32_t GenerateEID() { return ++EID_POOL; }
  static int32_t EID_POOL;

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
  void sendData(const std::string & data);

  /// Mutexes to protect access
  std::recursive_mutex io_mutex;  // for handleRead, handleWrite

private:
  /// Handle completion of a read operation.
  void handleRead(const boost::system::error_code & e, std::size_t bytes_transferred);

  /// Handle completion of a write operation.
  void handleWrite(const boost::system::error_code& e);

  /// Socket for the connection.
  boost::asio::ip::tcp::socket m_socket;

  /// The peer of the connection
  boost::asio::ip::tcp::endpoint m_peer;

  /// The manager for this connection.
  ConnectionManager & m_connection_manager;

  /// Buffer for incoming data.
  enum { read_buf_size = 4096 };
  char m_data[read_buf_size];

  /// The client's Entity ID and nickname.
  const int32_t m_EID;
  std::string m_nick;
};

typedef std::shared_ptr<Connection> ConnectionPtr;


/* Class ConnectionManager: manages open connections so that they may be cleanly
 * stopped when the server needs to shut down; gathers incoming data and sends
 * out data to clients.
 */

class ConnectionManager : private boost::noncopyable
{
  typedef std::map<int32_t, std::pair<std::deque<char>, std::shared_ptr<std::recursive_mutex>>> ClientData;

  friend class Server;

public:
  ConnectionManager();

  /// Add the specified connection to the manager and start it.
  void start(ConnectionPtr c);

  /// Stop the specified connection.
  void stop(ConnectionPtr c);
  inline void stop(int32_t eid) { auto it = findConnectionByEID(eid); if (it != m_connections.end()) stop(*it); }

  /// Stop all connections.
  void stopAll();

  /// Accessors
  inline       std::set<ConnectionPtr> & connections()       { return m_connections; }
  inline const std::set<ConnectionPtr> & connections() const { return m_connections; }
  inline const ClientData & clientData()               const { return m_client_data; }
  inline       ClientData & clientData()                     { return m_client_data; }

  /// Incoming data
  void storeReceivedData(int32_t eid, char * first, char * last);

  /// Outgoing data
  void sendDataToClient(int32_t eid, const std::string & data) const;

  /// Look up a connection pointer by EID
  struct EIDFinder { EIDFinder (int32_t eid) : e(eid) {} int32_t e; inline bool operator()(const ConnectionPtr & c) { return e == c->EID(); } };
  inline std::set<ConnectionPtr>::const_iterator findConnectionByEID(int32_t eid) const { return std::find_if(m_connections.begin(), m_connections.end(), EIDFinder(eid)); }
  inline Connection * findConnectionByEIDwp(int32_t eid) const { auto it = findConnectionByEID(eid); return it == m_connections.end() ? NULL : it->get(); }

  /// Synchronisation.
  std::mutex m_cd_mutex;

private:
  /// The managed connections.
  std::set<ConnectionPtr> m_connections;

  /// All incoming data is queued up here for processing.
  ClientData m_client_data;

  /// Synchronisation.
  bool m_input_ready;
  std::condition_variable m_input_ready_cond;
  std::mutex m_input_ready_mutex;
  std::deque<int32_t> m_pending_eids;
};


#endif
