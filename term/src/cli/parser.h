#include "src/cli/error.h"
#include "src/cli/token.h"
#include <algorithm>
#include <any>
#include <iostream>
#include <iterator>
#include <queue>
#include <unordered_map>
#include <variant>
#include <vector>

enum ParsingState {
  NONE,            // Nothing parsed
  READY,           // Finished to parsing command name
  ENTER_VALUE_OPT, // Need value
  ARGUMENTS,       // Parsing arguments.
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
  std::vector<std::string> arg_values;
  std::vector<Argument> arguments;
  std::string command;
  bool is_parsed;

  std::string buildHelpText() {
    std::string txt = this->command;

    // Arguments example line

    for (auto aiter = this->arguments.begin(); aiter < this->arguments.end();
         aiter++) {
      txt += std::string(" [") + aiter->name + "]";
    }

    if (!this->arguments.empty()) {
      txt += "\n\nArguments:\n";

      // Arguments help lines
      for (auto aiter = this->arguments.begin(); aiter < this->arguments.end();
           aiter++) {
        // clang-format off
        txt += std::string("  ") + aiter->name +
               std::string(20 - aiter->name.size(), ' ') + 
               "(required) " +
               aiter->help + "\n";
        // clang-format on
      }
    }

    if (!this->_long_name_map.empty()) {
      txt += "\n\nOptions:\n";

      // Options lines
      for (auto oiter = this->_long_name_map.begin();
           oiter != this->_long_name_map.end(); oiter++) {
        txt += std::string("  --") + oiter->second->name +
               std::string(18 - oiter->second->name.size(), ' ') +
               oiter->second->help + "\n";
        if (oiter->second->short_name.has_value()) {
          txt += std::string("  -") + oiter->second->short_name.value() +
                 std::string(17, ' ') + " short version of option --" +
                 oiter->second->name + "\n";
        }
      }
    }
    txt += "\n";
    return txt;
  };

public:
  Parser(std::string command) : command(command), is_parsed(false){};

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

  void addArgument(Argument *arg) { this->arguments.push_back(*arg); }

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
            arg_values.push_back(token);
            state = ParsingState::ARGUMENTS;
            continue;
          }
          }
          continue;
        }
        case ENTER_VALUE_OPT: {
          this->_value_map.find(current_option->name)->second = token;
          continue;
        }
        // NOT READY TO USE
        case ARGUMENTS: {
          arg_values.push_back(token);
          continue;
        }
        }
      }
    } catch (ParserError &e) {
      std::cout << this->buildHelpText();
      // exit(-1);
    }
    is_parsed = true;

    if (this->arguments.size() != this->arg_values.size()) {
      std::cout << this->buildHelpText();
      // exit(-1);
    }
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

  std::string getArgumentValue(std::string name) {
    int cnt = 0;
    for (auto iter = this->arguments.begin(); iter < this->arguments.end();
         iter++) {
      if (iter->name == name) {
        return this->arg_values.at(cnt);
      }
      cnt++;
    }
    throw OptionNotExistsError();
  }
};
