#include "src/cli/error.h"
#include <any>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

typedef std::vector<std::string> OptValues;
typedef std::string OptName;
typedef char ShortOptName;

class IToken {
public:
  OptName name;
  std::string help;

  IToken(){};
  IToken(std::string name, std::string help) : name(name), help(help){};

  bool operator==(const IToken &x) const = default;
  IToken &operator=(const IToken &x) = default;

  virtual bool isValueRequired() { return true; };

  virtual bool isMultiple() { return false; };

  virtual std::any getDefault() { return std::any(); };
};

class IOption : public IToken {
public:
  std::optional<ShortOptName> short_name;
  std::string group;

  IOption(){};
  IOption &operator=(const IOption &x) = default;
  IOption(std::string name, std::string help,
          std::optional<ShortOptName> short_name, std::string group)
      : IToken(name, help), short_name(short_name), group(group){};

  bool isValueRequired() { return true; };
  bool isMultiple() { return false; };
};

// ----------------NonValueOption-----------------
class INonValueOption : public IOption {
public:
  INonValueOption(OptName name, std::string help,
                  std::optional<ShortOptName> short_name, std::string group)
      : IOption(help, name, short_name, group){};

  virtual std::any feed(std::any &v) { return std::any(); };
  bool isValueRequired() { return false; };
  inline std::any getDefault() { return std::any(); };
};

class FlagOption : public INonValueOption {
  const std::optional<bool> default_value = false;

public:
  FlagOption(std::string help, OptName name,
             std::optional<ShortOptName> short_name, std::string group)
      : INonValueOption(help, name, short_name, group){};

  std::any feed(std::any &v) { return std::any(true); };
  bool isValueRequired() { return false; };
  bool isMultiple() { return false; };
  inline std::any getDefault() { return false; };
};

class CounterOption : public INonValueOption {
public:
  std::optional<int> max;

  CounterOption(std::string help, OptName name,
                std::optional<ShortOptName> short_name, std::string group,
                std::optional<int> max)
      : INonValueOption(help, name, short_name, group), max(max){};

  std::any feed(std::any &v) {
    if (v.has_value()) {
      int cnt = std::any_cast<int>(v);
      if (cnt == max) {
        return cnt;
      } else {
        return cnt + 1;
      }
    } else {
      v = std::any(1);
    }
    return v;
  }
  bool isMultiple() { return true; };
  inline std::any getDefault() { return (int)0; };
};

// ----------------ValueOption-----------------
class IValueOption : public IOption {
public:
  IValueOption(std::string help, std::string name,
               std::optional<ShortOptName> short_name, std::string group)
      : IOption(name, help, short_name, group){};

  bool isValueRequired() { return true; };
  inline bool isMultiple() { return false; };
  virtual inline std::any getDefault() {
    throw AbstractClassError();
    return std::any();
  }
  virtual inline std::any cast(std::string origin, std::string input) {
    return input;
  }
};

class StringOption : public IValueOption {
public:
  typedef std::string ValueType;

  std::optional<ValueType> default_value;
  std::optional<std::function<ValueType()>> default_factory;

  StringOption(std::string help, OptName name,
               std::optional<ShortOptName> short_name, std::string group,
               ValueType default_value)
      : IValueOption(help, name, short_name, group),
        default_value(default_value){};

  StringOption(std::string help, OptName name,
               std::optional<ShortOptName> short_name, std::string group,
               std::function<ValueType()> default_factory)
      : IValueOption(help, name, short_name, group),
        default_factory(default_factory){};

  bool isMultiple() { return false; };
  inline std::any getDefault() {
    if (this->default_value.has_value()) {
      return this->default_value.value();
    } else {
      return this->default_factory.value()();
    }
  }
};

// ----------------Argument-----------------
class Argument : public IToken {
public:
  Argument(){};
  Argument(std::string name, std::string help) : IToken(name, help){};
};