#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// if in doubt, check the man page !

static const char PORT[5] = "3490";
// static const char PORT[5] = "3491";
static const int BACKLOG{10};       // max connections in queue
static const int YES{1};            // used as a boolean
static const int MAXDATASIZE{1028}; // max number of bytes we can get at once
static const int TIMEOUT_SECONDS{15};

struct http_request_heading;
struct http_header;
struct client_connection;
struct http_response;
void reap_dead_child_processes(int signal);
void *get_in_addr(struct sockaddr *socket_address);
void bind_socket_to_address(int &listening_socket);
void listen_to_socket(int listening_socket);
void reap_child_process_on_end();
void handle_incoming_connection(int socket);
http_request_heading parse_http_request_header(const char buffer[MAXDATASIZE]);
int await_request(int socket);
int read_request(int socket, char buffer[MAXDATASIZE]);
int open_client_socket(std::string web_address);
void to_lowercase(std::string &string);
int await_response(int socket, int timeout = 10000);
int handle_incoming_request(client_connection &client);
std::string get_http_request_headers_string(http_request_heading heading);
int send_string(const int socket, std::string const &string);
int send_response(const int socket, http_response const &response);
int send_bytes(const int socket, const char bytes[],
               const unsigned long int length);
http_response parse_http_response_header(const char buffer[MAXDATASIZE]);
int read_response_body(client_connection const &client, http_response &response,
                       unsigned long int index);
std::string build_http_response_headers_string(http_response const &response);
void replace_string_in_place(std::string &string, const std::string &search,
                             const std::string &replace);
void manipulate_response(http_response &response);

enum Log_Level {
  DEBUG,
  INFO,
  WARNING,
  ERROR // probably just crashes

};
static const Log_Level log_level = INFO;

int main() {
  int listening_socket;
  int new_connection_socket;
  struct sockaddr_storage client_address;
  socklen_t client_address_size;
  char client_address_chars[INET6_ADDRSTRLEN];

  if (log_level <= INFO)
    std::cout << "Starting proxy server on port " << PORT << std::endl;

  bind_socket_to_address(listening_socket);
  listen_to_socket(listening_socket);
  reap_child_process_on_end();

  if (log_level <= INFO)
    std::cout << "server: waiting for connections on port " << PORT
              << std::endl;

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

    if (log_level <= INFO)
      std::cout << "IN (socket " << new_connection_socket
                << ") connection opened from " << client_address_chars
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

// handler function for reaping child processes after the connection is closed
void reap_dead_child_processes(int) {
  // errno is the error number sent by system calls or libraries
  // in this case it might be overwritten by waitpid()
  int saved_errno = errno;
  // WNOHANG option prevents waitpid() from blocking, we have other things to
  // do
  // https://stackoverflow.com/questions/33508997/waitpid-wnohang-wuntraced-how-do-i-use-these
  //
  // reap dead child processes
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  // restore previous error number
  errno = saved_errno;
}

// get IP adress
void *get_in_addr(struct sockaddr *socket_address) {
  if (socket_address->sa_family == AF_INET) {
    // IPv4
    return &(((struct sockaddr_in *)socket_address)->sin_addr);
  } else { // AF_INET6
    // IPv6
    return &(((struct sockaddr_in6 *)socket_address)->sin6_addr);
  }
}

// bind UNIX socket to host IP address
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
    if (log_level <= ERROR)
      std::cout << "getaddrinfo error :" << std::endl << gai_strerror(status);
    exit(1);
  }

  // loop through the available IP addresses and bind tothe first we can
  for (ip_addr = server_info; ip_addr != NULL; ip_addr = ip_addr->ai_next) {
    // Attempts to create a socket
    if ((listening_socket = socket(ip_addr->ai_family, ip_addr->ai_socktype,
                                   ip_addr->ai_protocol)) == -1) {
      if (log_level <= ERROR)
        perror("server: socket");
      continue;
    }

    // Attempts to enable the SO_REUSEADDR socket option
    // SO_REUSEADDR allows the program to re-bind to the IP/PORT combo
    // before the system assumes in-route packets have been dropped. See
    // https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &YES,
                   sizeof(int)) == -1) {
      if (log_level <= ERROR)
        perror("setsockopt");
      exit(1);
    }

    // Attempts to bind to the IP/PORT to the socket
    if (bind(listening_socket, ip_addr->ai_addr, ip_addr->ai_addrlen) == -1) {
      close(listening_socket);
      if (log_level <= ERROR)
        perror("server: bind");
      continue;
    }

    // socket successfully bound !
    break;
  }

  freeaddrinfo(server_info);

  if (ip_addr == NULL) {
    if (log_level <= ERROR)
      std::cerr << "server: failed to bind to any available address"
                << std::endl;
    exit(1);
  }
}

