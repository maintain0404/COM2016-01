#ifndef __CLI_ERROR__H__
#define __CLI_ERROR__H__
#include <exception>

class CliError : public std::exception {
  virtual const char *what() const throw() {
    return "Parser Exception happend";
  }
};

class AbstractClassError : public CliError {
  const char *what() const throw() { return "Need Implementation"; }
};

class OptionDeclarationError : public CliError {};

class DefaultDuplicateError : public OptionDeclarationError {};

class ParserError : public CliError {};

class ParserDeclationError : public ParserError {
  const char *what() const throw() { return "Option Duplication"; }
};

class ParseAlreadyFinishedError : public ParserError {};

class OptionDuplicateError : public ParserDeclationError {};

class InvalidOptionError : public ParserError {};

class OptionNotExistsError : public ParserError {};
#endif
