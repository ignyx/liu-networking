#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// if in doubt, check the man page !

const char PORT[5] = "3490";
const int BACKLOG{10}; // max connections in queue
const int YES{1};      // used as a boolean

void reap_dead_child_processes(int signal);
void *get_in_addr(struct sockaddr *socket_address);
void bind_socket_to_address(int &listening_socket);
void listen_to_socket(int listening_socket);
void reap_child_process_on_end();
void handle_incoming_connection(int socket);

int main() {
  int listening_socket;
  int new_connection_socket;
  struct sockaddr_storage client_address;
  socklen_t client_address_size;
  char client_address_chars[INET6_ADDRSTRLEN];

  std::cout << "Starting proxy server on port " << PORT << std::endl;

  bind_socket_to_address(listening_socket);
  listen_to_socket(listening_socket);
  reap_child_process_on_end();

  std::cout << "server: waiting for connections on port " << PORT << std::endl;

  // Accept all connections
  while (1) {
    client_address_size = sizeof client_address;
    new_connection_socket =
        accept(listening_socket, (struct sockaddr *)&client_address,
               &client_address_size);

    if (new_connection_socket == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(client_address.ss_family,
              get_in_addr((struct sockaddr *)&client_address),
              client_address_chars, sizeof client_address_chars);

    std::cout << "server: got connection from " << client_address_chars
              << std::endl;

    if (!fork()) {             // Create child process
      close(listening_socket); // child doesn't need the listening_socket
      handle_incoming_connection(new_connection_socket);
      exit(0);
    }

    close(new_connection_socket); // parent doesn't need this
  }

  return 0;
}

void reap_dead_child_processes(int) {
  // errno is the error number sent by system calls or libraries
  // in this case it might be overwritten by waitpid()
  int saved_errno = errno;
  // WNOHANG option prevents waitpid() from blocking, we have other things to do
  // https://stackoverflow.com/questions/33508997/waitpid-wnohang-wuntraced-how-do-i-use-these
  //
  // reap dead child processes
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  // restore previous error number
  errno = saved_errno;
}

void *get_in_addr(struct sockaddr *socket_address) {
  if (socket_address->sa_family == AF_INET) {
    // IPv4
    return &(((struct sockaddr_in *)socket_address)->sin_addr);
  } else { // AF_INET6
    // IPv6
    return &(((struct sockaddr_in6 *)socket_address)->sin6_addr);
  }
}

void bind_socket_to_address(int &listening_socket) {
  struct addrinfo hints;
  struct addrinfo *server_info;
  struct addrinfo *ip_addr;
  int status;

  memset(&hints, 0, sizeof hints); // overwrite hints with zeroes
  hints.ai_family = AF_UNSPEC;     // IPv4 / IPv6 agnostic
  hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
  hints.ai_flags = AI_PASSIVE;     // Automatically fill in IP

  // get info on available addresses
  if ((status = getaddrinfo(NULL, PORT, &hints, &server_info)) != 0) {
    std::cout << "getaddrinfo error :" << std::endl << gai_strerror(status);
    exit(1);
  }

  // loop through the available IP addresses and bind tothe first we can
  for (ip_addr = server_info; ip_addr != NULL; ip_addr = ip_addr->ai_next) {
    // Attempts to create a socket
    if ((listening_socket = socket(ip_addr->ai_family, ip_addr->ai_socktype,
                                   ip_addr->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    // Attempts to enable the SO_REUSEADDR socket option
    // SO_REUSEADDR allows the program to re-bind to the IP/PORT combo
    // before the system assumes in-route packets have been dropped. See
    // https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &YES,
                   sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    // Attempts to bind to the IP/PORT to the socket
    if (bind(listening_socket, ip_addr->ai_addr, ip_addr->ai_addrlen) == -1) {
      close(listening_socket);
      perror("server: bind");
      continue;
    }

    // socket successfully bound !
    break;
  }

  freeaddrinfo(server_info);

  if (ip_addr == NULL) {
    std::cerr << "server: failed to bind to any available address" << std::endl;
    exit(1);
  }
}

void listen_to_socket(int listening_socket) {
  if (listen(listening_socket, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }
}

void reap_child_process_on_end() {
  struct sigaction signal_action; // used to communicate with the kernel

  // When child process is done, clean it
  // About SA_RESTART :
  // https://www.gnu.org/software/libc/manual/html_node/Flags-for-Sigaction.html
  signal_action.sa_handler =
      reap_dead_child_processes;       // reap all dead processes
  sigemptyset(&signal_action.sa_mask); // initializes the signal set to empty
  signal_action.sa_flags = SA_RESTART;
  // SIGCHLD is fired when a child process ends
  if (sigaction(SIGCHLD, &signal_action, NULL)) {
    perror("sigaction");
    exit(1);
  }
}

void handle_incoming_connection(int socket) {
  if (send(socket, "Hello world !", 13, 0) == -1) {
    perror("send");
  }
  close(socket);
}
