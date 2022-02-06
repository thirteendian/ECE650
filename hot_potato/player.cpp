#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <ostream>

#include "potato.hpp"

void Player::buildServer() {
  memset(&this->host_info, 0, sizeof(this->host_info));  //struct is empt
  this->host_info.ai_flags = AI_PASSIVE;  // fill in my IP for me, bind socket to host IP
  this->host_info.ai_family = AF_UNSPEC;
  this->host_info.ai_socktype = SOCK_STREAM;  //TCP stream sockets
  this->status = getaddrinfo(
      NULL,
      "",
      &this->host_info,
      &this->host_info_list);  //this->hostname, this->port, &this->host_info, &this->host_info_list);

  if (this->status != 0) {
    std::string error_message;
    error_message.append("Error: cannot get address info for host  (");
    error_message.append(this->hostname);
    error_message.append(",");
    error_message.append(this->port);
    error_message.append(")\n");
    throw SocketExceptions(error_message);
  }
  ///////////////////////////////////////////////////////////////////////////
  //sin_port in socketaddr_in identify the port
  //if sin_port is set to 0, the caller expects the system to assign an available port

  struct sockaddr_in * addr_in = (struct sockaddr_in *)(this->host_info_list->ai_addr);
  addr_in->sin_port = 0;

  ///////////////////////////////////////////////////////////////////////////
  this->socket_fd = socket(this->host_info_list->ai_family,
                           this->host_info_list->ai_socktype,
                           this->host_info_list->ai_protocol);
  if (this->socket_fd == -1) {
    std::string error_message;
    error_message.append("Error: cannot create socket for  (");
    error_message.append(this->hostname);
    error_message.append(",");
    error_message.append(this->port);
    error_message.append(")\n");
    throw SocketExceptions(error_message);
  }  //if

  ////////////////////////////////////////////////////////////////////////////
  int yes = 1;
  this->status = setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  this->status = bind(
      this->socket_fd, this->host_info_list->ai_addr, this->host_info_list->ai_addrlen);
  //ERROR CHECKING ON BIND()
  if (status == -1) {
    std::string error_message;
    error_message.append("Error: cannot bind socket  (");
    error_message.append(this->hostname);
    error_message.append(",");
    error_message.append(this->port);
    error_message.append(")\n");
    throw SocketExceptions(error_message);
  }  //if

  ////////////////////////////////////////////////////////////////////////////
  this->status = listen(this->socket_fd, 100);
  if (this->status == -1) {
    std::string error_message;
    error_message.append("Error: cannot listen on socket  (");
    error_message.append(this->hostname);
    error_message.append(",");
    error_message.append(this->port);
    error_message.append(")\n");
    throw SocketExceptions(error_message);
  }  //if

  this->self_port = this->getPortNum(this->socket_fd);
}

void Player::buildClient() {
  memset(&host_info, 0, sizeof(this->host_info));
  this->host_info.ai_family = AF_UNSPEC;
  this->host_info.ai_socktype = SOCK_STREAM;
  this->status =
      getaddrinfo(this->hostname, this->port, &this->host_info, &this->host_info_list);
  if (this->status != 0) {
    std::string error_message;
    error_message.append("Error: cannot get address info for Client host  (");
    error_message.append(this->hostname);
    error_message.append(",");
    error_message.append(this->port);
    error_message.append(")\n");
    throw SocketExceptions(error_message);
  }  //if

  this->ringmaster_fd = socket(this->host_info_list->ai_family,
                               this->host_info_list->ai_socktype,
                               this->host_info_list->ai_protocol);
  if (this->ringmaster_fd == -1) {
    std::string error_message;
    error_message.append("Error: cannot create socket for Client host (");
    error_message.append(this->hostname);
    error_message.append(",");
    error_message.append(this->port);
    error_message.append(")\n");
    throw SocketExceptions(error_message);
  }  //if

  this->status = connect(this->ringmaster_fd,
                         this->host_info_list->ai_addr,
                         this->host_info_list->ai_addrlen);
  if (this->status == -1) {
    std::string error_message;
    error_message.append("Error: cannot connect to socket for Client host  (");
    error_message.append(this->hostname);
    error_message.append(",");
    error_message.append(this->port);
    error_message.append(")\n");
    throw SocketExceptions(error_message);
  }  //if
  //  return socket_fd;
}

void Player::buildNeighbourClient(const char * client_hostname,
                                  const char * client_port) {
  struct addrinfo host_info_n;
  struct addrinfo * host_info_list_n;
  memset(&host_info_n, 0, sizeof(host_info_n));
  host_info_n.ai_family = AF_UNSPEC;
  host_info_n.ai_socktype = SOCK_STREAM;

  int status_n =
      getaddrinfo(client_hostname, client_port, &host_info_n, &host_info_list_n);
  if (status_n != 0) {
    std::string error_message;
    error_message.append("Error: cannot get address info for Neighbour Client host (");
    error_message.append(client_hostname);
    error_message.append(",");
    error_message.append(client_port);
    error_message.append(")\n");
    throw SocketExceptions(error_message);
  }  //if

  right_neighbour_fd = socket(host_info_list_n->ai_family,
                              host_info_list_n->ai_socktype,
                              host_info_list_n->ai_protocol);
  //
  if (right_neighbour_fd == -1) {
    std::string error_message;
    error_message.append("Error: cannot create socket for Neighbour Client (");
    error_message.append(client_hostname);
    error_message.append(",");
    error_message.append(client_port);
    error_message.append(")\n");
    throw SocketExceptions(error_message);
  }  //if

  //std::cout << "right_neighbour_fd :" << right_neighbour_fd << std::endl;

  status_n = connect(
      right_neighbour_fd, host_info_list_n->ai_addr, host_info_list_n->ai_addrlen);
  if (status_n == -1) {
    std::string error_message;
    error_message.append("Error: cannot connect to socket for Neighbour Client (");
    error_message.append(client_hostname);
    error_message.append(",");
    error_message.append(client_port);
    error_message.append(")\n");
    throw SocketExceptions(error_message);
  }  //if
  freeaddrinfo(host_info_list_n);
  //  return socket_fd;
}

