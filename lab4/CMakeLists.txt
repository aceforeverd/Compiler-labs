cmake_minimum_required(VERSION 3.8)
project(lab4)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES
        absyn.c
        absyn.h
        env.c
        env.h
        errormsg.c
        errormsg.h
        helper.h
        parse.c
        parse.h
        prabsyn.c
        prabsyn.h
        semant.c
        semant.h
        symbol.c
        symbol.h
        table.c
        table.h
        tiger.lex
        tiger.y
        types.c
        types.h
        util.c
        util.h)

add_executable(lab4 ${SOURCE_FILES})