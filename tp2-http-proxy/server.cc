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
#include <vector>

// if in doubt, check the man page !

const char PORT[5] = "3490";
const int BACKLOG{10};       // max connections in queue
const int YES{1};            // used as a boolean
const int MAXDATASIZE{1028}; // max number of bytes we can get at once

struct http_request_heading;
struct http_header;
void reap_dead_child_processes(int signal);
void *get_in_addr(struct sockaddr *socket_address);
void bind_socket_to_address(int &listening_socket);
void listen_to_socket(int listening_socket);
void reap_child_process_on_end();
void handle_incoming_connection(int socket);
http_request_heading parse_http_request_header(char buffer[MAXDATASIZE]);

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

struct http_header {
  std::string name;
  std::string value;
};

struct http_request_heading {
  std::string method;
  std::string path;
  std::vector<http_header> headers;
};

void handle_incoming_connection(int socket) {
  char buffer[MAXDATASIZE];
  int number_of_bytes;
  http_request_heading heading;

  if (send(socket, "HTTP/1.1 200 OK\n\nhi!", 20, 0) == -1) {
    perror("send");
  }

  if ((number_of_bytes = recv(socket, buffer, MAXDATASIZE - 1, 0)) == -1) {
    perror("recv");
    return;
  }
  buffer[number_of_bytes] = '\0';
  std::cout << "server: received " << buffer << std::endl;

  if (send(socket, "Hello dude !", 12, 0) == -1) {
    perror("send");
  }

  heading = parse_http_request_header(buffer);

  std::cout << "Extracted method : " << heading.method << std::endl;
  std::cout << "Extracted path   : " << heading.path << std::endl;
  for (http_header header : heading.headers) {
    std::cout << "Extracted header : " << header.name << ": " << header.value
              << std::endl;
  }

  close(socket);
}

http_request_heading parse_http_request_header(char buffer[MAXDATASIZE]) {
  int index{0};
  int line{0};
  int word{0};
  int start_of_word{0};
  http_request_heading heading;
  http_header current_header;
  static const http_header empty_header;

  while (buffer[index] != '\0' && index < MAXDATASIZE) {
    // TODO skip body
    std::cout << buffer[index];
    if (buffer[index] == ' ') {
      // end of a word

      if (line == 0 && word == 0) {
        heading.method = std::string(buffer, index - start_of_word);
      } else if (line == 0 && word == 1) {
        heading.path =
            std::string(buffer, start_of_word, index - start_of_word);
      }
      word++;
      start_of_word = index + 1;

    } else if (buffer[index] == ':' && current_header.name == "") {
      current_header.name =
          std::string(buffer, start_of_word, index - start_of_word);
    } else if (line >= 1 && current_header.name != "" &&
               buffer[index] == '\r') {
      current_header.value =
          std::string(buffer, start_of_word, index - start_of_word);
      heading.headers.push_back(current_header);
      current_header = empty_header;

    } else if (buffer[index] == '\n') {
      word = 0;
      line++;
      start_of_word = index + 1;
    }
    index++;
  }

  std::cout << std::flush;

  return heading;
}
