#ifndef __PROTOCOL_PACKET_H__
#define __PROTOCOL_PACKET_H__

#include <string>
#include <sys/types.h>
#include <variant>
#include <vector>

enum MessageType { ENTER, MESSAGE, RECV_MESSAGE };

using Data = std::vector<uint8_t>;

class Header {
public:
  int version; // Always 1
  unsigned int type;
  int size;

  Header(){};
  Header(MessageType type, int size) : version(1), type(type), size(size){};
};

// send by client
struct SendEnter {
  std::string name;
};

struct SendMessage {
  std::string content;
};

enum HandleReturn {
  SHORTER_THAN_HEADER,
  INVALID_VERSION,
  INVALID_TYPE,
  INVALID_SIZE,
};

using SendPacket = std::variant<SendEnter, SendMessage>;

// received by client
class RecvMessage {
public:
  int name_size;
  std::string sender_name;
  std::string content;

  RecvMessage(){};
  RecvMessage(std::string sender_name, std::string content)
      : name_size(sender_name.size()), sender_name(sender_name),
        content(content) {}

  int size() { return sizeof(name_size) + sender_name.size() + content.size(); }
};
#endif