CC := g++
TARGET_NAME := def_conv.out verifier.out genscript.out test.out
TARGET = $(addprefix $(BINDIR)/, $(TARGET_NAME))
TARGET_D = $(addprefix $(BINDIR_D)/, $(TARGET_NAME))
TARGET_PUB = $(addprefix $(BINDIR)/, $(filter-out test.out, $(TARGET_NAME)))

SRCDIR := src
INCDIR := include

OUTDIR := out
BINDIR := $(OUTDIR)/.bin
BINDIR_D := $(OUTDIR)/.bin_d
OBJDIR := $(OUTDIR)/.obj
OBJDIR_D := $(OUTDIR)/.obj_d
DEPDIR := $(OUTDIR)/.dep

CPPFLAGS := -I$(INCDIR) -Wall -Wextra -O2 -std=c++17
DEBUGFLAGS = -fsanitize=address -fno-omit-frame-pointer -g
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(addprefix $(OBJDIR)/, $(notdir $(SRCS:.cpp=.o)))
OBJS_D := $(addprefix $(OBJDIR_D)/, $(notdir $(SRCS:.cpp=.o)))
DEPS := $(addprefix $(DEPDIR)/, $(notdir $(SRCS:.cpp=.d)))

$(BINDIR)/%.out: $(filter-out $(TARGET:out/.bin/%.out=out/.obj/%.o), $(OBJS)) $(OBJDIR)/%.o | $(BINDIR)
	$(CC) $(CPPFLAGS) -o $@ $^
$(BINDIR_D)/%.out: $(filter-out $(TARGET_D:out/.bin_d/%.out=out/.obj_d/%.o), $(OBJS_D)) $(OBJDIR_D)/%.o | $(BINDIR_D)
	$(CC) $(CPPFLAGS) $(DEBUGFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(DEPDIR)/%.d | $(OBJDIR) $(DEPDIR)
	$(CC) $(DEPFLAGS) $(CPPFLAGS) -c -o $@ $<
$(OBJDIR_D)/%.o: $(SRCDIR)/%.cpp $(DEPDIR)/%.d | $(OBJDIR_D) $(DEPDIR)
	$(CC) $(DEPFLAGS) $(CPPFLAGS) $(DEBUGFLAGS) -c -o $@ $<

$(DEPS):

include $(wildcard $(DEPS))

.SECONDARY:

$(BINDIR) $(OBJDIR) $(DEPDIR) $(BINDIR_D) $(OBJDIR_D):
	mkdir -p $@

.PHONY: alias
alias: $(TARGET_PUB)
	-ln -b -s -t . $(TARGET_PUB)

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
check: bin/genscript.out bin/verifier.out $(DEF_FILE)
	bin/genscript.out -f $(DEF_FILE) -o out/$(notdir DEF_FILE).script || (echo "\033[1m\033[31merror\033[m: failed to generate a script of \"$(TARGET_DEF)\""; exit 1)
	bin/verifier.out -c -f out/$(TARGET_DEF).script -o out/out.book || (echo "\033[1m\033[31merror\033[m: failed to verify the script of \"$(TARGET_DEF)\""; exit 1)

.PHONY: check-%
check-%: TARGET_DEF = $(@:check-%=%)
check-%: bin/genscript.out bin/verifier.out $(DEF_FILE)
	bin/genscript.out -f $(DEF_FILE) -t $(TARGET_DEF) -o out/$(TARGET_DEF).script || (echo "\033[1m\033[31merror\033[m: failed to generate a script of \"$(TARGET_DEF)\""; exit 1)
	bin/verifier.out -c -f out/$(TARGET_DEF).script -o out/out.book || (echo "\033[1m\033[31merror\033[m: failed to verify the script of \"$(TARGET_DEF)\""; exit 1)

# test commands
.PHONY: test test-%
test: out/.bin_d/test.out $(DEF_FILE)
	@$< || (echo "\033[1m\033[31merror\033[m: test.out exited with an error."; exit 1)
	@echo "\033[1m\033[32mpassed\033[m: test.cpp"

test-conv: out/.bin_d/def_conv.out $(DEF_FILE)
	@$< -c -f $(DEF_FILE) > out/def_conv_out_c || (echo "\033[1m\033[31merror\033[m: format conversion (*->c) failed."; exit 1)
	@$< -n -f $(DEF_FILE) > out/def_conv_out_n || (echo "\033[1m\033[31merror\033[m: format conversion (*->n) failed."; exit 1)
	@$< -c -f out/def_conv_out_n > out/def_conv_out_nc || (echo "\033[1m\033[31merror\033[m: format conversion (n->c) failed."; exit 1)
	@$< -n -f out/def_conv_out_c > out/def_conv_out_cn || (echo "\033[1m\033[31merror\033[m: format conversion (c->n) failed."; exit 1)
	@cmp -s out/def_conv_out_c out/def_conv_out_nc || (echo "\033[1m\033[31merror\033[m: format conversion (n->c) didn't match the reference (*->c)."; exit 1)
	@cmp -s out/def_conv_out_n out/def_conv_out_cn || (echo "\033[1m\033[31merror\033[m: format conversion (c->n) didn't match the reference (*->n)."; exit 1)
	@echo "\033[1m\033[32mpassed\033[m: conv"

test-verify: out/.bin_d/verifier.out resource/script_test resource/script_test_result
	@$< -c -f resource/script_test -o out/test-verify.tmp || (echo "\033[1m\033[31merror\033[m: book generation failed."; exit 1)
	@cmp -s out/test-verify.tmp resource/script_test_result || (echo "\033[1m\033[31merror\033[m: the result book didn't match the reference."; exit 1)
	@echo "\033[1m\033[32mpassed\033[m: verify"

test-gen: out/.bin_d/genscript.out $(DEF_FILE)
	@$< -f $(DEF_FILE) -s || (echo "\033[1m\033[31merror\033[m: script generation failed."; exit 1)
	@echo "\033[1m\033[32mpassed\033[m: gen"

test-%:
	@(echo "\033[1m\033[31merror\033[m: unknown test command: $(@:test-%=%)"; exit 1)