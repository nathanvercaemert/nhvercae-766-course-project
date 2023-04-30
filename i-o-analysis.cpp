#include <iostream>
#include <map>
#include <vector>
#include <utility>
#include <sstream>
#include <cstring>
#include <string>
#include <algorithm>
#include <fstream>
#include <clang-c/Index.h>

enum CXChildVisitResult getChildCursorVisitor(CXCursor cursor, CXCursor parent, CXClientData client_data) {
    std::string *var_name = reinterpret_cast<std::string*>(client_data);
    if (clang_getCursorKind(cursor) == CXCursor_DeclRefExpr && var_name->empty()) {
        std::string cursor_spelling = clang_getCString(clang_getCursorSpelling(cursor));
        if (cursor_spelling != "fgetc" && cursor_spelling != "getc") {
            *var_name = cursor_spelling;
        }
    }
    return CXChildVisit_Recurse;
}

std::string trim(const std::string &s)
{
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }
 
    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
 
    return std::string(start, end + 1);
}

std::string get_surrounding_context(CXSourceLocation location, int before, int after) {
    unsigned line_number;
    CXFile file;
    clang_getSpellingLocation(location, &file, &line_number, nullptr, nullptr);
    CXString file_name_cxstr = clang_getFileName(file);
    const char *file_name = clang_getCString(file_name_cxstr);
    std::ifstream file_stream(file_name);
    std::string line, context;
    for (unsigned i = 1; std::getline(file_stream, line); ++i) {
        if (i >= line_number - before && i <= line_number + after) {
            if (before + after <= 1) {
                context += line;
            } else {
                context += line + '\n';
            }
        }
    }
    clang_disposeString(file_name_cxstr);
    return context;
}

std::vector<std::pair<std::string, std::string>> variables_and_definition;
std::vector<std::pair<std::string, std::string>> variables_and_use;

bool is_function_call(CXCursor cursor, const char *func_name) {
    if (clang_getCursorKind(cursor) == CXCursor_CallExpr || clang_getCursorKind(cursor) == CXCursor_UnexposedExpr) {
        CXString cursor_spelling = clang_getCursorSpelling(cursor);
        bool result = strcmp(clang_getCString(cursor_spelling), func_name) == 0;
        clang_disposeString(cursor_spelling);
        return result;
    }
    return false;
}

void find_use_in_children(CXCursor cursor, std::string &var_name, bool &eof_found) {
    std::pair<std::string*, bool*> data = std::make_pair(&var_name, &eof_found);
    clang_visitChildren(cursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data) {
            auto data = reinterpret_cast<std::pair<std::string*, bool*>*>(client_data);
            std::string *var_name = data->first;
            bool *eof_found = data->second;

            if (is_function_call(c, "fgetc") || is_function_call(c, "getc")) {
                clang_visitChildren(c, getChildCursorVisitor, var_name);
            } else if (clang_getCursorKind(c) == CXCursor_IntegerLiteral) {
                auto res = clang_Cursor_Evaluate(c);
                auto int_lit_value = clang_EvalResult_getAsInt(res);
                clang_EvalResult_dispose(res);
                if (int_lit_value == 1) { // == 1 if EOF
                    *eof_found = true;
                }
            }

            if (var_name->empty() || !*eof_found) {
                find_use_in_children(c, *var_name, *eof_found);
            }

            return CXChildVisit_Continue;
        },
        &data);
}

void determine_use(CXCursor cursor) {
    std::string var_name;
    bool eof_found = false;
    find_use_in_children(cursor, var_name, eof_found);

    if (!var_name.empty() && eof_found) {
        CXSourceLocation location = clang_getCursorLocation(cursor);
        std::string context = get_surrounding_context(location, 1, -1);
        if (strstr(context.c_str(), "EOF") != NULL) {
            variables_and_use.emplace_back(var_name, context);
        }
    }
}

