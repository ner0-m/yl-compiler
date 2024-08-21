#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>

#include "llvm/IR/Module.h"

#include "backward.hpp"

#include "ast.h"
#include "cfg.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "utils.h"

void displayHelp() {
    std::print("Usage\n"
               "  compiler [options] <source_file>\n"
               "Options:\n"
               "  -h           display this message\n"
               "  -o <file>    write executable to <file>\n"
               "  -ast-dump    print the abstract syntax tree\n"
               "  -res-dump    print the resolved syntax tree\n"
               "  -llvm-dump   print the llvm module\n"
               "  -cfg-dump    print the control flow graph\n");
}

[[noreturn]] void error(std::string_view msg) {
    std::print(std::cerr, "error: {}\n", msg);
    displayHelp();
    std::exit(1);
}

struct CompilerOptions {
    std::filesystem::path source;
    std::filesystem::path output;
    bool displayHelp = false;
    bool astDump = false;
    bool resDump = false;
    bool llvmDump = false;
    bool cfgDump = false;
};

CompilerOptions parseArguments(int argc, const char **argv) {
    CompilerOptions options;

    int idx = 1;
    while (idx < argc) {
        std::string_view arg = argv[idx];

        if (arg[0] != '-') {
            if (!options.source.empty()) {
                error("unexpected argument '" + std::string(arg) + '\'');
            }

            options.source = arg;
        } else {
            if (arg == "-h")
                options.displayHelp = true;
            else if (arg == "-o")
                options.output = ++idx >= argc ? "" : argv[idx];
            else if (arg == "-ast-dump")
                options.astDump = true;
            else if (arg == "-res-dump")
                options.resDump = true;
            else if (arg == "-llvm-dump")
                options.llvmDump = true;
            else if (arg == "-cfg-dump")
                options.cfgDump = true;
            else
                error("unexpected option '" + std::string(arg) + '\'');
        }

        ++idx;
    }

    return options;
}

int main(int argc, const char **argv) {
    backward::SignalHandling sh;

    CompilerOptions options = parseArguments(argc, argv);

    if (options.displayHelp) {
        displayHelp();
        return 0;
    }

    if (options.source.empty()) {
        error("no source file specified");
    }

    if (options.source.extension() != ".yl") {
        error("unexpected source file extension");
    }

    std::ifstream file(options.source);
    if (!file) {
        error(std::format("failed to open '{}'", options.source.string()));
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    source_file src_file = {options.source.c_str(), buffer.str()};

    lexer lexer(src_file);
    parser parser(lexer);

    auto [ast, success] = parser.parse_source_file();

    if (options.astDump) {
        for (auto &&fn : ast) {
            fn->dump();
        }
        return 0;
    }

    if (!success) {
        return 1;
    }

    sema sema(std::move(ast));
    auto resolvedTree = sema.resolve_ast();

    if (options.resDump) {
        for (auto &&fn : resolvedTree) {
            fn->dump();
        }
        return 0;
    }

    if (options.cfgDump) {
        for (auto &&fn : resolvedTree) {
            std::print(std::cerr, "{}:\n", fn->identifier);
            cfg_builder().build(*fn).dump();
        }
        return 0;
    }

    if (resolvedTree.empty()) {
        return 1;
    }

    codegen codegen(std::move(resolvedTree), options.source.c_str());
    llvm::Module *llvmIR = codegen.generate_ir();

    if (options.llvmDump) {
        codegen.dump();
        return 0;
    }

    std::stringstream path;
    path << "tmp-" << std::filesystem::hash_value(options.source) << ".ll";
    const std::string &llvmIRPath = path.str();

    std::error_code errorCode;
    llvm::raw_fd_ostream f(llvmIRPath, errorCode);
    llvmIR->print(f, nullptr);

    std::stringstream command;
    command << "clang " << llvmIRPath;
    if (!options.output.empty())
        command << " -o " << options.output;

    int ret = std::system(command.str().c_str());

    std::filesystem::remove(llvmIRPath);

    return ret;
}
