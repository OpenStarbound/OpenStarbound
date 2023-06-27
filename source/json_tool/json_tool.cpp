#include "StarFile.hpp"
#include "json_tool.hpp"
#include "editor_gui.hpp"

// Tool for scripting and mass-editing of JSON+Comments files without affecting
// formatting.

using namespace Star;

FormattedJson GenericInputFormat::toJson(String const& input) const {
  return FormattedJson::parse(input);
}

String GenericInputFormat::fromJson(FormattedJson const& json) const {
  return json.repr();
}

FormattedJson GenericInputFormat::getDefault() const {
  return FormattedJson::ofType(Json::Type::Null);
}

FormattedJson CommaSeparatedStrings::toJson(String const& input) const {
  if (input.trim() == "")
    return FormattedJson::ofType(Json::Type::Array);
  StringList strings = input.split(',');
  JsonArray array = strings.transformed(bind(&String::trim, _1, "")).transformed(construct<Json>());
  return Json(array);
}

String CommaSeparatedStrings::fromJson(FormattedJson const& json) const {
  StringList strings = json.toJson().toArray().transformed(bind(&Json::toString, _1));
  return strings.join(", ");
}

FormattedJson CommaSeparatedStrings::getDefault() const {
  return FormattedJson::ofType(Json::Type::Array);
}

FormattedJson StringInputFormat::toJson(String const& input) const {
  return FormattedJson(Json(input));
}

String StringInputFormat::fromJson(FormattedJson const& json) const {
  return json.toJson().toString();
}

FormattedJson StringInputFormat::getDefault() const {
  return FormattedJson(Json(""));
}

String Star::reprWithLineEnding(FormattedJson const& json) {
  // Append a newline, preserving the newline style of the file, e.g windows or
  // unix.
  String repr = json.repr();
  if (repr.contains("\r"))
    return strf("{}\r\n", repr);
  return strf("{}\n", repr);
}

void OutputOnSeparateLines::out(FormattedJson const& json) {
  coutf("{}", reprWithLineEnding(json));
}

void OutputOnSeparateLines::flush() {}

void ArrayOutput::out(FormattedJson const& json) {
  if (!m_unique || !m_results.contains(json))
    m_results.append(json);
}

void ArrayOutput::flush() {
  FormattedJson array = FormattedJson::ofType(Json::Type::Array);
  for (FormattedJson const& result : m_results) {
    array = array.append(result);
  }
  coutf("{}", reprWithLineEnding(array));
}

FormattedJson Star::addOrSet(bool add,
    JsonPath::PathPtr path,
    FormattedJson const& input,
    InsertLocation insertLocation,
    FormattedJson const& value) {
  JsonPath::EmptyPathOp<FormattedJson> emptyPathOp = [&value, add](FormattedJson const& document) {
    if (!add || document.type() == Json::Type::Null)
      return value;
    throw JsonException("Cannot add a value to the entire document, it is not empty.");
  };
  JsonPath::ObjectOp<FormattedJson> objectOp = [&value, &insertLocation](
      FormattedJson const& object, String const& key) {
    if (insertLocation.is<AtBeginning>())
      return object.prepend(key, value);
    if (insertLocation.is<AtEnd>())
      return object.append(key, value);
    if (insertLocation.is<BeforeKey>())
      return object.insertBefore(key, value, insertLocation.get<BeforeKey>().key);
    if (insertLocation.is<AfterKey>())
      return object.insertAfter(key, value, insertLocation.get<AfterKey>().key);
    return object.set(key, value);
  };
  JsonPath::ArrayOp<FormattedJson> arrayOp = [&value, add](FormattedJson const& array, Maybe<size_t> i) {
    if (i.isValid()) {
      if (add)
        return array.insert(*i, value);
      return array.set(*i, value);
    }
    return array.append(value);
  };
  return path->apply(input, emptyPathOp, objectOp, arrayOp);
}