void print_ast(CXCursor cursor, int depth) {
    if (clang_getCursorKind(cursor) == CXCursor_FirstInvalid)
        return;

    // for (int i = 0; i < depth; ++i)
    //     std::cout << "  ";

    CXString cursor_kind_name = clang_getCursorKindSpelling(clang_getCursorKind(cursor));
    CXString cursor_spelling = clang_getCursorSpelling(cursor);
    // std::cout << clang_getCString(cursor_kind_name) << " " << clang_getCString(cursor_spelling) << std::endl;

    if (clang_getCursorKind(cursor) == CXCursor_BinaryOperator) {
        bool found = false;
        clang_visitChildren(cursor,
            [](CXCursor c, CXCursor parent, CXClientData client_data) {
                bool *found = reinterpret_cast<bool*>(client_data);
                if (clang_getCursorKind(c) == CXCursor_CallExpr || clang_getCursorKind(c) == CXCursor_UnexposedExpr) {
                    CXString call_cursor_spelling = clang_getCursorSpelling(c);
                    if (strcmp(clang_getCString(call_cursor_spelling), "fopen") == 0 || strcmp(clang_getCString(call_cursor_spelling), "fopen_utf8") == 0) {
                        *found = true;
                        clang_visitChildren(parent,
                            [](CXCursor c, CXCursor parent, CXClientData client_data) {
                                if (clang_getCursorKind(c) == CXCursor_DeclRefExpr) {
                                    CXString variable_name = clang_getCursorSpelling(c);
                                    CXSourceLocation location = clang_getCursorLocation(c);
                                    std::string context = get_surrounding_context(location, 1, -1);
                                    variables_and_definition.emplace_back(clang_getCString(variable_name), context);
                                    clang_disposeString(variable_name);
                                }
                                return CXChildVisit_Continue;
                            },
                            nullptr);
                    }
                    clang_disposeString(call_cursor_spelling);
                }
                return *found ? CXChildVisit_Break : CXChildVisit_Continue;
            },
            &found);
    }

    if (clang_getCursorKind(cursor) == CXCursor_WhileStmt || clang_getCursorKind(cursor) == CXCursor_ForStmt) {
        determine_use(cursor);
    }

    clang_disposeString(cursor_kind_name);
    clang_disposeString(cursor_spelling);

    // if (clang_getCursorKind(cursor) == CXCursor_IntegerLiteral) {
    //     auto res = clang_Cursor_Evaluate(cursor);
    //     auto int_lit_value = clang_EvalResult_getAsInt(res);
    //     clang_EvalResult_dispose(res);
    //     std::cout << int_lit_value << std::endl;
    // }

    clang_visitChildren(cursor,
                        [](CXCursor c, CXCursor parent, CXClientData client_data) {
                            int *depth = reinterpret_cast<int*>(client_data);
                            print_ast(c, *depth + 1);
                            return CXChildVisit_Continue;
                        },
                        &depth);

}

void print_results() {
    std::map<std::string, std::pair<std::string, std::vector<std::string>>> variable_info;

    for (const auto& var_and_def : variables_and_definition) {
        variable_info[var_and_def.first].first = var_and_def.second;
    }

    for (const auto& var_and_use : variables_and_use) {
        variable_info[var_and_use.first].second.push_back(var_and_use.second);
    }

    for (const auto& var_info : variable_info) {
        if (!var_info.second.first.empty() && !var_info.second.second.empty()) {
            std::cout << "Variable: " << var_info.first << std::endl;

            std::string def_context = var_info.second.first;
            if (strstr(def_context.c_str(), "\n") != NULL) {
                std::cout << "Definition:" << std::endl << def_context << std::endl;
            } else {
                std::cout << "Definition: " << trim(def_context) << std::endl;
            }

            for (const auto& use_context : var_info.second.second) {
                if (strstr(use_context.c_str(), "\n") != NULL) {
                    std::cout << "Use:" << std::endl << use_context << std::endl;
                } else {
                    std::cout << "Use: " << trim(use_context) << std::endl;
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: ast_printer <path_to_c_file>" << std::endl;
        return 1;
    }

    // Read the contents of the file into a string.
    std::ifstream file_stream(argv[1]);
    std::stringstream buffer;
    buffer << "#include <stdio.h>\n#include <sys/types.h>" << file_stream.rdbuf();
    std::string file_contents = buffer.str();

    CXIndex index = clang_createIndex(0, 0);

    // Create a CXUnsavedFile structure for the modified file.
    CXUnsavedFile unsaved_file = {
        argv[1],  // filename
        file_contents.c_str(),  // contents
        static_cast<unsigned long>(file_contents.length())  // length
    };

    const char *args[] = {"-std=c11", "-I/usr/include", "-I/usr/local/include", "-fparse-all-comments"};
    CXTranslationUnit tu = clang_parseTranslationUnit(
    index, argv[1], args, 3,
    &unsaved_file, 1,
    CXTranslationUnit_DetailedPreprocessingRecord |
    CXTranslationUnit_IncludeAttributedTypes |
    CXTranslationUnit_VisitImplicitAttributes |
    CXTranslationUnit_IgnoreNonErrorsFromIncludedFiles |
    CXTranslationUnit_KeepGoing |
    CXTranslationUnit_IncludeBriefCommentsInCodeCompletion |
    CXTranslationUnit_RetainExcludedConditionalBlocks |
    CXTranslationUnit_IgnoreNonErrorsFromIncludedFiles
                                                      );

    if (!tu) {
        std::cerr << "Error: Unable to parse the C file." << std::endl;
        return 1;
    }

    CXCursor root_cursor = clang_getTranslationUnitCursor(tu);

    print_ast(root_cursor, 0);

    print_results();

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return 0;
}
