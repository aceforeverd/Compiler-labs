cmake_minimum_required(VERSION 3.8)
project(lab3)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        absyn.c
        absyn.h
        errormsg.c
        errormsg.h
        Makefile
        parse.c
        parse.h
        prabsyn.c
        prabsyn.h
        symbol.c
        symbol.h
        table.c
        table.h
        tiger.lex
        tiger.y
        util.c
        util.h)

add_executable(lab3 ${SOURCE_FILES})