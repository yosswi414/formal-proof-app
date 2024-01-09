CPPFLAGS = -std=c++17 -Wall -Wextra -O2
DEBUGFLAGS = -fsanitize=address -fno-omit-frame-pointer -g
INCDIR = -I./include

bin/%.obj: src/%.cpp include/*
	g++ ${CPPFLAGS} ${INCDIR} -c -o $@ $<

bin/verifier.out: bin/verifier.obj bin/common.obj bin/lambda.obj
	g++ ${CPPFLAGS} $^ -o $@

bin/def_conv.out: bin/def_conv.obj bin/common.obj bin/lambda.obj bin/parser.obj
	g++ ${CPPFLAGS} $^ -o $@

bin/def_conv_leak.out: bin/def_conv.obj bin/common.obj bin/lambda.obj bin/parser.obj
	g++ ${CPPFLAGS} ${DEBUGFLAGS} $^ -o $@

bin/test.out: bin/test.obj bin/common.obj bin/lambda.obj
	g++ ${CPPFLAGS} $^ -o $@

src/def_file_nocomm: src/def_file bin/def_conv.out
	bin/def_conv.out -f $< -c > $@


.PHONY: test book parse clean test_read lambda conv conv_leak

test_read: bin/def_conv.out src/def_file
	$^ -r

parse: bin/test_read.out src/def_file
	$< src/def_file

book: bin/verifier.out src/script_test
	$< src/script_test

test: bin/test.out
	$<

conv: bin/def_conv.out src/def_file
	$< -s -f src/def_file

conv_leak: bin/def_conv_leak.out src/def_file
	$< -s -f src/def_file

clean:
	rm -f ./bin/*.out
	rm -f ./bin/*.obj
	rm -f ./src/def_file_nocomm