void forEachFileRecursive(String const& directory, function<void(String)> func) {
  for (pair<String, bool> entry : File::dirList(directory)) {
    String filename = File::relativeTo(directory, entry.first);
    if (entry.second)
      forEachFileRecursive(filename, func);
    else
      func(filename);
  }
}

StringList Star::findFiles(FindInput const& findArgs) {
  StringList matches;
  forEachFileRecursive(findArgs.directory,
      [&findArgs, &matches](String const& filename) {
        if (filename.endsWith(findArgs.filenameSuffix))
          matches.append(filename);
      });
  return matches;
}

void forEachChild(FormattedJson const& parent, function<void(FormattedJson const&)> func) {
  if (parent.isType(Json::Type::Object)) {
    for (String const& key : parent.toJson().toObject().keys()) {
      func(parent.get(key));
    }
  } else if (parent.isType(Json::Type::Array)) {
    for (size_t i = 0; i < parent.size(); ++i) {
      func(parent.get(i));
    }
  } else {
    throw JsonPath::TraversalException::format(
        "Cannot get the children of Json type {}, must be either Array or Object", parent.typeName());
  }
}

bool process(function<void(FormattedJson const&)> output,
    Command const& command,
    Options const& options,
    FormattedJson const& input) {
  if (command.is<GetCommand>()) {
    GetCommand const& getCmd = command.get<GetCommand>();
    try {
      FormattedJson value = getCmd.path->get(input);
      if (getCmd.children) {
        forEachChild(value, output);
      } else {
        output(value);
      }
    } catch (JsonPath::TraversalException const& e) {
      if (!getCmd.opt)
        throw e;
    }

  } else if (command.is<SetCommand>()) {
    SetCommand const& setCmd = command.get<SetCommand>();
    output(addOrSet(false, setCmd.path, input, options.insertLocation, setCmd.value));

  } else if (command.is<AddCommand>()) {
    AddCommand const& addCmd = command.get<AddCommand>();
    output(addOrSet(true, addCmd.path, input, options.insertLocation, addCmd.value));

  } else if (command.is<RemoveCommand>()) {
    output(command.get<RemoveCommand>().path->remove(input));

  } else {
    starAssert(command.empty());
    output(input);
  }
  return true;
}

bool process(
    function<void(FormattedJson const&)> output, Command const& command, Options const& options, String const& input) {
  FormattedJson inJson = FormattedJson::parse(input);
  return process(output, command, options, inJson);
}

bool process(
    function<void(FormattedJson const&)> output, Command const& command, Options const& options, Input const& input) {
  if (input.is<JsonLiteralInput>()) {
    return process(output, command, options, input.get<JsonLiteralInput>().json);
  }

  bool success = true;
  StringList files;

  if (input.is<FileInput>()) {
    files = StringList{input.get<FileInput>().filename};
  } else {
    files = findFiles(input.get<FindInput>());
  }

  for (String const& file : files) {
    if (options.inPlace) {
      output = [&file](FormattedJson const& json) { File::writeFile(reprWithLineEnding(json), file); };
    }
    success &= process(output, command, options, File::readFileString(file));
  }
  return success;
}

JsonPath::PathPtr parsePath(String const& path) {
  if (path.beginsWith("/"))
    return make_shared<JsonPath::Pointer>(path);
  return make_shared<JsonPath::QueryPath>(path);
}

pair<JsonPath::PathPtr, bool> parseGetPath(String path) {
  // --get and --opt have a special syntax for getting the child values of
  // the value at the given path. These end with *, e.g.:
  //    /foo/bar/*
  //    foo.bar.*
  //    foo.bar[*]

  bool children = false;
  if (path.endsWith("/*") || path.endsWith(".*")) {
    path = path.substr(0, path.size() - 2);
    children = true;
  } else if (path.endsWith("[*]")) {
    path = path.substr(0, path.size() - 3);
    children = true;
  }
  return make_pair(parsePath(path), children);
}