// Start listening for new connections host socket
void listen_to_socket(int listening_socket) {
  if (listen(listening_socket, BACKLOG) == -1) {
    if (log_level <= ERROR)
      perror("listen");
    exit(1);
  }
}

// cleanup child processes when finished
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
    if (log_level <= ERROR)
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
  std::string hostname;
  std::vector<http_header> headers;
  bool keep_alive;
};

struct client_connection {
  int client_socket;
  int open_server_socket;
};

// listen and proxy requests until connection times out
void handle_incoming_connection(int socket) {
  client_connection client;
  client.client_socket = socket;
  bool client_timed_out{false};

  while (!client_timed_out) {
    if (await_request(socket)) {
      if (handle_incoming_request(client) == 0) {
        continue;
      } else {
        // POLLIN event + no bytes read means remote closed connection.
        // https://stackoverflow.com/questions/63101815/pollin-event-on-tcp-socket-but-no-data-to-read
        if (log_level <= INFO)
          std::cout << "IN (socket " << socket
                    << ") connection closed (remote closed connection)"
                    << std::endl;
        close(socket);

        return;
      }

    } else {
      client_timed_out = true;

      if (log_level <= INFO)
        std::cout << "IN (socket " << socket
                  << ") connection closed (timed out)" << std::endl;
      close(socket);
    }
  }

  if (log_level <= INFO)
    std::cout << "IN (socket " << socket << ") connection closed" << std::endl;
  close(socket);
}

// parse HTTP request headers from data buffer
http_request_heading parse_http_request_header(const char buffer[MAXDATASIZE]) {
  int index{0};
  int line{0};
  int word{0};
  int start_of_word{0};
  http_request_heading heading;
  heading.keep_alive = false;
  http_header current_header;
  static const http_header empty_header;

  while (buffer[index] != '\0' && index < MAXDATASIZE) {
    if (log_level <= DEBUG)
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
      to_lowercase(current_header.name);
    } else if (line >= 1 && current_header.name != "" &&
               buffer[index] == '\r') {
      current_header.value =
          std::string(buffer, start_of_word, index - start_of_word);

      if (current_header.name == "connection" &&
          current_header.value == "keep-alive")
        heading.keep_alive = true;
      if (current_header.name == "host")
        heading.hostname =
            current_header.value.substr(0, current_header.value.find(':'));
      heading.headers.push_back(current_header);
      current_header = empty_header;

    } else if (buffer[index] == '\n') {
      word = 0;
      line++;
      start_of_word = index + 1;
      current_header.name = "";
    }
    index++;
  }

  if (log_level <= DEBUG) {
    std::cout << "Extracted method : " << heading.method << "\n"
              << "Extracted path   : " << heading.path << "\n"
              << "Host             : " << heading.hostname << "\n"
              << "Keep-alive       : " << heading.keep_alive << "\n";
    for (http_header header : heading.headers) {
      std::cout << "Extracted header : " << header.name << ": " << header.value
                << "\n";
    }
  }

  std::cout << std::flush;

  return heading;
}

// wait until a request arrives or timeout
int await_request(int socket) {
  struct pollfd file_descriptor[1];
  file_descriptor[0].fd = socket;
  file_descriptor[0].events = POLLIN;
  int poll_response;

  if (log_level == DEBUG)
    std::cout << "IN (socket " << socket << ") polling..." << std::endl;

  // listen to events on 1 socket with a timeout of 10 000 ms
  // 0 means the request timed out
  poll_response = poll(file_descriptor, 1, 10000);

  if (log_level == DEBUG)
    std::cout << "IN (socket " << socket << ") polled with response "
              << poll_response << std::endl;

  return poll_response;
}

