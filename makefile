CPPFLAGS = -std=c++17 -Wall -Wextra
DEBUGFLAGS = -fsanitize=address -fno-omit-frame-pointer -g
INCDIR = include
INCFLAG = -I$(INCDIR)
OPTFLAG = -O2

DEF_FILE = def_file_bez

SRCS_LAMBDA_BASE = lambda.cpp context.cpp definition.cpp environment.cpp judgement.cpp book.cpp
SRCS_DEPEND_BASE = common.cpp parser.cpp inference.cpp

INCLUDES = $(INCDIR)/*
OBJS_LAMBDA = $(addprefix bin/,$(SRCS_LAMBDA_BASE:.cpp=.obj))
OBJS_DEPEND = $(addprefix bin/,$(SRCS_DEPEND_BASE:.cpp=.obj))
OBJS_LAMBDA_DEBUG = $(addprefix bin/,$(SRCS_LAMBDA_BASE:.cpp=_leak.obj))
OBJS_DEPEND_DEBUG = $(addprefix bin/,$(SRCS_DEPEND_BASE:.cpp=_leak.obj))

bin/%.obj: src/%.cpp $(INCLUDES)
	g++ ${CPPFLAGS} ${OPTFLAG} ${INCFLAG} -c -o $@ $<
bin/%_leak.obj: src/%.cpp $(INCLUDES)
	g++ ${CPPFLAGS} ${DEBUGFLAGS} ${INCFLAG} -c -o $@ $<

bin/verifier.out: bin/verifier.obj bin/common.obj bin/parser.obj $(OBJS_LAMBDA)
	g++ ${CPPFLAGS} ${OPTFLAG} $^ -o $@

bin/verifier_leak.out: bin/verifier_leak.obj bin/common_leak.obj bin/parser_leak.obj $(OBJS_LAMBDA_DEBUG)
	g++ ${CPPFLAGS} ${DEBUGFLAGS} $^ -o $@

bin/def_conv.out: bin/def_conv.obj bin/common.obj bin/parser.obj $(OBJS_LAMBDA)
	g++ ${CPPFLAGS} ${OPTFLAG} $^ -o $@

bin/def_conv_leak.out: bin/def_conv_leak.obj bin/common_leak.obj bin/parser_leak.obj $(OBJS_LAMBDA_DEBUG)
	g++ ${CPPFLAGS} ${DEBUGFLAGS} $^ -o $@

bin/test.out: bin/test.obj $(OBJS_LAMBDA) $(OBJS_DEPEND)
	g++ ${CPPFLAGS} ${OPTFLAG} $^ -o $@

bin/test_leak.out: bin/test_leak.obj $(OBJS_LAMBDA_DEBUG) $(OBJS_DEPEND_DEBUG)
	g++ ${CPPFLAGS} ${DEBUGFLAGS} $^ -o $@

src/def_file_nocomm: src/def_file bin/def_conv.out
	bin/def_conv.out -f $< -c > $@

.PHONY: test book parse clean test_read lambda conv test_leak book_leak conv_leak nocomm compile-% test_nocomm

nocomm: src/def_file_nocomm

# usage: $ make compile-<def_name>
compile-%: bin/verifier.out
	@rm -f src/def_file_nocomm
	@make nocomm
	@./test_automake3 src/def_file_nocomm $(@:compile-%=%) script_autotest > /dev/null
	@bin/verifier.out -f script_autotest -d src/def_file_nocomm -s || (echo "\033[1m\033[31mcheck #1 failed.\033[m"; exit 1)
	@./test_book3 src/def_file_nocomm script_autotest > /dev/null || (echo "\033[1m\033[31mcheck #2 failed.\033[m"; exit 1)
	@echo "\033[1m\033[32mDefinition \""$(@:compile-%=%)"\" has been verified successfully\033[m"

test_nocomm: src/def_file_nocomm
	@./test_automake3 src/def_file_nocomm implies script_autotest > /dev/null || (echo "\033[1m\033[31mcheck failed.\033[m"; exit 1)
	@echo "\033[1m\033[32mdef_file_nocomm has been verified successfully\033[m"

test_read: bin/def_conv.out src/def_file
	$^ -r

parse: bin/test_read.out src/def_file
	$< src/def_file

book: bin/verifier.out src/script_test
	$< -c -f src/script_test -o src/verifier_out
	diff -s src/verifier_out src/script_test_result

book_leak: bin/verifier_leak.out src/script_test
	$< -c -f src/script_test -o src/verifier_out
	diff -s src/verifier_out src/script_test_result

test: bin/test.out ${DEF_FILE}
	$<

test_leak: bin/test_leak.out ${DEF_FILE}
	$<

conv: bin/def_conv.out src/def_file
	$< -c -f ${DEF_FILE} > src/def_conv_out_c
	$< -n -f ${DEF_FILE} > src/def_conv_out_n
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
	rm -f ./def_conv_out*
	rm -f ./src/def_conv_out*
	rm -f ./def_file_bez*_cp
	rm -f ./log*
	rm -f ./script_autotest*

