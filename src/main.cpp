#include <cstdio>
#include <cstring>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "Luau/Ast.h"
#include "Luau/Location.h"
#include "Luau/ParseOptions.h"
#include "Luau/Parser.h"

static void displayHelp(const char *program_name) {
  printf("Usage: %s [file]\n", program_name);
}

static int assertionHandler(const char *expr, const char *file, int line,
                            const char *function) {
  printf("%s(%d): ASSERTION FAILED: %s\n", file, line, expr);
  return 1;
}

std::string formatLocation(const char *name, const Luau::Location &location) {
  return std::format("{}({},{})", name, location.begin.line + 1,
                     location.begin.column + 1);
}

static void report(const char *name, const Luau::Location &location,
                   const char *type, const char *message) {
  fprintf(stderr, "%s: %s: %s\n", formatLocation(name, location).c_str(), type,
          message);
}

static void reportUncachedClosure(const char *name,
                                  const Luau::Location &location,
                                  const char *message) {
  fprintf(stderr, "%s: %s: %s\n", formatLocation(name, location).c_str(),
          "UncachedClosureWarning", message);
}

struct AstVisitor : public Luau::AstVisitor {
  std::vector<Luau::AstExprFunction *> functionStack;
  const char *fileName;

  AstVisitor(const char *fileName) : fileName(fileName) {}

  bool visit(Luau::AstExprFunction *func) override {
    functionStack.push_back(func);

    func->body->visit(this);

    functionStack.pop_back();

    // prevent default traversal
    return false;
  }

  bool visit(Luau::AstExprLocal *localVariableExpression) override {
    if (localVariableExpression->upvalue &&
        localVariableExpression->local->functionDepth != 0 &&
        !functionStack.empty()) {
      Luau::AstExprFunction *currentFunction = functionStack.back();
      Luau::AstLocal *localVariable = localVariableExpression->local;

      if (currentFunction->functionDepth > localVariable->functionDepth) {
        std::string closureDetails("anonymous closure");
        if (currentFunction->debugname.value) {
          closureDetails =
              std::format("closure \"{}\"", currentFunction->debugname.value);
        }

        reportUncachedClosure(
            fileName, localVariableExpression->location,
            std::format("Usage of upvalue \"{}\" declared at {} prevents {} "
                        "at {} from being cached",
                        localVariable->name.value,
                        formatLocation(fileName, localVariable->location),
                        closureDetails,
                        formatLocation(fileName, currentFunction->location))
                .c_str());
      }
    }

    return true;
  }
};

std::optional<std::string> readFile(const std::string &name) {
  std::ifstream file{name, std::ios_base::binary};

  if (!file.is_open()) {
    return std::nullopt;
  }

  std::string contents;
  std::string line;

  while (std::getline(file, line)) {
    // if line is a shebang, skip
    if (!(line.length() > 2 && line.at(0) == '#' && line.at(1) == '!')) {
      contents.append(line);
      contents.append("\n");
    }
  }

  file.close();

  return contents;
}

int main(int argc, char **argv) {
  Luau::assertHandler() = assertionHandler;

  for (Luau::FValue<bool> *flag = Luau::FValue<bool>::list; flag;
       flag = flag->next)
    if (strncmp(flag->name, "Luau", 4) == 0)
      flag->value = true;

  if (argc >= 2 && strcmp(argv[1], "--help") == 0) {
    displayHelp(argv[0]);
    return 0;
  } else if (argc < 2) {
    displayHelp(argv[0]);
    return 1;
  }

  const char *fileName = (argc == 3) ? argv[2] : argv[1];
  std::string source;

  if (strcmp(fileName, "-") == 0) {
    // read from stdin
    std::string line;
    while (std::getline(std::cin, line)) {
      source.append(line);
      source.append("\n");
    }
  } else {
    // read from file
    std::optional<std::string> fileContents = readFile(fileName);

    if (fileContents == std::nullopt) {
      std::cerr << "failed reading file: " << fileName << std::endl;
      return 1;
    }

    source = fileContents.value();
  }

  Luau::Allocator allocator;
  Luau::AstNameTable names(allocator);
  Luau::ParseOptions options;

  Luau::ParseResult parseResult = Luau::Parser::parse(
      source.data(), source.size(), names, allocator, options);

  if (!parseResult.errors.empty()) {
    std::cerr << "Parse errors were encountered:" << std::endl;
    for (const Luau::ParseError &error : parseResult.errors) {
      report(fileName, error.getLocation(), "SyntaxError",
             error.getMessage().c_str());
    }

    return 1;
  }

  AstVisitor visitor(fileName);
  parseResult.root->visit(&visitor);

  return 0;
}
