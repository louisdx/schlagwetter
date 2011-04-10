#include <iostream>
#include <iomanip>
#include "ui.h"
#include "connection.h"
#include "packetcrafter.h"

std::string SimpleUI::readline(const std::string & prompt)
{
  std::string line;
  std::cout << prompt;
  std::getline(std::cin, line);
  if (std::cin.eof()) return "exit";
  return line;
}

#ifdef HAVE_GNUREADLINE

GNUReadlineUI::GNUReadlineUI(const std::string & histfile)
  :
  UI(),
  m_histfile(histfile)
{
  rl_cleanup_after_signal();
  using_history();
  read_history(m_histfile.c_str());
}

GNUReadlineUI::~GNUReadlineUI()
{
  write_history(m_histfile.c_str());
}

std::string GNUReadlineUI::readline(const std::string & prompt)
{
  m_line = ::readline(prompt.c_str());

  if (m_line == NULL) return "exit";

  if (*m_line && *m_line != ' ') add_history(m_line);

  std::string line(m_line);
  free(m_line);
  return line;
}

#endif

bool pump(ConnectionManager & connection_manager, UI & ui)
{
  std::string line = ui.readline(std::string("> "));
  std::transform(line.begin(), line.end(), line.begin(), ::tolower);

  if (line == "exit" || line == "quit" || line == "bye")
  {
    return false;
  }
  else if (line == "")
  {
    return true;
  }
  else if (line == "help" || line == "panic")
  {
    std::cout << std::endl
              << "Welcome, friend. This is the LDX Minecraft Server." << std::endl
              << "Please help yourself to the following commands:" << std::endl
              << std::endl
              << "  list:                    Lists all connected users" << std::endl
              << "  dump:                    Dump each connection's data" << std::endl
              << "  raw <client> <data>:     Sends raw data to a client (prob. not very useful)" << std::endl
              << "  kcik <client> <message>: Kicks a client with a given message" << std::endl
              << "  dump:                    Dump each connection's data" << std::endl
              << "  exit:                    Shuts down the server" << std::endl
              << std::endl;
  }
  else if (line == "list")
  {
    for (std::set<ConnectionPtr>::const_iterator i = connection_manager.connections().begin(); i != connection_manager.connections().end(); ++i)
    {
      auto di = connection_manager.clientData().find((*i)->EID());
      std::cout << "Connection #" << std::dec << (*i)->EID() << ": " << (*i)->peer().address().to_string() << ":" << std::dec << (*i)->peer().port()
                << ", #refs = " << i->use_count();
      if (di != connection_manager.clientData().end()) std::cout << ", " << di->second->size() << " bytes of unprocessed data";
      std::cout << std::endl;
    }
  }
  else if (line == "dump")
  {
    for (std::set<ConnectionPtr>::const_iterator i = connection_manager.connections().begin(); i != connection_manager.connections().end(); ++i)
    {
      std::cout << "Connection: " << (*i)->peer().address().to_string() << ":" << std::dec << (*i)->peer().port() << ".";
      auto di = connection_manager.clientData().find((*i)->EID());
      if (di == connection_manager.clientData().end())
      {
        std::cout << " ** no data **";
      }
      else
      {
        // Sorry, we have no thread-safe iteration at the moment, this function has to go.
//        for (auto it = di->second.first.begin(); it != di->second.first.end(); ++it)
//          std::cout << " " << std::hex << std::setw(2) << (unsigned int)(*it);
      }
      std::cout << std::endl;
    }
  }
  else if (line.compare(0, 3, "raw") == 0)
  {
    std::istringstream s(line);
    std::string tmp;
    int32_t eid = -1;

    s >> tmp >> eid;
    std::string t;
    std::getline(s, t);

    if (eid == -1 || t.empty())
    {
      std::cout << "Syntax: raw <client> <message>" << std::endl;
    }
    else
    {
      std::cout << "Trying to send to client #" << eid << " the data \"" << t.substr(1) << "\"." << std::endl;
      connection_manager.sendDataToClient(eid, t.substr(1));
    }
  }
  else if (line.compare(0, 4, "kick") == 0)
  {
    std::istringstream s(line);
    std::string tmp;
    int32_t eid = -1;

    s >> tmp >> eid;
    std::string t;
    std::getline(s, t);

    if (eid == -1 || t.empty())
    {
      std::cout << "Syntax: kick <client> <message>" << std::endl;
    }
    else
    {
      std::cout << "Kicking client #" << eid << " with message \"" << t.substr(1) << "\"." << std::endl;

      PacketCrafter p(PACKET_DISCONNECT);
      p.addJString(t.substr(1));
      connection_manager.sendDataToClient(eid, p.craft());
    }
  }
  else
  {
    std::cout << "Sorry, I did not understand! Type 'help' for help." << std::endl;
  }
  return true;
}