// read data from one packet (or up to MAXDATASIZE bytes)
int read_request(int socket, char buffer[MAXDATASIZE]) {
  int number_of_bytes;

  if ((number_of_bytes = recv(socket, buffer, MAXDATASIZE - 1, 0)) == -1) {
    if (log_level <= ERROR)
      perror("recv");
    return 0;
  }
  buffer[number_of_bytes] = '\0';

  if (log_level <= DEBUG)
    std::cout << "server: received " << number_of_bytes << " bytes from socket "
              << socket << std::endl;

  return number_of_bytes;
}

// open a client connection to host server
int open_client_socket(std::string web_address) {
  int client_socket;
  struct addrinfo hints;
  struct addrinfo *server_info;
  struct addrinfo *ip_addr;
  int status;
  char server_ip_address[INET6_ADDRSTRLEN];

  // convert string to char[]
  char *hostname = new char[web_address.length() + 1];
  strcpy(hostname,
         web_address.c_str()); // vulnerable if null byte in string

  memset(&hints, 0, sizeof hints); // overwrite hints with zeroes
  hints.ai_family = AF_UNSPEC;     // IPv4 / IPv6 agnostic
  hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

  // get info on available addresses
  if ((status = getaddrinfo(hostname, "80", &hints, &server_info)) != 0) {
    if (log_level <= ERROR)
      std::cout << "client getaddrinfo error : \n"
                << gai_strerror(status) << std::endl;
    return -1;
  }

  // loop through the available IP addresses and bind tothe first we can
  for (ip_addr = server_info; ip_addr != NULL; ip_addr = ip_addr->ai_next) {
    // Attempts to create a socket
    if ((client_socket = socket(ip_addr->ai_family, ip_addr->ai_socktype,
                                ip_addr->ai_protocol)) == -1) {
      if (log_level <= ERROR)
        perror("client: socket");
      continue;
    }

    // get server IP address string
    inet_ntop(ip_addr->ai_family,
              get_in_addr((struct sockaddr *)ip_addr->ai_addr),
              server_ip_address, sizeof server_ip_address);
    if (log_level <= DEBUG)
      std::cout << "OUT(socket " << client_socket << ") client: connecting to "
                << server_ip_address << std::endl;

    if (connect(client_socket, ip_addr->ai_addr, ip_addr->ai_addrlen) == -1) {
      std::cout << "client connect ip: " << server_ip_address << std::endl;
      close(client_socket);
      perror("client: connect");
      continue;
    }

    // socket successfully bound !
    break;
  }

  freeaddrinfo(server_info);
  delete[] hostname; // free memory

  return client_socket;
}

// convert a string to lowercase (in place)
void to_lowercase(std::string &string) {
  for (unsigned int index; index < string.length(); index++) {
    if (isupper(string[index]))
      string[index] = tolower(string[index]);
  }
}

// wait for response packet or timeout
int await_response(int socket, int timeout) {
  struct pollfd file_descriptor[1];
  file_descriptor[0].fd = socket;
  file_descriptor[0].events = POLLIN;
  int poll_response;

  if (log_level == DEBUG)
    std::cout << "OUT (socket " << socket << ") polling..." << std::endl;

  // listen to events on 1 socket with a default timeout of 10 000 ms
  // 0 means the request timed out
  poll_response = poll(file_descriptor, 1, timeout);

  if (log_level == DEBUG)
    std::cout << "OUT (socket " << socket << ") polled with response "
              << poll_response << std::endl;

  return poll_response;
}

struct http_response {
  std::string status_code;
  std::vector<http_header> headers;
  bool keep_alive;
  unsigned long int content_length;
  unsigned int body_start;
  char *body; // declare body as pointer to first element of char[]
};

