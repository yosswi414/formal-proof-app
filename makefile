
test_read: src/main.cpp
	g++ -Wall -Wextra -o $@ $<

test_read_leak: src/main.cpp
	g++ -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer -g -o $@ $<

.PHONY: run clean leak def_test

run: test_read
	./test_read src/def_file

clean:
	rm -f ./test_read
	rm -f ./test_read_leak
	rm -f ./src/def_file_nocomm

leak: test_read_leak
	./test_read_leak src/def_file

def_name=implies

def_test: src/def_file
	cat src/def_file | sed "s/\/\/.*//g" | sed "s/\\s//g" > src/def_file_nocomm
	./test_automake3 src/def_file_nocomm ${def_name} script
	./test_book3 src/def_file_nocomm script