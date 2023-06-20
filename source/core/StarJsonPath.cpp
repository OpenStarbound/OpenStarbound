#include "StarJsonPath.hpp"

namespace Star {

namespace JsonPath {

  TypeHint parsePointer(String& buffer, String const& path, String::const_iterator& iterator, String::const_iterator end) {
    buffer.clear();

    if (*iterator != '/')
      throw ParsingException::format("Missing leading '/' in Json pointer \"%s\"", path);
    iterator++;

    while (iterator != end && *iterator != '/') {
      if (*iterator == '~') {
        ++iterator;
        if (iterator == end)
          throw ParsingException::format("Incomplete escape sequence in Json pointer \"%s\"", path);

        if (*iterator == '0')
          buffer.append('~');
        else if (*iterator == '1')
          buffer.append('/');
        else
          throw ParsingException::format("Invalid escape sequence in Json pointer \"%s\"", path);
        ++iterator;
      } else
        buffer.append(*iterator++);
    }

    Maybe<size_t> index = maybeLexicalCast<size_t>(buffer);
    if (index.isValid() || (buffer == "-" && iterator == end))
      return TypeHint::Array;
    return TypeHint::Object;
  }

  TypeHint parseQueryPath(String& buffer, String const& path, String::const_iterator& iterator, String::const_iterator end) {
    buffer.clear();

    if (*iterator == '.') {
      throw ParsingException::format("Entry starts with '.' in query path \"%s\"", path);

    } else if (*iterator == '[') {
      // Parse array number and ']'
      // Consume initial '['
      ++iterator;

      while (iterator != end && *iterator >= '0' && *iterator <= '9')
        buffer.append(*iterator++);

      if (iterator == end || *iterator != ']')
        throw ParsingException::format("Array has no trailing ']' or has invalid character in query path \"%s\"", path);

      // Consume trailing ']'
      ++iterator;

      // Consume trailing '.'
      if (iterator != end && *iterator == '.')
        ++iterator;

      return TypeHint::Array;

    } else {
      // Parse path up to next '.' or '['
      while (iterator != end && *iterator != '.' && *iterator != '[')
        buffer.append(*iterator++);

      // Consume single trailing '.' if it exists
      if (iterator != end && *iterator == '.')
        ++iterator;
      return TypeHint::Object;
    }
  }
}

}
