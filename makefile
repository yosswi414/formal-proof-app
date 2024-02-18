CC := g++
TARGET_NAME := def_conv.out verifier.out genscript.out test.out
TARGET = $(addprefix $(BINDIR)/, $(TARGET_NAME))
TARGET_D = $(addprefix $(BINDIR_D)/, $(TARGET_NAME))
TARGET_PUB = $(addprefix $(BINDIR)/, $(filter-out test.out, $(TARGET_NAME)))
TARGET_ROOT = $(addprefix ./, $(filter-out test.out, $(TARGET_NAME)))

SRCDIR := src
INCDIR := include

OUTDIR := out
BINDIR := $(OUTDIR)/.bin
BINDIR_D := $(OUTDIR)/.bin_d
OBJDIR := $(OUTDIR)/.obj
OBJDIR_D := $(OUTDIR)/.obj_d
DEPDIR := $(OUTDIR)/.dep

CPPFLAGS := -I$(INCDIR) -Wall -Wextra -std=c++17
OPTFLAG := -O2
DEBUGFLAGS = -fsanitize=address -fno-omit-frame-pointer -g
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(addprefix $(OBJDIR)/, $(notdir $(SRCS:.cpp=.o)))
OBJS_D := $(addprefix $(OBJDIR_D)/, $(notdir $(SRCS:.cpp=.o)))
DEPS := $(addprefix $(DEPDIR)/, $(notdir $(SRCS:.cpp=.d)))

$(BINDIR)/%.out: $(filter-out $(TARGET:out/.bin/%.out=out/.obj/%.o), $(OBJS)) $(OBJDIR)/%.o | $(BINDIR)
	$(CC) $(CPPFLAGS) $(OPTFLAG) -o $@ $^