Maybe<ParsedArgs> parseArgs(int argc, char** argv) {
  // Skip the program name
  Deque<String> args = Deque<String>(argv + 1, argv + argc);

  ParsedArgs parsed;

  // Parse option arguments
  while (!args.empty()) {
    String const& arg = args.takeFirst();
    if (arg == "--get" || arg == "--opt") {
      // Retrieve values at a given path in the Json document
      if (!parsed.command.empty() || args.empty())
        return {};
      JsonPath::PathPtr path;
      bool children = false;
      tie(path, children) = parseGetPath(args.takeFirst());
      parsed.command = GetCommand{path, arg == "--opt", children};

    } else if (arg == "--set") {
      // Set the value at the given path in the Json document
      if (!parsed.command.empty() || args.size() < 2)
        return {};
      JsonPath::PathPtr path = parsePath(args.takeFirst());
      FormattedJson value = FormattedJson::parse(args.takeFirst());
      parsed.command = SetCommand{path, value};

    } else if (arg == "--add") {
      // Add (insert) a path to a Json document
      if (!parsed.command.empty() || args.size() < 2)
        return {};
      JsonPath::PathPtr path = parsePath(args.takeFirst());
      FormattedJson value = FormattedJson::parse(args.takeFirst());
      parsed.command = AddCommand{path, value};

    } else if (arg == "--remove") {
      // Remove a path from a Json document
      if (!parsed.command.empty() || args.empty())
        return {};
      parsed.command = RemoveCommand{parsePath(args.takeFirst())};

    } else if (arg == "--edit") {
      // Interactive bullk Json editor
      if (!parsed.command.empty() || args.empty())
        return {};
      parsed.command = EditCommand{parsePath(args.takeFirst())};

    } else if (arg == "--editor-image") {
      if (args.empty())
        return {};
      parsed.options.editorImages.append(parsePath(args.takeFirst()));

    } else if (arg == "--input") {
      // Configure the input syntax for --edit
      if (args.empty())
        return {};
      String format = args.takeFirst();
      if (format == "json" || format == "generic")
        parsed.options.editFormat = make_shared<GenericInputFormat>();
      else if (format == "css" || format == "csv")
        parsed.options.editFormat = make_shared<CommaSeparatedStrings>();
      else if (format == "string")
        parsed.options.editFormat = make_shared<StringInputFormat>();
      else
        return {};

    } else if (arg == "--array") {
      // Output multiple results as a single array
      parsed.options.output = make_shared<ArrayOutput>(false);

    } else if (arg == "--array-unique") {
      // Output multiple results as a single array, with duplicate results
      // removed
      parsed.options.output = make_shared<ArrayOutput>(true);

    } else if (arg == "--help") {
      return {};

    } else if (arg == "-j") {
      // Use command line argument as input
      parsed.inputs.append(JsonLiteralInput{args.takeFirst()});

    } else if (arg == "--find") {
      // Search for files recursively in the given directory with a given
      // suffix.
      if (args.size() < 2)
        return {};
      String directory = args.takeFirst();
      String suffix = args.takeFirst();
      parsed.inputs.append(FindInput{directory, suffix});

    } else if (arg == "-i") {
      // Update files in place rather than print to stdout
      parsed.options.inPlace = true;

    } else if (arg == "--at") {
      // Insert new object keys at the beginning or end of the document
      if (!parsed.options.insertLocation.empty() || args.size() < 1)
        return {};
      String pos = args.takeFirst();
      if (pos == "beginning" || pos == "start")
        parsed.options.insertLocation = AtBeginning{};
      else if (pos == "end")
        parsed.options.insertLocation = AtEnd{};
      else
        return {};

    } else if (arg == "--before") {
      // Insert new object keys before the given key
      if (!parsed.options.insertLocation.empty() || args.size() < 1)
        return {};
      parsed.options.insertLocation = BeforeKey{args.takeFirst()};

    } else if (arg == "--after") {
      // Insert new object keys after the given key
      if (!parsed.options.insertLocation.empty() || args.size() < 1)
        return {};
      parsed.options.insertLocation = AfterKey{args.takeFirst()};

    } else {
      if (!File::exists(arg)) {
        cerrf("File {} doesn't exist\n", arg);
        return {};
      }
      parsed.inputs.append(FileInput{arg});
    }
  }

  if (!parsed.options.output)
    parsed.options.output = make_shared<OutputOnSeparateLines>();

  bool anyFileInputs = false, anyNonFileInputs = false;
  for (Input const& input : parsed.inputs) {
    if (input.is<FindInput>() || input.is<FileInput>()) {
      anyFileInputs = true;
    } else {
      anyNonFileInputs = true;
    }
  }
  if (parsed.command.is<EditCommand>() && !anyFileInputs) {
    cerrf("Files to edit must be supplied when using --edit.\n");
    return {};
  }

  if (parsed.options.inPlace && !anyFileInputs) {
    cerrf("In-place writing (-i) can only be used with files specified on the command line.\n");
    return {};
  }
  if (parsed.options.inPlace && parsed.command.is<EditCommand>()) {
    cerrf("Interactive edit (--edit) is always in-place. Explicitly specifying -i is not needed.\n");
    return {};
  }
  if (parsed.command.is<EditCommand>() && anyNonFileInputs) {
    cerrf("Interactie edit (--edit) can only be used with file input sources.\n");
    return {};
  }

  if (parsed.options.editFormat && !parsed.command.is<EditCommand>()) {
    cerrf("--input can only be used with --edit.\n");
    return {};
  } else if (!parsed.options.editFormat && parsed.command.is<EditCommand>()) {
    parsed.options.editFormat = make_shared<GenericInputFormat>();
  }

  return parsed;
}

