CPPFLAGS = -std=c++17 -Wall -Wextra
DEBUGFLAGS = -fsanitize=address -fno-omit-frame-pointer -g
INCDIR = -I./include
OPTFLAG = -O3

bin/%.obj: src/%.cpp include/*
	g++ ${CPPFLAGS} ${OPTFLAG} ${INCDIR} -c -o $@ $<
bin/%_leak.obj: src/%.cpp include/*
	g++ ${CPPFLAGS} ${DEBUGFLAGS} ${INCDIR} -c -o $@ $<

bin/verifier.out: bin/verifier.obj bin/common.obj bin/lambda.obj bin/parser.obj
	g++ ${CPPFLAGS} ${OPTFLAG} $^ -o $@

bin/verifier_leak.out: bin/verifier_leak.obj bin/common_leak.obj bin/lambda_leak.obj bin/parser_leak.obj
	g++ ${CPPFLAGS} ${DEBUGFLAGS} $^ -o $@

bin/def_conv.out: bin/def_conv.obj bin/common.obj bin/lambda.obj bin/parser.obj
	g++ ${CPPFLAGS} ${OPTFLAG} $^ -o $@

bin/def_conv_leak.out: bin/def_conv_leak.obj bin/common_leak.obj bin/lambda_leak.obj bin/parser_leak.obj
	g++ ${CPPFLAGS} ${DEBUGFLAGS} $^ -o $@

bin/test.out: bin/test.obj bin/common.obj bin/lambda.obj bin/parser.obj
	g++ ${CPPFLAGS} ${OPTFLAG} $^ -o $@

src/def_file_nocomm: src/def_file bin/def_conv.out
	bin/def_conv.out -f $< -c > $@

.PHONY: test book parse clean test_read lambda conv conv_leak nocomm noperiod

nocomm: bin/def_conv.out src/def_file
	$< -c -f src/def_file > src/def_file_nocomm

def_file_bez%_cp: def_file_bez%
	cat $< | sed 's/)\.(/-@-/g' | sed 's/\./-/g' | sed 's/-@-/)\.(/g' > $@

noperiod: def_file_bez_213k_cp

test_read: bin/def_conv.out src/def_file
	$^ -r

parse: bin/test_read.out src/def_file
	$< src/def_file

book: bin/verifier.out src/script_test
	$< -c -f src/script_test > src/verifier_out
	diff -s src/verifier_out src/script_test_result

book_leak: bin/verifier_leak.out src/script_test
	$< -c -f src/script_test > src/verifier_out
	diff -s src/verifier_out src/script_test_result

test: bin/test.out
	$<

conv: bin/def_conv.out src/def_file
	$< -c -f src/def_file > src/def_conv_out_c
	$< -n -f src/def_file > src/def_conv_out_n
	$< -c -f src/def_conv_out_n > src/def_conv_out_nc
	$< -n -f src/def_conv_out_c > src/def_conv_out_cn
	diff -s src/def_conv_out_c src/def_conv_out_nc
	diff -s src/def_conv_out_n src/def_conv_out_cn

conv_leak: bin/def_conv_leak.out src/def_file
	$< -c -f src/def_file > src/def_conv_out_c
	$< -n -f src/def_file > src/def_conv_out_n
	$< -c -f src/def_conv_out_n > src/def_conv_out_nc
	$< -n -f src/def_conv_out_c > src/def_conv_out_cn
	diff -s src/def_conv_out_c src/def_conv_out_nc
	diff -s src/def_conv_out_n src/def_conv_out_cn

clean:
	rm -f ./bin/*.out
	rm -f ./bin/*.obj
	rm -f ./src/def_file_nocomm
	rm -f ./verifier_out*
	rm -f ./src/verifier_out*
	rm -f ./src/def_conv_out*
	rm -f ./def_file_bez*_cp
	rm -f ./log*

