
bin/test_read.out: src/def_validator.cpp
	g++ -Wall -Wextra -o $@ $<

bin/test_read_leak.out: src/def_validator.cpp
	g++ -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer -g -o $@ $<

bin/%.obj: src/%.cpp
	g++ -Wall -Wextra -c -Wl,-lstdc++ -o $@ $<

bin/verifier.out: bin/verifier.obj bin/common.obj
	g++ $^ -O2 -o $@

.PHONY: book parse clean leak lambda

parse: bin/test_read.out src/def_file
	bin/test_read.out src/def_file

src/def_file_nocomm: src/def_file
	./trim_comment.sh $@

book: bin/verifier.out src/script_test
	bin/verifier.out src/script_test

lambda: src/lambda.cpp src/lambda.hpp
	g++ -Wall -Wextra -o bin/lambda.out src/lambda.cpp
	bin/lambda.out

clean:
	rm -f ./bin/*.out
	rm -f ./bin/*.obj
	rm -f ./src/def_file_nocomm

leak: bin/test_read_leak.out
	bin/test_read_leak.out src/def_file