String readStdin() {
  String result;
  char buffer[1024];
  while (!feof(stdin)) {
    size_t readBytes = fread(buffer, 1, 1024, stdin);
    result.append(buffer, readBytes);
  }
  return result;
}

int main(int argc, char** argv) {
  try {
    Maybe<ParsedArgs> parsedArgs = parseArgs(argc, argv);

    if (!parsedArgs) {
      cerrf("Usage: {} [--get <json-path>] (-j <json> | <json-file>*)\n", argv[0]);
      cerrf(
          "Usage: {} --set <json-path> <json> [-i] [(--at (beginning|end) | --before <key> | --after <key>)] (-j "
          "<json> | <json-file>*)\n",
          argv[0]);
      cerrf(
          "Usage: {} --add <json-path> <json> [-i] [(--at (beginning|end) | --before <key> | --after <key>)] (-j "
          "<json> | <json-file>*)\n",
          argv[0]);
      cerrf(
          "Usage: {} --edit <json-path> [(--at (beginning|end) | --before <key> | --after <key>)] [--input "
          "(csv|json|string)] <json-file>+\n",
          argv[0]);
      cerrf("\n");
      cerrf("Example: {} --get /dialog/0/message guard.npctype\n", argv[0]);
      cerrf("Example: {} --get 'foo[0]' -j '{\"foo\":[0,1,2,3]}'\n", argv[0]);
      cerrf("Example: {} --edit /tags --input csv --find ../assets/ .object\n", argv[0]);
      return 1;
    }

    OutputPtr output = parsedArgs->options.output;
    bool success = true;

    if (parsedArgs->command.is<EditCommand>()) {
      return edit(argc, argv, parsedArgs->command.get<EditCommand>().path, parsedArgs->options, parsedArgs->inputs);

    } else if (parsedArgs->inputs.size() == 0) {
      // No files were provided. Reading from stdin
      success &= process(output->toFunction(), parsedArgs->command, parsedArgs->options, readStdin());
    } else {
      for (Input const& input : parsedArgs->inputs) {
        success &= process(output->toFunction(), parsedArgs->command, parsedArgs->options, input);
      }
    }

    output->flush();

    if (!success)
      return 1;
    return 0;

  } catch (JsonParsingException const& e) {
    cerrf("{}\n", e.what());
    return 1;

  } catch (JsonException const& e) {
    cerrf("{}\n", e.what());
    return 1;

  } catch (std::exception const& e) {
    cerrf("Exception caught: {}\n", outputException(e, true));
    return 1;
  }
}