void Player::acceptConnection() {
  //from ringmaster
  recv(ringmaster_fd, &this->player_id, sizeof(this->player_id), 0);
  //from ringmaster
  recv(ringmaster_fd, &this->num_players, sizeof(this->num_players), 0);

  //////////////CLIENT AS SERVER START/////////////////////////
  //server start
  buildServer();

  //Ringmaster:: recv(fd[i],&port[i],sizeof(port[i]),MSG_WAITALL);
  //std::cout << "build client as server self_port:" << self_port << std::endl;
  send(ringmaster_fd, &self_port, sizeof(self_port), 0);

  std::cout << "Connected as player " << player_id << " out of " << num_players
            << " total players\n";
}

int Player::acceptNeighbourConnection(std::string & ip) {
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  int client_connect_fd;
  //////////////////////////////////////////////////////////////////////////////
  //Note if no pending connections are present on the queue
  //program will stop here block caller until connection is present
  client_connect_fd =
      accept(this->socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  //ERROR CHECKING ON ACCEPT()
  if (client_connect_fd == -1) {
    std::string error_message;
    error_message.append("Error: cannot accept connection on socket");
    throw SocketExceptions(error_message);
  }
  struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
  ip = inet_ntoa(addr->sin_addr);

  return client_connect_fd;
}

void Player::setupNeighbourConnection() {
  ClientInfo client_info;

  recv(ringmaster_fd, &client_info, sizeof(client_info), MSG_WAITALL);

  char neighbour_port[9];
  sprintf(neighbour_port, "%d", client_info.port);

  //As a client connected to Neighbour Server
  //std::cout << "Now I receive"
  //          << " neighbour port:" << neighbour_port
  //          << " neighbour addr:" << client_info.addr
  //          << " right_neighbour_fd:" << right_neighbour_fd << std::endl;
  buildNeighbourClient(client_info.addr, neighbour_port);  //, right_neighbour_fd);
  std::string ip_temp;

  //As a Server accept conection from left neigh client
  this->left_neighbour_fd = acceptNeighbourConnection(ip_temp);
}

void Player::play() {
  Potato potato;
  fd_set readfds;
  //set random function seeds
  srand((unsigned int)time(NULL) + player_id);
  std::vector<int> fd{left_neighbour_fd, right_neighbour_fd, ringmaster_fd};
  //because select will examine nfds -1 we plus one here
  int nfds = 1 + *std::max_element(fd.begin(), fd.end());
  //int nfds = 1 + (left_neighbour_fd > right_neighbour_fd ? left_neighbour_fd
  //                                                     : right_neighbour_fd);
  while (true) {
    //initial file descriptor set to 0 bits
    //Note it's an set
    FD_ZERO(&readfds);

    //put fd into readfds set
    for (int i = 0; i < 3; i++) {
      FD_SET(fd[i], &readfds);
    }

    //Which side is ready for reading, readfds is ready to be checked
    int ret_value = select(nfds, &readfds, NULL, NULL, NULL);
    //select will return 0 or -1 if error
    if (ret_value <= 0) {
      throw PlayExceptions("Select function "
                           "Error!\n");
    }

    for (int i = 0; i < 3; i++) {
      //Check if fd is in readfds
      if (FD_ISSET(fd[i], &readfds)) {
        if ((status = recv(fd[i], &potato, sizeof(potato), MSG_WAITALL)) !=
            sizeof(potato)) {
          std::cout << "Received Wrong "
                       "Potatos!\n";
          throw PlayExceptions("Player received "
                               "Wrong Potatos!\n");
        }
        break;
      }
    }

    if (potato.getHops() == 0) {
      return;
    }
    //play on potato
    potato.runOneHops();
    potato.setPath(player_id, potato.getCounts());

    //check if I'm it
    if (potato.getHops() == 0) {
      if (send(ringmaster_fd, &potato, sizeof(potato), 0) != sizeof(potato)) {
        std::cerr << "Send potato to "
                     "master wrong\n";
      }
      std::cout << "I'm it\n";
      continue;
    }

    //send potato to next neighbour:
    int random = rand() % 2;
    if (send(fd[random], &potato, sizeof(potato), 0) != sizeof(potato)) {
      std::cerr << "Send potato to "
                   "Neighbour Wrong\n";
    }

    if (random == 0) {
      int left_neighbour_id = (player_id - 1 + num_players) % num_players;
      std::cout << "Sending potato to " << left_neighbour_id << std::endl;
    }
    else {
      int right_neighbour_id = (player_id + 1) % num_players;
      std::cout << "Sending potato to " << right_neighbour_id << std::endl;
    }
  }
}

int main(int argc, char ** argv) {
  if (argc < 3) {
    throw ParameterException("Example: ./player <machine_name> <port_number>\n");
  }

  //Constructor setup connection to Ringmaster
  Player * player = new Player(argv);

  //Setup connection with neighbour
  player->setupNeighbourConnection();
  player->play();
  sleep(1);
  delete player;
  return EXIT_SUCCESS;
}
