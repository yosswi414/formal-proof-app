CPPFLAGS = -std=c++17 -Wall -Wextra
DEBUGFLAGS = -fsanitize=address -fno-omit-frame-pointer -g
INCDIR = include
INCFLAG = -I$(INCDIR)
OPTFLAG = -O2

DEF_FILE = resource/def_file
DEF_NOCOMM = out/def_file_conventional

SRCS_LAMBDA_BASE = lambda.cpp context.cpp definition.cpp environment.cpp judgement.cpp book.cpp
SRCS_DEPEND_BASE = common.cpp parser.cpp inference.cpp

INCLUDES = $(INCDIR)/*
OBJS_LAMBDA = $(addprefix bin/,$(SRCS_LAMBDA_BASE:.cpp=.obj))
OBJS_DEPEND = $(addprefix bin/,$(SRCS_DEPEND_BASE:.cpp=.obj))
OBJS_LAMBDA_DEBUG = $(addprefix bin/,$(SRCS_LAMBDA_BASE:.cpp=_leak.obj))
OBJS_DEPEND_DEBUG = $(addprefix bin/,$(SRCS_DEPEND_BASE:.cpp=_leak.obj))
OBJS = $(OBJS_LAMBDA) $(OBJS_DEPEND)
OBJS_DEBUG = $(OBJS_LAMBDA_DEBUG) $(OBJS_DEPEND_DEBUG)

bin/%_leak.obj: src/%.cpp $(INCLUDES)
	g++ ${CPPFLAGS} ${DEBUGFLAGS} ${INCFLAG} -c -o $@ $<

bin/%.obj: src/%.cpp $(INCLUDES)
	g++ ${CPPFLAGS} ${OPTFLAG} ${INCFLAG} -c -o $@ $<

bin/%_leak.out: bin/%_leak.obj $(OBJS_DEBUG)
	g++ ${CPPFLAGS} ${DEBUGFLAGS} $^ -o $@

bin/%.out: bin/%.obj $(OBJS)
	g++ ${CPPFLAGS} ${OPTFLAG} $^ -o $@

$(DEF_NOCOMM): $(DEF_FILE) bin/def_conv.out
	mkdir -p out
	bin/def_conv.out -f $< -c > $@

.PHONY: clean test test_leak book book_leak conv conv_leak nocomm compile-% compile-noheader-% compile-fall-% test_nocomm all gen gen_leak

nocomm: $(DEF_NOCOMM)

TARGETS = bin/verifier.out bin/def_conv.out bin/genscript.out

all: $(TARGETS)

# usage: $ make compile-<def_name>
# 	to get a script of <def_name> as out/<def_name>.script
#   and its book as out/<def_name>.book
compile-%: TARGET_DEF = $(@:compile-%=%)
compile-%: bin/genscript.out bin/verifier.out $(DEF_FILE)
	bin/genscript.out -f $(DEF_FILE) -t $(TARGET_DEF) -o out/$(TARGET_DEF).script || (make $(@:compile-%=compile-fail-%); exit 0)
	bin/verifier.out -d $(DEF_FILE) -c -f out/$(TARGET_DEF).script -o out/$(TARGET_DEF).book

compile-noheader-%: TARGET_DEF = $(@:compile-noheader-%=%)
compile-noheader-%: bin/genscript.out bin/verifier.out $(DEF_FILE)
	bin/genscript.out -f $(DEF_FILE) -t $(TARGET_DEF) -o out/$(TARGET_DEF).script || (make $(@:compile-%=compile-fail-%); exit 0)
	bin/verifier.out -c -f out/$(TARGET_DEF).script -o out/$(TARGET_DEF).book

compile-fall-%: bin/verifier.out $(DEF_NOCOMM)
	@./test_automake3 $(DEF_NOCOMM) $(@:compile-fail-%=%) out/script_autotest > /dev/null
	@bin/verifier.out -f out/script_autotest -d $(DEF_NOCOMM) -s || (echo "\033[1m\033[31mcheck #1 failed.\033[m"; exit 1)
	@./test_book3 $(DEF_NOCOMM) out/script_autotest > /dev/null || (echo "\033[1m\033[31mcheck #2 failed.\033[m"; exit 1)
	@echo "\033[1m\033[32mDefinition \""$(@:compile-%=%)"\" has been verified successfully\033[m"

test_nocomm: $(DEF_NOCOMM)
	@./test_automake3 $(DEF_NOCOMM) implies script_autotest > /dev/null || (echo "\033[1m\033[31mcheck failed.\033[m"; exit 1)
	@echo "\033[1m\033[32m$(DEF_NOCOMM) has been verified successfully\033[m"

book: bin/verifier.out resource/script_test
book_leak: bin/verifier_leak.out src/script_test
book book_leak:
	mkdir -p out
	$< -c -f resource/script_test -o out/verifier_out
	cmp -s out/verifier_out resource/script_test_result || (echo "\033[1m\033[31moutput book didn't match the answer.\033[m"; exit 1)

test: bin/test.out ${DEF_FILE}
test_leak: bin/test_leak.out ${DEF_FILE}
test test_leak:
	$<

conv: bin/def_conv.out $(DEF_FILE)
conv_leak: bin/def_conv_leak.out $(DEF_FILE)
conv conv_leak:
	mkdir -p out
	@$< -c -f ${DEF_FILE} > out/def_conv_out_c
	@$< -n -f ${DEF_FILE} > out/def_conv_out_n
	@$< -c -f out/def_conv_out_n > out/def_conv_out_nc
	@$< -n -f out/def_conv_out_c > out/def_conv_out_cn
	@cmp -s out/def_conv_out_c out/def_conv_out_nc || (echo "\033[1m\033[31mformat conversion (n->c) failed.\033[m"; exit 1)
	@cmp -s out/def_conv_out_n out/def_conv_out_cn || (echo "\033[1m\033[31mformat conversion (c->n) failed.\033[m"; exit 1)

gen: bin/genscript.out ${DEF_FILE}
gen_leak: bin/genscript_leak.out ${DEF_FILE}
gen gen_leak:
	$< -f ${DEF_FILE} -s

clean:
	rm -f ./bin/*.out
	rm -f ./bin/*.obj
	rm -f ./out/*
