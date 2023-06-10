#ifndef __PROTOCOL_HANDLE_H__
#define __PROTOCOL_HANDLE_H__

#include "src/logging/logging.hpp"
#include "src/protocol/packet.hpp"
#include <arpa/inet.h>
#include <climits>
#include <cstdint>
#include <cstring>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

using Packet = std::variant<SendEnter, SendMessage>;
using RecvPacket = std::variant<RecvMessage>;

class Handle {
  Header parseHeader(Data &data, int &pos) {
    Header header;

    LOG_DEBUG("Parsing header ...");

    std::memcpy(&header, data.data() + pos, sizeof(header));
    pos += sizeof(header);

    if (data.size() < sizeof(Header) + header.size) {
      throw HandleReturn::INVALID_SIZE;
    }
    LOG_DEBUG("Parsing header finished");
    return header;
  };

  SendEnter parseSendEnter(Data &data, const Header &header, int &pos) {
    SendEnter enter;
    enter.name =
        std::string(data.data() + pos, data.data() + pos + header.size);
    pos += sizeof(header.size);
    return enter;
  }

  SendMessage parseSendMessage(Data &data, const Header &header, int &pos) {
    SendMessage msg;
    msg.content =
        std::string(data.data() + pos, data.data() + pos + header.size);
    pos += sizeof(header.size);
    return msg;
  }

  RecvMessage parseRecvMessage(Data &data, const Header &header, int &pos) {
    RecvMessage msg;

    std::memcpy(&msg.name_size, data.data() + pos, sizeof(msg.name_size));
    pos += sizeof(msg.name_size);

    if (msg.name_size <= 0) {
      throw INVALID_SIZE;
    }

    msg.sender_name =
        std::string(data.data() + pos, data.data() + pos + msg.name_size);
    pos += msg.name_size;

    msg.content = std::string(data.data() + pos,
                              data.data() + sizeof(Header) + header.size);
    pos = sizeof(Header) + header.size;
    return msg;
  };

public:
  Packet feed(std::vector<uint8_t> &buffer) {
    if (buffer.size() < sizeof(Header)) {
      throw HandleReturn::SHORTER_THAN_HEADER;
    }
    int pos = 0;

    LOG_DEBUG("Start parsing");
    Header header = parseHeader(buffer, pos);

    switch (header.type) {
    case ENTER: {
      return parseSendEnter(buffer, header, pos);
    };
    case MESSAGE: {
      return parseSendMessage(buffer, header, pos);
    };
    default: {
      throw HandleReturn::INVALID_TYPE;
    }
    }
  }

  RecvPacket parseRecv(Data &buffer) {
    if (buffer.size() < sizeof(Header)) {
      throw HandleReturn::SHORTER_THAN_HEADER;
    }
    int pos = 0;

    Header header = parseHeader(buffer, pos);

    switch (header.type) {
    case RECV_MESSAGE: {
      return parseRecvMessage(buffer, header, pos);
    }
    default: {
      throw HandleReturn::INVALID_TYPE;
    }
    }
  }

  Data buildRecvMessage(RecvMessage &msg) {
    Data data;
    int pos = 0;
    Header header(RECV_MESSAGE, msg.size());
    data.resize(sizeof(Header) + header.size);

    std::memcpy(data.data() + pos, &header, sizeof(Header));
    pos += sizeof(Header);

    std::memcpy(data.data() + pos, &(msg.name_size), sizeof(msg.name_size));
    pos += sizeof(msg.name_size);

    std::memcpy(data.data() + pos, msg.sender_name.c_str(),
                msg.sender_name.size());
    pos += msg.sender_name.size();

    std::memcpy(data.data() + pos, msg.content.c_str(), msg.content.size());
    pos += msg.content.size();

    return data;
  };
};

#endif
