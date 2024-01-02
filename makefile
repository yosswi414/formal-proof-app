CPPFLAGS = -Wall -Wextra -Iinclude/
DEBUGFLAGS = -fsanitize=address -fno-omit-frame-pointer -g

bin/test_read.out: src/def_validator.cpp
	g++ ${CPPFLAGS} -o $@ $<

bin/test_read_leak.out: src/def_validator.cpp
	g++ ${CPPFLAGS} ${DEBUGFLAGS} -o $@ $<

bin/%.obj: src/%.cpp
	g++ ${CPPFLAGS} -c -Wl,-lstdc++ -o $@ $<

bin/verifier.out: bin/verifier.obj bin/common.obj
	g++ ${CPPFLAGS} $^ -O2 -o $@

.PHONY: book parse clean leak lambda

parse: bin/test_read.out src/def_file
	bin/test_read.out src/def_file

src/def_file_nocomm: src/def_file
	./trim_comment.sh $@

book: bin/verifier.out src/script_test
	bin/verifier.out src/script_test

bin/lambda.out: src/lambda.cpp include/lambda.hpp
	g++ -O0 ${CPPFLAGS} ${DEBUGFLAGS} -o $@ $<

lambda: bin/lambda.out
	bin/lambda.out

clean:
	rm -f ./bin/*.out
	rm -f ./bin/*.obj
	rm -f ./src/def_file_nocomm

leak: bin/test_read_leak.out
	bin/test_read_leak.out src/def_file

