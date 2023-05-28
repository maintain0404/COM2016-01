#include "src/cli/parser.h"
#include "gtest/gtest.h"

TEST(TEST_PARSER, ADD_OPTION) {
  Parser p;
  std::string optname = "flag";
  char shortname = 'f';
  FlagOption flag("help text", optname, shortname, "GLOBAL");

  try {
    p.addOption(&flag);
  } catch (std::exception &ex) {
    FAIL() << "Parser Error";
  }

  EXPECT_EQ(p.getOptionWithLong(optname), &flag);
  EXPECT_EQ(p.getOptionWithShort(shortname), &flag);

  EXPECT_THROW(
      {
        try {
          p.addOption(&flag);
        } catch (...) {
          throw;
        }
      },
      OptionDuplicateError);
};

TEST(TEST_PARSER, FLAG_NOT_SET) {
  Parser p;
  std::string optname = "flag";
  char shortname = 'f';
  FlagOption flag("help text", optname, shortname, "GLOBAL");
  const char *command[] = {"cmd", "hello"};

  p.addOption(&flag);

  try {
    p.run(2, (char **)command);
  } catch (std::exception &e) {
    FAIL() << e.what();
  }

  EXPECT_EQ(p.getValue(optname).has_value(), true) << "Option";
  EXPECT_EQ(std::any_cast<bool>(p.getValue(optname)), false)
      << "Check Flag Value";
}

TEST(TEST_PARSER, FLAG_SET) {
  Parser p;
  std::string optname = "flag";
  char shortname = 'f';
  FlagOption flag("help text", optname, shortname, "GLOBAL");
  const char *command[] = {"cmd", "-f", "hello"};

  p.addOption(&flag);

  try {
    p.run(3, (char **)command);
  } catch (std::exception &e) {
    FAIL() << e.what();
  }

  EXPECT_EQ(p.getValue(optname).has_value(), true) << "Option";
  EXPECT_EQ(std::any_cast<bool>(p.getValue(optname)), true)
      << "Check Flag Value";
}

TEST(TEST_PARSER, COUNTER_NOT_SET) {
  Parser p;
  std::string optname = "counter";
  char shortname = 'c';
  CounterOption counter("help text", optname, shortname, "GLOBAL", 3);
  const char *command[] = {"cmd"};

  p.addOption(&counter);

  try {
    p.run(1, (char **)command);
  } catch (std::exception &e) {
    FAIL() << e.what();
  }

  EXPECT_EQ(p.getValue(optname).has_value(), true) << "Option";
  EXPECT_EQ(std::any_cast<int>(p.getValue(optname)), 0) << "Check Flag Value";
}

TEST(TEST_PARSER, COUNTER_SET) {
  Parser p;
  std::string optname = "counter";
  char shortname = 'c';
  CounterOption counter("help text", optname, shortname, "GLOBAL", 3);
  const char *command[] = {"cmd", "--counter", "-c"};

  p.addOption(&counter);

  try {
    p.run(3, (char **)command);
  } catch (std::exception &e) {
    FAIL() << e.what();
  }

  EXPECT_EQ(p.getValue(optname).has_value(), true) << "Option";
  EXPECT_EQ(std::any_cast<int>(p.getValue(optname)), 2) << "Check Flag Value";
}

TEST(TEST_PARSER, COUNTER_SET_OVER_MAX) {
  Parser p;
  std::string optname = "counter";
  char shortname = 'c';
  CounterOption counter("help text", optname, shortname, "GLOBAL", 3);
  const char *command[] = {"cmd", "--counter", "-c", "-c", "-c"};

  p.addOption(&counter);

  try {
    p.run(5, (char **)command);
  } catch (std::exception &e) {
    FAIL() << e.what();
  }

  EXPECT_EQ(p.getValue(optname).has_value(), true) << "Option";
  EXPECT_EQ(std::any_cast<int>(p.getValue(optname)), 3) << "Check Flag Value";
}

TEST(TEST_PARSER, STRING_NOT_SET) {
  Parser p;
  std::string voptname = "string";
  char vshortname = 's';
  StringOption voption("help text", voptname, vshortname, "GLOBAL",
                       std::string("vdefault"));

  std::string foptname = "factory";
  char fshortname = 'f';
  std::function<std::string()> f = []() { return std::string("fdefault"); };
  StringOption foption("help text", foptname, fshortname, "GLOBAL", f);
  const char *command[] = {"cmd"};

  p.addOption(&voption);
  p.addOption(&foption);

  try {
    p.run(1, (char **)command);
  } catch (std::exception &e) {
    FAIL() << e.what();
  }

  EXPECT_EQ(p.getValue(voptname).has_value(), true) << "Option";
  EXPECT_EQ(p.getValue(voptname).type(), typeid(StringOption::ValueType));
  EXPECT_EQ(
      std::any_cast<StringOption::ValueType>(p.getValue(voptname)).value(),
      std::string("vdefault"));

  EXPECT_EQ(p.getValue(foptname).has_value(), true) << "Option";
  EXPECT_EQ(p.getValue(foptname).type(), typeid(StringOption::ValueType));
  EXPECT_EQ(
      std::any_cast<StringOption::ValueType>(p.getValue(foptname)).value(),
      std::string("fdefault"));
};

TEST(TEST_PARSER, STRING_SET) {
  Parser p;
  std::string optname = "string";
  char shortname = 's';
  StringOption counter("help text", optname, shortname, "GLOBAL",
                       std::string("default"));
  const char *command[] = {"cmd", "--string", "input"};

  p.addOption(&counter);

  try {
    p.run(3, (char **)command);
  } catch (std::exception &e) {
    FAIL() << e.what();
  }

  EXPECT_EQ(p.getValue(optname).has_value(), true) << "Option";
  EXPECT_EQ(p.getValue(optname).type(), typeid(StringOption::ValueType));
  EXPECT_EQ(std::any_cast<StringOption::ValueType>(p.getValue(optname)).value(),
            std::string("input"))
      << "Check Flag Value";
}