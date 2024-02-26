#pragma once

#include "StarString.hpp"
#include "StarVariant.hpp"
#include "StarOrderedMap.hpp"
#include "StarOrderedSet.hpp"

namespace Star {

STAR_EXCEPTION(OptionParserException, StarException);

// Simple command line argument parsing and help printing, only simple single
// dash flags are supported, no flag combining is allowed and all components
// must be separated by a space.
//
// A 'flag' here refers to a component that is preceded by a dash, like -f or
// -quiet.
//
// Three kinds of things are parsed:
// - 'switches' which are flags that do not have a value, like `-q` for quiet
// - 'parameters' are flags with a value that follows, like `-mode full`
// - 'arguments' are everything else, sorted positionally
class OptionParser {
public:
  enum RequirementMode {
    Optional,
    Required,
    Multiple
  };

  struct Options {
    OrderedSet<String> switches;
    StringMap<StringList> parameters;
    StringList arguments;
  };

  void setCommandName(String commandName);
  void setSummary(String summary);
  void setAdditionalHelp(String help);

  void addSwitch(String const& flag, String description = {});
  void addParameter(String const& flag, String argumentName, RequirementMode mode, String description = {});
  void addArgument(String argumentName, RequirementMode mode, String description = {});

  // Parse the given arguments into an options set, returns the options parsed
  // and a list of all the errors encountered while parsing.
  pair<Options, StringList> parseOptions(StringList const& arguments) const;

  // Print help text to the given std::ostream
  void printHelp(std::ostream& os) const;

private:
  struct Switch {
    String flag;
    String description;
  };

  struct Parameter {
    String flag;
    String argument;
    RequirementMode requirementMode;
    String description;
  };

  struct Argument {
    String argumentName;
    RequirementMode requirementMode;
    String description;
  };

  typedef Variant<Switch, Parameter> Option;

  String m_commandName;
  String m_summary;
  String m_additionalHelp;

  OrderedHashMap<String, Option> m_options;
  List<Argument> m_arguments;
};

}
