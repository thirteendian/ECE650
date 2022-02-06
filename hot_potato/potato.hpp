#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>
////////////////////////////////////////////////////////////////////////////////////////////////////
class ParameterException : public std::exception {
 public:
  std::string Message;
  ParameterException(std::string whaterror) {
    Message.append("Parameter Error");
    Message.append(whaterror);
  }
  const char * what() const throw() { return Message.c_str(); }
};

class SocketExceptions : public std::exception {
 public:
  std::string Message;
  SocketExceptions(std::string whaterror) {
    Message.append("Socket Error: ");
    Message.append(whaterror);
  }
  const char * what() const throw() { return Message.c_str(); }
};

class PlayExceptions : public std::exception {
 public:
  std::string Message;
  PlayExceptions(std::string whaterror) {
    Message.append("Play Process Error:");
    Message.append(whaterror);
  }
  const char * what() const throw() { return Message.c_str(); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class Potato {
 private:
  int Hops;
  int Counts;
  int Path[512];

 public:
  Potato() : Hops(0), Counts(0) { memset(Path, 0, sizeof(Path)); }
  void printPotato() {
    std::cout << "Trace of potato:\n";
    for (int i = 0; i < Counts; i++) {
      std::cout << Path[i];
      if (i == Counts - 1) {
        std::cout << "\n";
      }
      else {
        std::cout << ",";
      }
    }
  }
  int getHops() { return Hops; }
  int getCounts() { return Counts; }
  int * getPath() { return Path; }
  void setPath(int player_id, int position) {
    Path[position] = player_id;
    Counts++;
  }
  void setHops(int num_hops) { this->Hops = num_hops; }
  void runOneHops() { Hops--; }
};

class ClientInfo {
 public:
  char addr[100];
  int port;
  ClientInfo() : port(-1) { memset(addr, 0, sizeof(addr)); }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class Socket {
 public:
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;  //will point to the results
  const char * hostname;             //= NULL;      //????????????????
  const char * port;
  Socket() : status(0), socket_fd(0), host_info_list(NULL), hostname(NULL), port(NULL) {}
  Socket(const char * port) :
      status(0), socket_fd(0), host_info_list(NULL), hostname(NULL), port(port) {}
  Socket(const char * hostname, const char * port) :
      status(0), socket_fd(0), host_info_list(NULL), hostname(hostname), port(port) {}
  ~Socket() {
    if (socket_fd != 0)
      close(socket_fd);
    if (host_info_list != NULL)
      freeaddrinfo(host_info_list);
  }

  int getPortNum(int socket_fd) {
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(socket_fd, (struct sockaddr *)&sin, &len) == -1) {
      std::string error_message;
      error_message.append("Error: cannot get socket name");
      throw ParameterException(error_message);
    }
    return ntohs(sin.sin_port);
  }
};
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
class Ringmaster : public Socket {
 public:
  int num_players;
  int num_hops;

  //Communicate Client as client
  std::vector<int> client_fd;
  std::vector<std::string> client_ip;

  //Client as Server port
  std::vector<int> client_port;

  void buildServer();

  Ringmaster(char ** argv) : Socket(argv[1]) {
    num_players = atoi(argv[2]);
    num_hops = atoi(argv[3]);
    client_fd.resize(num_players);
    client_ip.resize(num_players);
    client_port.resize(num_players);
    this->buildServer();
  }
  virtual ~Ringmaster() {
    for (int i = 0; i < num_players; i++) {
      close(client_fd[i]);
    }
  }

  int acceptConnection(std::string & ip);
  void printInitially();
  void setupConnections();
  void setupClientNeighbourConnection();
  void play();
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class Player : public Socket {
 private:
  int player_id;
  int num_players;
  int ringmaster_fd;       //ringmaster
  int self_port;           //player server listening port
  int left_neighbour_fd;   //id-1
  int right_neighbour_fd;  //id+1
  void buildServer();      //server to each other
  void buildClient();      //client to ringmaster
  void buildNeighbourClient(const char * client_hostname,
                            const char * client_port);  //,
                                                        //int & neigh_fd);
  void acceptConnection();

 public:
  Player(char ** argv) : Socket(argv[1], argv[2]) {
    this->buildClient();
    this->acceptConnection();
  }
  virtual ~Player() {
    close(ringmaster_fd);
    close(right_neighbour_fd);
    close(left_neighbour_fd);
  }
  void setupNeighbourConnection();

  int acceptNeighbourConnection(std::string & ip);
  void play();
};