$(BINDIR_D)/%.out: $(filter-out $(TARGET_D:out/.bin_d/%.out=out/.obj_d/%.o), $(OBJS_D)) $(OBJDIR_D)/%.o | $(BINDIR_D)
	$(CC) $(CPPFLAGS) $(DEBUGFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(DEPDIR)/%.d | $(OBJDIR) $(DEPDIR)
	$(CC) $(DEPFLAGS) $(CPPFLAGS) $(OPTFLAG) -c -o $@ $<
$(OBJDIR_D)/%.o: $(SRCDIR)/%.cpp $(DEPDIR)/%.d | $(OBJDIR_D) $(DEPDIR)
	$(CC) $(DEPFLAGS) $(CPPFLAGS) $(DEBUGFLAGS) -c -o $@ $<

$(DEPS):

include $(wildcard $(DEPS))

.SECONDARY:

$(BINDIR) $(OBJDIR) $(DEPDIR) $(BINDIR_D) $(OBJDIR_D):
	mkdir -p $@

$(TARGET_ROOT): $(TARGET_PUB)
	-ln --backup=none -s -t . $(@:%.out=./$(BINDIR)/%.out)

.PHONY: alias
alias: $(TARGET_ROOT)

.PHONY: all_min
all_min: $(TARGET_PUB) alias

.DEFAULT_GOAL := all_min

.PHONY: all
all: clean all_min

.PHONY: all_min_d
all_min_d: $(TARGET_D)

.PHONY: all_d
all_d: clean all_min_d



.PHONY: clean
clean:
	-rm -rf $(OUTDIR)
	-rm -f ./*.out

DEF_FILE := resource/def_file

.PHONY: check
check: genscript.out verifier.out $(DEF_FILE)
	@./genscript.out -f $(DEF_FILE) -o out/$(notdir $(DEF_FILE)).script || (echo "\033[1m\033[31merror\033[m: failed to generate a script of last definition in $(DEF_FILE)"; exit 1)
	@./verifier.out -c -f out/$(DEF_FILE).script -o out/out.book -e out/verifier_out.log || (echo "\033[1m\033[31merror\033[m: failed to verify the script of last definition in $(DEF_FILE)"; exit 1)
	@echo "\033[1m\033[32mOK\033[m: script -> out/$(DEF_FILE).script, book -> out/out.book"

.PHONY: check-%
check-%: TARGET_DEF = $(@:check-%=%)
check-%: genscript.out verifier.out $(DEF_FILE)
	@./genscript.out -f $(DEF_FILE) -t $(TARGET_DEF) -o out/$(TARGET_DEF).script || (echo "\033[1m\033[31merror\033[m: failed to generate a script of \"$(TARGET_DEF)\""; exit 1)
	@./verifier.out -c -f out/$(TARGET_DEF).script -o out/out.book -e out/$(TARGET_DEF).log || (echo "\033[1m\033[31merror\033[m: failed to verify the script of \"$(TARGET_DEF)\""; exit 1)
	@echo "\033[1m\033[32mOK\033[m: script -> out/$(TARGET_DEF).script, book -> out/out.book"

# test commands
.PHONY: test test-% test_d test_d-%
test test-%: IS_DEBUG = ""
test_d test_d-%: IS_DEBUG = " (debug)"

test: out/.bin/test.out $(DEF_FILE)
test_d: out/.bin_d/test.out $(DEF_FILE)
test test_d:
	@echo "running test: test.cpp$(IS_DEBUG)..."
	@$< || (echo "\033[1m\033[31merror\033[m: test.out exited with an error."; exit 1)
	@echo "\033[1m\033[32mpassed\033[m: test.cpp$(IS_DEBUG)"

test-conv: out/.bin/def_conv.out $(DEF_FILE)
test_d-conv: out/.bin_d/def_conv.out $(DEF_FILE)
test-conv test_d-conv:
	@echo "running test: conv$(IS_DEBUG)..."
	@$< -c -f $(DEF_FILE) > out/def_conv_out_c || (echo "\033[1m\033[31merror\033[m: format conversion (*->c) failed."; exit 1)
	@$< -n -f $(DEF_FILE) > out/def_conv_out_n || (echo "\033[1m\033[31merror\033[m: format conversion (*->n) failed."; exit 1)
	@$< -c -f out/def_conv_out_n > out/def_conv_out_nc || (echo "\033[1m\033[31merror\033[m: format conversion (n->c) failed."; exit 1)
	@cmp out/def_conv_out_c out/def_conv_out_nc || (echo "\033[1m\033[31merror\033[m: format conversion (n->c) didn't match the reference (*->c)."; exit 1)
	@$< -n -f out/def_conv_out_c > out/def_conv_out_cn || (echo "\033[1m\033[31merror\033[m: format conversion (c->n) failed."; exit 1)
	@$< -c -f out/def_conv_out_cn > out/def_conv_out_cnc || (echo "\033[1m\033[31merror\033[m: format conversion (c->n->c) failed."; exit 1)
	@cmp out/def_conv_out_c out/def_conv_out_cnc || (echo "\033[1m\033[31merror\033[m: format conversion (c->n->c) didn't match the reference (*->c)."; exit 1)
	@echo "\033[1m\033[32mpassed\033[m: conv$(IS_DEBUG)"

test-verify: out/.bin/verifier.out resource/script_test resource/script_test_result
test_d-verify: out/.bin_d/verifier.out resource/script_test resource/script_test_result
test-verify test_d-verify:
	@echo "running test: verify$(IS_DEBUG)..."
	@$< -c -f resource/script_test -o out/test-verify.tmp || (echo "\033[1m\033[31merror\033[m: book generation failed."; exit 1)
	@cmp out/test-verify.tmp resource/script_test_result || (echo "\033[1m\033[31merror\033[m: the result book didn't match the reference."; exit 1)
	@echo "\033[1m\033[32mpassed\033[m: verify$(IS_DEBUG)"

test-gen: out/.bin/genscript.out $(DEF_FILE)
test_d-gen: out/.bin_d/genscript.out $(DEF_FILE)
test-gen test_d-gen:
	@echo "running test: gen$(IS_DEBUG)..."
	@$< -f $(DEF_FILE) -s || (echo "\033[1m\033[31merror\033[m: script generation failed."; exit 1)
	@echo "\033[1m\033[32mpassed\033[m: gen$(IS_DEBUG)"

test-all: TESTTYPE = "test"
test_d-all: TESTTYPE = "test_d"
test-all test_d-all:
	@echo "running test: all$(IS_DEBUG)..."
	@(make $(TEST_TYPE)) || (echo "\033[1m\033[31merror\033[m: test test.cpp$(IS_DEBUG) failed."; exit 1)
	@(make $(TEST_TYPE)-conv) || (echo "\033[1m\033[31merror\033[m: test conv$(IS_DEBUG) failed."; exit 1)
	@(make $(TEST_TYPE)-verify) || (echo "\033[1m\033[31merror\033[m: test verify$(IS_DEBUG) failed."; exit 1)
	@(make $(TEST_TYPE)-gen) || (echo "\033[1m\033[31merror\033[m: test gen$(IS_DEBUG) failed."; exit 1)
	@echo "\033[1m\033[32mpassed\033[m: all$(IS_DEBUG)"

test-%:
	@(echo "\033[1m\033[31merror\033[m: unknown test command: $(@:test-%=%)"; exit 1)