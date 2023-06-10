#include "src/cli/error.h"
#include "src/cli/token.h"
#include <algorithm>
#include <any>
#include <iterator>
#include <queue>
#include <unordered_map>
#include <variant>
#include <vector>

enum ParsingState {
  NONE,               // Nothing parsed
  READY,              // Finished to parsing command name
  ENTER_VALUE_OPT,    // Need value
  REQUIRED_ARGUMENTS, // Parsing arguments. Do not allow any options more.
  MULTIPLE_ARGUMENTS  // Parsing arguments.
};

enum TokenType {
  SHORT_, // Short option. -e
  LONG_,  // Long option. --example
  VALUE_, // Not option. example
};

typedef std::vector<std::string> ArgsIter;
typedef std::any OptValue;

class Parser {
private:
  std::unordered_map<ShortOptName, IOption *> _short_name_map;
  std::unordered_map<OptName, IOption *> _long_name_map;
  std::unordered_map<OptName, OptValue> _value_map;
  std::queue<std::string> arguments;
  std::string command;
  bool is_parsed;

public:
  Parser() : is_parsed(false){};

  void addOption(IOption *option) {
    if (option->short_name.has_value()) {
      auto shortiter = this->_short_name_map.find(option->short_name.value());
      if (shortiter != this->_short_name_map.end()) {
        throw OptionDuplicateError();
      };

      this->_short_name_map.insert(
          std::make_pair(option->short_name.value(), option));
    }
    auto longiter = this->_long_name_map.find(option->name);
    if (longiter != this->_long_name_map.end()) {
      throw OptionDuplicateError();
    }
    this->_long_name_map.insert(std::make_pair(option->name, option));
  }

  void run(int argc, char *argv[]) {
    if (is_parsed) {
      throw ParseAlreadyFinishedError();
    }
    std::queue<std::string> args_queue;
    ParsingState state = ParsingState::NONE;
    IOption *current_option;

    // Prepare parsing
    this->fillDefault();
    std::for_each(argv, argv + argc, [&args_queue](char *&arg) {
      args_queue.push(std::string(arg));
    });

    // Run parsing.
    try {
      while (!args_queue.empty()) {
        auto token = args_queue.front();
        args_queue.pop();

        switch (state) {
        case NONE: {
          this->command = token;
          state = ParsingState::READY;
          continue;
        }
        case READY: {
          auto [trimed_token, token_type] = trimToken(token);
          switch (token_type) {
          case SHORT_: {
            current_option = getOptionWithShort(trimed_token.c_str()[0]);
            state = handleOption(current_option);
            continue;
          }
          case LONG_: {
            current_option = getOptionWithLong(trimed_token);
            state = handleOption(current_option);
            continue;
          }
          case VALUE_: {
            arguments.push(token);
            state = ParsingState::REQUIRED_ARGUMENTS;
            continue;
          }
          }
          continue;
        }
        case ENTER_VALUE_OPT: {
          this->_value_map.find(current_option->name)->second =
              std::optional(token);
          continue;
        }
        // NOT READY TO USE
        case REQUIRED_ARGUMENTS: {
          arguments.push(token);
          continue;
        }
        // NOT READY TO USE
        case MULTIPLE_ARGUMENTS: {
          arguments.push(token);
          continue;
        }
        }
      }
    } catch (ParserError &e) {
      throw;
    }
    is_parsed = true;
  };

  OptValue getValue(OptName name) {
    auto viter = this->_value_map.find(name);
    if (viter != this->_value_map.end()) {
      return viter->second;
    }

    throw OptionNotExistsError();
  }

  std::pair<std::string, TokenType> trimToken(std::string token) {
    if (token.starts_with("--")) {
      if (token.length() < 3) {
        throw InvalidOptionError();
      }
      return std::make_pair(token.substr(2), TokenType::LONG_);
    } else if (token.starts_with("-")) {
      if (token.length() != 2) {
        throw InvalidOptionError();
      }
      return std::make_pair(token.substr(1), TokenType::SHORT_);
    } else {
      return std::make_pair(token, TokenType::VALUE_);
    }
  }

  void fillDefault() {
    for (auto iter = this->_long_name_map.begin();
         iter != this->_long_name_map.end(); iter++) {
      this->_value_map.insert(
          std::make_pair(iter->second->name, iter->second->getDefault()));
    }
  }

  ParsingState handleOption(IOption *current_option) {
    if (current_option->isValueRequired()) {
      return ParsingState::ENTER_VALUE_OPT;
    } else {
      auto viter = this->_value_map.find(current_option->name);
      viter->second = ((INonValueOption *)current_option)->feed(viter->second);
      return ParsingState::READY;
    }
  };

  IOption *getOptionWithShort(char short_name) {
    auto iter = this->_short_name_map.find(short_name);
    if (iter == _short_name_map.end()) {
      throw OptionNotExistsError();
    }
    return iter->second;
  }

  IOption *getOptionWithLong(std::string &token) {
    auto iter = this->_long_name_map.find(token);
    if (iter == _long_name_map.end()) {
      throw OptionNotExistsError();
    }
    return iter->second;
  }
};
