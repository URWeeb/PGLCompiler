cmake_minimum_required(VERSION 3.16)
project(PGLCompiler C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wshadow)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
find_package(GTest REQUIRED)

if(APPLE)
    execute_process(
            COMMAND brew --prefix llvm@20
            OUTPUT_VARIABLE HOMEBREW_LLVM
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(CMAKE_PREFIX_PATH "${HOMEBREW_LLVM}" ${CMAKE_PREFIX_PATH})
    list(PREPEND CMAKE_PROGRAM_PATH "${HOMEBREW_LLVM}/bin")
    include_directories(SYSTEM "${HOMEBREW_LLVM}/include")
endif()

find_program(LLVM_CONFIG NAMES llvm-config-20 llvm-config REQUIRED)
execute_process(COMMAND ${LLVM_CONFIG} --version OUTPUT_VARIABLE LLVM_PACKAGE_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG} --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG} --libdir OUTPUT_VARIABLE LLVM_LIBRARY_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG} --libs
        core
        support
        target
        mc
        targetparser
        nativecodegen
        ipo
        passes
        scalaropts
        instcombine
        asmprinter
        object
        transformutils
        OUTPUT_VARIABLE LLVM_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(COMMAND ${LLVM_CONFIG} --system-libs OUTPUT_VARIABLE LLVM_SYSTEM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} via ${LLVM_CONFIG}")
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
separate_arguments(LLVM_LIBS)
separate_arguments(LLVM_SYSTEM_LIBS)

BISON_TARGET(Parser grammar/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cc)
FLEX_TARGET(Scanner grammar/scanner.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cc)
ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)

set_source_files_properties(${FLEX_Scanner_OUTPUTS} PROPERTIES LANGUAGE CXX)

include_directories(include ${CMAKE_CURRENT_BINARY_DIR})

set(VISITOR_SOURCES
    src/Interpreter.cpp
    src/IRVisitor.cpp
    src/PrintVisitor.cpp
    src/ScopeVisitor.cpp
    src/TypeChecker.cpp
    src/Compiler.cpp
)

add_executable(compiler
    src/main.cpp
    ${VISITOR_SOURCES}
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Scanner_OUTPUTS}
)

target_link_libraries(compiler ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS})

add_executable(run_tests
    tests/tests.cpp
    ${VISITOR_SOURCES}
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Scanner_OUTPUTS}
)

target_link_libraries(run_tests GTest::gtest GTest::gtest_main pthread ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS})

enable_testing()
add_test(NAME CompilerTests COMMAND run_tests)
