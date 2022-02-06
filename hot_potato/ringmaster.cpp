#include <sys/select.h>
#include <sys/socket.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "potato.hpp"

void Ringmaster::buildServer() {
  memset(&this->host_info, 0, sizeof(this->host_info));  //struct is empt
  this->host_info.ai_flags = AI_PASSIVE;  // fill in my IP for me, bind socket to host IP
  this->host_info.ai_family = AF_UNSPEC;
  this->host_info.ai_socktype = SOCK_STREAM;  //TCP stream sockets
  this->status =
      getaddrinfo(this->hostname, this->port, &this->host_info, &this->host_info_list);

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
  this->status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  this->status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
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
}

int Ringmaster::acceptConnection(std::string & ip) {
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

void Ringmaster::setupConnections() {
  for (int i = 0; i < num_players; i++) {
    client_fd[i] = acceptConnection(client_ip[i]);
    //give each client an ID
    //The ID format is  0, 1, 2, 3 with int value
    send(client_fd[i], &i, sizeof(i), 0);
    send(client_fd[i], &this->num_players, sizeof(this->num_players), 0);

    //will block here
    recv(client_fd[i], &client_port[i], sizeof(client_port[i]), MSG_WAITALL);
    std::cout << "Player " << i << " is ready to play" << std::endl;
  }
}

void Ringmaster::setupClientNeighbourConnection() {
  for (int i = 0; i < num_players; i++) {
    int right_neigh_id = (i + 1) % this->num_players;
    ClientInfo right_client_info;
    right_client_info.port = client_port[right_neigh_id];
    strcpy(right_client_info.addr, client_ip[right_neigh_id].c_str());
    ////////////////
    // std::cout << "Now I sent client [" << i
    //          << "] the info port: " << right_client_info.port
    //          << " addr: " << right_client_info.addr << std::endl;
    send(client_fd[i], &right_client_info, sizeof(right_client_info), 0);
  }
}

void Ringmaster::play() {
  Potato potato;
  potato.setHops(num_hops);
  //If hops run to 0 end the game
  if (potato.getHops() == 0) {
    for (int i = 0; i < num_players; i++) {
      if (send(client_fd[i], &potato, sizeof(potato), 0) != sizeof(potato)) {
        throw PlayExceptions("Wrong potato!\n");
      }
    }
    return;
  }

  srand((unsigned int)time(NULL) + num_players);
  int random = rand() % num_players;
  std::cout << "Ready to start the game, sending potato to player " << random << "\n";
  if (send(client_fd[random], &potato, sizeof(potato), 0) != sizeof(potato)) {
    throw PlayExceptions("Initially Send a Wrong Potato!\n");
  }

  fd_set readfds;
  int nfds = 1 + *std::max_element(client_fd.begin(), client_fd.end());
  FD_ZERO(&readfds);
  for (int i = 0; i < num_players; i++) {
    FD_SET(client_fd[i], &readfds);
  }
  int return_value = select(nfds, &readfds, NULL, NULL, NULL);
  if (return_value != 1) {
    throw PlayExceptions("Select Wrong!\n");
  }

  for (int i = 0; i < num_players; i++) {
    if (FD_ISSET(client_fd[i], &readfds)) {
      int potato_length = recv(client_fd[i], &potato, sizeof(potato), MSG_WAITALL);
      if (potato_length != sizeof(potato)) {
        throw PlayExceptions("Wrong Potato! in receive\n");
      }
      for (int k = 0; k < num_players; k++) {
        potato_length = send(client_fd[k], &potato, sizeof(potato), 0);
        if (potato_length != sizeof(potato)) {
          throw PlayExceptions("Wrong Potato ! in sent\n");
        }
      }
      break;
    }
  }

  potato.printPotato();
  sleep(1);
}

void Ringmaster::printInitially() {
  std::cout << "Potato Ringmaster\n";
  std::cout << "Players = " << num_players << "\n";
  std::cout << "Hops = " << num_hops << "\n";
}

int main(int argc, char ** argv) {
  if (argc < 4) {
    throw ParameterException(
        "example: ./ringmaster <port_num> <num_players> <num_hops>\n");
  }
  if (atoi(argv[2]) <= 0) {
    throw ParameterException("Entering an valid number of players!\n");
  }
  if (atoi(argv[3]) < 0 || atoi(argv[3]) > 512) {
    throw ParameterException("Entering an valid number of hops! (0,512)\n");
  }

  Ringmaster * ringmaster = new Ringmaster(argv);
  ringmaster->printInitially();

  //Note: This is a loop function for num_player times
  //stop every time here waiting for all player(num_player) to join
  ringmaster->setupConnections();
  ringmaster->setupClientNeighbourConnection();
  ringmaster->play();
  delete ringmaster;
  return EXIT_SUCCESS;
}
