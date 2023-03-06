#include "apollo_utils.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <cerrno>

std::string GetHostname() {
  std::string hostname;
  hostname.resize(128);
  if (gethostname(hostname.data(), hostname.size()) != 0) {
    return "";
  }
  return {hostname.c_str()};
}

std::string GetIp() {
  struct addrinfo *answer, hint;
  bzero(&hint, sizeof(hint));
  hint.ai_family = AF_INET;
  hint.ai_socktype = SOCK_STREAM;

  std::string hostname = GetHostname();
  if (hostname.empty()) {
    return "";
  }

  if (getaddrinfo(hostname.c_str(), nullptr, &hint, &answer) != 0) {
    return "";
  }
  std::string ip;
  ip.resize(16);

  for (auto curr = answer; curr != nullptr; curr = curr->ai_next) {
    inet_ntop(AF_INET, &(((struct sockaddr_in *)(curr->ai_addr))->sin_addr), ip.data(), ip.size());
  }

  freeaddrinfo(answer);
  return {ip.c_str()};
}