// proxy one request, opening and closing one egress connection
int handle_incoming_request(client_connection &client) {
  char buffer[MAXDATASIZE];
  http_request_heading heading;
  http_response response;
  unsigned int bytes_in_first_packet;

  if (read_request(client.client_socket, buffer) <= 0) {
    return -1;
  }

  heading = parse_http_request_header(buffer);

  if (heading.method != "GET") {
    response.status_code = "400 Bad Request";
    char content[] = "Proxy only supports GET requests.\n";
    response.body = content;
    response.content_length = sizeof content;
    http_header content_type{"Content-Type", "text/plain"};
    response.headers.push_back(content_type);
    return send_response(client.client_socket, response);
  }

  // Assume hostname is provided in Host header
  client.open_server_socket = open_client_socket(heading.hostname);

  // forward request, assume GET request
  send_string(client.open_server_socket,
              get_http_request_headers_string(heading));

  // handle first packet
  if (await_response(client.open_server_socket) == 0) {
    if (log_level <= WARNING)
      std::cout << "OUT(socket " << client.client_socket
                << ") upstream server timed out" << std::endl;
    return -1;
    // it's a trick, send no reply
  }

  bytes_in_first_packet = read_request(client.open_server_socket, buffer);
  if (!bytes_in_first_packet) {
    if (log_level <= WARNING)
      std::cout << "OUT(socket " << client.client_socket
                << ") upstream server sent an empty reply" << std::endl;
    return -1;
  }

  // assume headers are all in the first packet
  response = parse_http_response_header(buffer);

  response.body = new char[response.content_length]();
  // copy body from first packet
  memcpy(response.body, &buffer[response.body_start],
         bytes_in_first_packet - response.body_start);
  // read bytes from following packets
  if (read_response_body(client, response,
                         bytes_in_first_packet - response.body_start) == -1) {
    if (log_level <= WARNING)
      std::cout << "OUT(socket " << client.client_socket
                << ") error receiving packets from upstream server"
                << std::endl;
    response.status_code = "500 Bad Request";
    char content[] = "Error receiving packets from upstream server\n";
    response.body = content;
    response.content_length = sizeof content;
    http_header content_type{"Content-Type", "text/plain"};
    response.headers.push_back(content_type);
    return send_response(client.client_socket, response);
  }

  manipulate_response(response);

  send_response(client.client_socket, response);

  if (log_level <= INFO)
    std::cout << "IN (socket " << client.client_socket << ") " << heading.method
              << " " << heading.path << std::endl;

  delete[] response.body;

  close(client.open_server_socket);

  return 0;
}

// build a string containing an HTTP request
std::string get_http_request_headers_string(http_request_heading heading) {
  std::string request{"GET " + heading.path + " HTTP/1.1\r\n"};
  for (http_header header : heading.headers)
    request += header.name + ": " + header.value + "\r\n";

  request += "\r\n\r\n";

  return request;
}

// convert a string to bytes and send them through a socket
int send_string(const int socket, std::string const &string) {
  int result;

  // convert string to char[]
  char *content = new char[string.length()];
  strcpy(content,
         string.c_str()); // vulnerable if null byte in string

  if (log_level <= DEBUG)
    std::cout << "OUT (socket " << socket << ") sending payload :\n" << content;

  result = send_bytes(socket, content, string.length());

  delete[] content; // free memory

  return result;
}

// send an HTTP response through a socket
int send_response(const int socket, http_response const &response) {
  int result;
  std::string headers = build_http_response_headers_string(response);

  char *content = new char[headers.length() + response.content_length];

  // copy headers to buffer
  strcpy(content,
         headers.c_str()); // vulnerable if null byte in string

  // not optimized
  memcpy(&content[headers.length()], response.body, response.content_length);

  if (log_level <= DEBUG)
    std::cout << "OUT (socket " << socket << ") sending headers :\n"
              << headers << std::endl;

  result =
      send_bytes(socket, content, headers.length() + response.content_length);

  delete[] content; // free memory

  return result;
}

// send bytes through a socket
int send_bytes(const int socket, const char bytes[],
               const unsigned long int length) {
  int result;
  int packet_payload_length;

  // Send packets of up to 1500 bytes
  int long unsigned index{0};
  while (index < length and result != -1) {
    packet_payload_length = length - index > 1500 ? 1500 : length - index;

    if (log_level <= DEBUG)
      std::cout << "OUT (socket " << socket << ") sending packet :\n"
                << "index = " << index
                << ", packet_payload_length = " << packet_payload_length
                << "\n";

    result = send(socket, &bytes[index], packet_payload_length, 0);

    index += packet_payload_length;
  }

  if (result == -1 and log_level <= ERROR)
    perror("send");

  return result;
}

