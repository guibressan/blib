#!/usr/bin/env bash
####################
set -e
####################
readonly CC="zig cc"
####################

run() 
{
	mkdir -p out

	${CC} -Wall -O3 ${1} -std=c89 -pedantic -o out/tests tests/test_main.c \
	&& ./out/tests
}

####################
run
