#include "StarOptionParser.hpp"
#include "StarIterator.hpp"

namespace Star {

void OptionParser::setCommandName(String commandName) {
  m_commandName = move(commandName);
}

void OptionParser::setSummary(String summary) {
  m_summary = move(summary);
}

void OptionParser::setAdditionalHelp(String help) {
  m_additionalHelp = move(help);
}

void OptionParser::addSwitch(String const& flag, String description) {
  if (!m_options.insert(flag, Switch{flag, move(description)}).second)
    throw OptionParserException::format("Duplicate switch '-%s' added", flag);
}

void OptionParser::addParameter(String const& flag, String argument, RequirementMode requirementMode, String description) {
  if (!m_options.insert(flag, Parameter{flag, move(argument), requirementMode, move(description)}).second)
    throw OptionParserException::format("Duplicate flag '-%s' added", flag);
}

void OptionParser::addArgument(String argument, RequirementMode requirementMode, String description) {
  m_arguments.append(Argument{move(argument), requirementMode, move(description)});
}

pair<OptionParser::Options, StringList> OptionParser::parseOptions(StringList const& arguments) const {
  Options result;
  StringList errors;
  bool endOfFlags = false;

  auto it = makeSIterator(arguments);
  while (it.hasNext()) {
    auto const& arg = it.next();
    if (arg == "--") {
      endOfFlags = true;
      continue;
    }

    if (!endOfFlags && arg.beginsWith("-")) {
      String flag = arg.substr(1);
      auto option = m_options.maybe(flag);
      if (!option) {
        errors.append(strf("No such option '-%s'", flag));
        continue;
      }

      if (option->is<Switch>()) {
        result.switches.add(move(flag));
      } else {
        auto const& parameter = option->get<Parameter>();
        if (!it.hasNext()) {
          errors.append(strf("Option '-%s' must be followed by an argument", flag));
          continue;
        }
        String val = it.next();
        if (parameter.requirementMode != Multiple && result.parameters.contains(flag)) {
          errors.append(strf("Option with argument '-%s' specified multiple times", flag));
          continue;
        }
        result.parameters[move(flag)].append(move(val));
      }

    } else {
      result.arguments.append(move(arg));
    }
  }

  for (auto const& pair : m_options) {
    if (pair.second.is<Parameter>()) {
      auto const& na = pair.second.get<Parameter>();
      if (na.requirementMode == Required && !result.parameters.contains(pair.first))
        errors.append(strf("Missing required flag with argument '-%s'", pair.first));
    }
  }

  size_t minimumArguments = 0;
  size_t maximumArguments = 0;
  for (auto const& argument : m_arguments) {
    if ((argument.requirementMode == Optional || argument.requirementMode == Required) && maximumArguments != NPos)
      ++maximumArguments;
    if (argument.requirementMode == Required)
      ++minimumArguments;
    if (argument.requirementMode == Multiple)
      maximumArguments = NPos;
  }
  if (result.arguments.size() < minimumArguments)
    errors.append(strf(
        "Too few positional arguments given, expected at least %s got %s", minimumArguments, result.arguments.size()));
  if (result.arguments.size() > maximumArguments)
    errors.append(strf(
        "Too many positional arguments given, expected at most %s got %s", maximumArguments, result.arguments.size()));

  return {move(result), move(errors)};
}

void OptionParser::printHelp(std::ostream& os) const {
  if (!m_commandName.empty() && !m_summary.empty())
    format(os, "%s: %s\n\n", m_commandName, m_summary);
  else if (!m_commandName.empty())
    format(os, "%s:\n\n", m_commandName);
  else if (!m_summary.empty())
    format(os, "%s\n\n", m_summary);

  String cmdLineText;

  for (auto const& p : m_options) {
    if (p.second.is<Switch>()) {
      cmdLineText += strf(" [-%s]", p.first);
    } else {
      auto const& parameter = p.second.get<Parameter>();
      if (parameter.requirementMode == Optional)
        cmdLineText += strf(" [-%s <%s>]", parameter.flag, parameter.argument);
      else if (parameter.requirementMode == Required)
        cmdLineText += strf(" -%s <%s>", parameter.flag, parameter.argument);
      else if (parameter.requirementMode == Multiple)
        cmdLineText += strf(" [-%s <%s>]...", parameter.flag, parameter.argument);
    }
  }

  for (auto const& p : m_arguments) {
    if (p.requirementMode == Optional)
      cmdLineText += strf(" [<%s>]", p.argumentName);
    else if (p.requirementMode == Required)
      cmdLineText += strf(" <%s>", p.argumentName);
    else
      cmdLineText += strf(" [<%s>...]", p.argumentName);
  }

  if (m_commandName.empty())
    format(os, "Command Line Usage:%s\n", cmdLineText);
  else
    format(os, "Command Line Usage: %s%s\n", m_commandName, cmdLineText);

  for (auto const& p : m_options) {
    if (p.second.is<Switch>()) {
      auto const& sw = p.second.get<Switch>();
      if (!sw.description.empty())
        format(os, "  -%s\t- %s\n", sw.flag, sw.description);
    }
    if (p.second.is<Parameter>()) {
      auto const& parameter = p.second.get<Parameter>();
      if (!parameter.description.empty())
        format(os, "  -%s <%s>\t- %s\n", parameter.flag, parameter.argument, parameter.description);
    }
  }

  for (auto const& p : m_arguments) {
    if (!p.description.empty())
      format(os, "  <%s>\t- %s\n", p.argumentName, p.description);
  }

  if (!m_additionalHelp.empty())
    format(os, "\n%s\n", m_additionalHelp);
}

}