// parse HTTP response headers from a buffer
http_response parse_http_response_header(const char buffer[MAXDATASIZE]) {
  int index{0};
  int line{0};
  int word{0};
  int start_of_word{0};
  http_response response;
  response.keep_alive = false;
  http_header current_header;
  static const http_header empty_header;
  bool done{false};

  while (!done and buffer[index] != '\0' and index < MAXDATASIZE) {
    if (log_level <= DEBUG)
      std::cout << buffer[index];
    if (buffer[index] == ' ') {
      // end of a word

      if (word == 0) {
        word++;
        start_of_word = index + 1;
      }

    } else if (buffer[index] == ':' && current_header.name == "") {
      current_header.name =
          std::string(buffer, start_of_word, index - start_of_word);
      to_lowercase(current_header.name);
    } else if (line >= 1 && current_header.name != "" &&
               buffer[index] == '\r') {
      current_header.value =
          std::string(buffer, start_of_word, index - start_of_word);

      if (current_header.name == "connection") {
        to_lowercase(current_header.value);
        response.keep_alive = current_header.value == "keep-alive";
      }
      if (current_header.name == "content-length")
        response.content_length = stoi(current_header.value);
      response.headers.push_back(current_header);
      current_header = empty_header;

    } else if (buffer[index] == '\n') {
      if (line == 0 && word == 1) {
        response.status_code =
            std::string(buffer, start_of_word, index - start_of_word);
      }

      if (word == 0 and line > 0) {
        // empty line => body starts on next line
        done = true;
        response.body_start = index + 1;
      }
      word = 0;
      line++;
      start_of_word = index + 1;
      current_header.name = "";
    }
    index++;
  }

  if (log_level <= DEBUG) {
    std::cout << "Extracted status : " << response.status_code << "\n"
              << "Keep-alive       : " << response.keep_alive << "\n"
              << "Body start       : " << response.body_start << "\n"
              << "Content-length   : " << response.content_length << "\n";
    for (http_header header : response.headers) {
      std::cout << "Extracted header : " << header.name << ": " << header.value
                << "\n";
    }
  }

  std::cout << std::flush;

  return response;
}

// read response body to pre-existing buffer of correct size
int read_response_body(client_connection const &client, http_response &response,
                       unsigned long int index) {
  // assumes response.body already initialized
  unsigned int packet_payload_length{0};

  while (index < response.content_length) {
    if (await_response(client.open_server_socket) == 0) {
      return -1;
    }

    packet_payload_length =
        read_request(client.open_server_socket, response.body + index);

    if (packet_payload_length == 0) {
      // error
      return -1;
    }

    index += packet_payload_length;
  }

  return index;
}

// build a string containing an HTTP reponse
std::string build_http_response_headers_string(http_response const &response) {
  std::string header_string{"HTTP/1.1 " + response.status_code + "\r\n"};
  for (http_header header : response.headers) {
    if (header.name != "content-length")
      // content-length header may not have been updated
      header_string += header.name + ": " + header.value + "\r\n";
  }
  header_string +=
      "Content-Length: " + std::to_string(response.content_length) + "\r\n";

  header_string += "\r\n";

  return header_string;
}

// replace all occurences of a string by a string in another string (in place)
void replace_string_in_place(std::string &string, const std::string &search,
                             const std::string &replace) {
  size_t position{0};
  while ((position = string.find(search, position)) != std::string::npos) {
    string.replace(position, search.length(), replace);
    position += replace.length();
  }
}

// edit response body and headers according to assignment directives
void manipulate_response(http_response &response) {
  bool is_text;
  for (http_header header : response.headers) {
    if (header.name == "content-type" and header.value.find("text") == 0)
      is_text = true;
  }

  if (!is_text)
    return;

  std::string body_string = std::string(response.body, response.content_length);

  // not perfectly optimized but ok
  replace_string_in_place(body_string, "Smiley", "Trolly");
  replace_string_in_place(body_string, "Stockholm", "Linköping");
  // kinda hacky, avoids replacing Stockholm in links
  replace_string_in_place(body_string, "/Linköping", "/Stockholm");
  replace_string_in_place(body_string, "smiley.jpg", "trolly.jpg");

  delete[] response.body;
  response.body = new char[body_string.length()];
  strcpy(response.body,
         body_string.c_str()); // vulnerable if null byte in string

  response.content_length = body_string.length();
}
