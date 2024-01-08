CPPFLAGS = -std=c++17 -Wall -Wextra
DEBUGFLAGS = -fsanitize=address -fno-omit-frame-pointer -g
INCDIR = -I./include

bin/%.obj: src/%.cpp include/*
	g++ ${CPPFLAGS} ${INCDIR} -c -o $@ $<

bin/test_read.out: src/def_validator.cpp
	g++ ${CPPFLAGS} -o $@ $<

bin/test_read_leak.out: src/def_validator.cpp
	g++ ${CPPFLAGS} ${DEBUGFLAGS} -o $@ $<

bin/verifier.out: bin/verifier.obj bin/common.obj
	g++ ${CPPFLAGS} $^ -O2 -o $@

bin/def_conv.out: bin/def_conv.obj bin/common.obj bin/lambda.obj bin/parser.obj
	g++ ${CPPFLAGS} $^ -O2 -o $@

bin/def_conv_leak.out: bin/def_conv.obj bin/common.obj bin/lambda.obj bin/parser.obj
	g++ ${CPPFLAGS} ${DEBUGFLAGS} $^ -o $@

src/def_file_nocomm: src/def_file bin/def_conv.out
	bin/def_conv.out -f $< -c > $@


.PHONY: book parse clean read_leak lambda conv conv_leak

read_leak: bin/test_read_leak.out src/def_file
	$< src/def_file

parse: bin/test_read.out src/def_file
	$< src/def_file

book: bin/verifier.out src/script_test
	$< src/script_test

conv: bin/def_conv.out src/def_file
	$< -s -f src/def_file

conv_leak: bin/def_conv_leak.out src/def_file
	$< -s -f src/def_file

clean:
	rm -f ./bin/*.out
	rm -f ./bin/*.obj
	rm -f ./src/def_file_nocomm

