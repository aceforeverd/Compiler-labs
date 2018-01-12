cmake_minimum_required(VERSION 3.8)
project(lab5)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES
        absyn.c
        absyn.h
        env.c
        env.h
        errormsg.c
        errormsg.h
        frame.h
        helper.h
        parse.c
        parse.h
        prabsyn.c
        prabsyn.h
        printtree.c
        printtree.h
        semant.c
        semant.h
        symbol.c
        symbol.h
        table.c
        table.h
        temp.c
        temp.h
        tiger.lex
        tiger.y
        translate.c
        translate.h
        tree.c
        tree.h
        types.c
        types.h
        util.c
        util.h
        x86frame.c)

add_executable(lab5 ${SOURCE_FILES})