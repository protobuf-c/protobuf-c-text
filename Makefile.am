ACLOCAL_AMFLAGS = -I m4 --install
PROTOBUF_C_TEST_SRCS = t/addressbook.pb-c.c
PROTOBUF_C_TEST_HDRS = $(patsubst %.c,%.h,$(PROTOBUF_C_TEST_SRCS) \
				          $(PROTOBUF_C_TEST_SRCS))
AM_CFLAGS = -I $(srcdir)/protobuf-c-text
export GREP

lib_LTLIBRARIES = protobuf-c-text/libprotobuf-c-text.la
TESTS = $(srcdir)/t/test_generation.sh $(srcdir)/t/test_parse.sh t/test_msgs
check_PROGRAMS = t/c-dump t/c-dump2 t/c-parse t/c-parse2 t/test_msgs
check_SCRIPTS = t/test_generation.sh t/test_parse.sh
BUILT_SOURCES = $(PROTOBUF_C_TEST_SRCS) $(PROTOBUF_C_TEST_HDRS) \
		protobuf-c-text/parse.c
noinst_HEADERS = protobuf-c-text/protobuf-c-util.h

dist_man3_MANS = man/libprotobuf-c-text.3 \
		 man/protobuf_c_text_from_file.3 \
		 man/protobuf_c_text_from_string.3 \
		 man/protobuf_c_text_to_string.3

# Libraries.
nodist_protobuf_c_text_libprotobuf_c_text_la_SOURCES = \
    protobuf-c-text/parse.c
protobuf_c_text_libprotobuf_c_text_la_SOURCES = \
    protobuf-c-text/generate.c \
    protobuf-c-text/parse.re
protobuf_c_text_libprotobuf_c_text_la_CFLAGS = $(AM_CFLAGS) $(COVERAGE_CFLAGS)
# Version updating rules:
# 1. If the library source code has changed but interfaces are the same,
#    then increment revision (c:r:a becomes c:r+1:a).
# 2. If any interfaces have been removed or changed since the last release,
#    set revision and age to 0 and increment current (c:r:a becomes c+1:0:0).
# 3. If interfaces have only been added since the last release, set
#    release to 0 and increment current and age (c:r:a becomes c+1:0:a+1).
protobuf_c_text_libprotobuf_c_text_la_LDFLAGS = \
	$(COVERAGE_LDFLAGS) -version-info 2:0:0

# Headers for libraries.
libprotobuf_c_textdir = $(includedir)/google/protobuf-c
libprotobuf_c_text_HEADERS = protobuf-c-text/protobuf-c-text.h
pkgconfig_DATA = protobuf-c-text/libprotobuf-c-text.pc

# Static analysis.
analyze_srcs = protobuf-c-text/generate.c protobuf-c-text/parse.c
.PHONY: analyze
analyze: all
	@for f in $(analyze_srcs); do \
	  if test "@AM_V@" = 1; then \
	    echo clang --analyze -x c -I build-aux $$f -o /dev/null; \
	  else \
	    echo "  CCSA  " $$f; \
	  fi; \
	  clang --analyze -x c -I build-aux $$f -o /dev/null; \
	done

# Test programs.
t_c_dump_SOURCES = t/c-dump.c t/broken-alloc.h t/broken-alloc.c
nodist_t_c_dump_SOURCES = $(PROTOBUF_C_TEST_SRCS)
t_c_dump_CFLAGS = $(AM_CFLAGS) $(COVERAGE_CFLAGS) -I t @CHECK_CFLAGS@
t_c_dump_LDADD = protobuf-c-text/libprotobuf-c-text.la $(COVERAGE_LDFLAGS) @CHECK_LIBS@
t_c_dump_LDFLAGS = -static

t_c_dump2_SOURCES = t/c-dump2.c t/broken-alloc.h t/broken-alloc.c
nodist_t_c_dump2_SOURCES = $(PROTOBUF_C_TEST_SRCS)
t_c_dump2_CFLAGS = $(AM_CFLAGS) $(COVERAGE_CFLAGS) -I t @CHECK_CFLAGS@
t_c_dump2_LDADD = protobuf-c-text/libprotobuf-c-text.la $(COVERAGE_LDFLAGS) @CHECK_LIBS@
t_c_dump2_LDFLAGS = -static

t_c_parse_SOURCES = t/c-parse.c t/broken-alloc.h t/broken-alloc.c
nodist_t_c_parse_SOURCES = $(PROTOBUF_C_TEST_SRCS)
t_c_parse_CFLAGS = $(AM_CFLAGS) $(COVERAGE_CFLAGS) -I t @CHECK_CFLAGS@
t_c_parse_LDADD = protobuf-c-text/libprotobuf-c-text.la $(COVERAGE_LDFLAGS) @CHECK_LIBS@
t_c_parse_LDFLAGS = -static

t_c_parse2_SOURCES = t/c-parse2.c t/broken-alloc.h t/broken-alloc.c
nodist_t_c_parse2_SOURCES = $(PROTOBUF_C_TEST_SRCS)
t_c_parse2_CFLAGS = $(AM_CFLAGS) $(COVERAGE_CFLAGS) -I t @CHECK_CFLAGS@
t_c_parse2_LDADD = protobuf-c-text/libprotobuf-c-text.la $(COVERAGE_LDFLAGS) @CHECK_LIBS@
t_c_parse2_LDFLAGS = -static

t_test_msgs_SOURCES = t/test_msgs.c
nodist_t_test_msgs_SOURCES = $(PROTOBUF_C_TEST_SRCS)
t_test_msgs_CFLAGS = $(AM_CFLAGS) $(COVERAGE_CFLAGS) -I t @CHECK_CFLAGS@
t_test_msgs_LDADD = protobuf-c-text/libprotobuf-c-text.la \
		    $(COVERAGE_LDFLAGS) @CHECK_LIBS@
t_test_msgs_LDFLAGS = -static

EXTRA_DIST = t/addressbook.proto \
	     t/addressbook.data \
	     t/tutorial_test.data \
	     protobuf-c-text/libprotobuf-c-text.pc.in \
	     $(check_SCRIPTS) \
	     t/broken/bareword-instead-of-string.text \
	     t/broken/float-for-int.text \
	     t/broken/int-too-big.text \
	     t/broken/message-for-int.text \
	     t/broken/negative-in-unsigned.text \
	     t/broken/non-existant-field.text

# Define macros to have quiet protoc-c statements.
AM_V_PROTOC_C = $(AM_V_PROTOC_C_@AM_V@)
AM_V_PROTOC_C_ = $(AM_V_PROTOC_C_@AM_DEFAULT_V@)
AM_V_PROTOC_C_0 = @echo "  PBC_C " $<;
%.pb-c.c %.pb-c.h: %.proto
	$(AM_V_PROTOC_C)$(PROTOC_C) --proto_path=$(dir $<) --c_out=$(dir $@) $<

# Define macros to have quiet re2c statements.
AM_V_RE2C = $(AM_V_RE2C_@AM_V@)
AM_V_RE2C_ = $(AM_V_RE2C_@AM_DEFAULT_V@)
AM_V_RE2C_0 = @echo "  RE2C  " $<;
%.c: %.re
	$(AM_V_RE2C)$(RE2C) -s -o $@ $<

include am/aminclude_coverage.am
include am/aminclude_doxygen.am

.PHONY: gh-pages
if HAVE_GCOV
gh-pages: clean
	test `git rev-parse --abbrev-ref HEAD` = master
	rm -f $(PWD)/.git/index.gh-pages
	$(MKDIR_P) docs/html
	GIT_INDEX_FILE=$(PWD)/.git/index.gh-pages git --work-tree $(PWD)/docs/html checkout gh-pages
	$(MAKE) doxygen-doc coverage-html || (git co master; exit 1)
	GIT_INDEX_FILE=$(PWD)/.git/index.gh-pages git --work-tree $(PWD)/docs/html checkout .nojekyll
	GIT_INDEX_FILE=$(PWD)/.git/index.gh-pages git --work-tree $(PWD)/docs/html checkout CNAME
	GIT_INDEX_FILE=$(PWD)/.git/index.gh-pages git --work-tree $(PWD)/docs/html add .
	GIT_INDEX_FILE=$(PWD)/.git/index.gh-pages git --work-tree $(PWD)/docs/html ls-files --deleted | GIT_INDEX_FILE=$(PWD)/.git/index.gh-pages xargs -r git --work-tree $(PWD)/docs/html rm
	GIT_INDEX_FILE=$(PWD)/.git/index.gh-pages git --work-tree $(PWD)/docs/html commit -m "Update docs."
	git checkout master
else
gh-pages:
	@echo "ERROR: Need to run ./configure --enable-gcov"
	@false
endif # HAVE_GCOV

.PHONY: changelog
changelog:
	@git shortlog --pretty='* [%h] %s' -e `git log -n 1 --pretty='%H' -- ChangeLog`..HEAD > ChangeLog.new

# Add things to remove for the "distclean" target.
distclean-local: distclean-local-ac
.PHONY: distclean-local-ac
distclean-local-ac:
	@rm -rf autom4te.cache build-aux/*~ m4/*~

# Add things to remove for the "clean" target.
clean-local: clean-local-build-dirs clean-coverage-html clean-gcno
.PHONY: clean-local-build-dirs
clean-local-build-dirs:
	@rm -rf bin lib tests
CLEANFILES = protobuf-c-text/libprotobuf-c-text.pc
MOSTLYCLEANFILES = $(DX_CLEANFILES) \
		   $(BUILT_SOURCES) \
		   $(PROTOBUF_C_TEST_HDRS) \
		   t/addressbook.c.text \
		   t/addressbook.protoc-c.text \
		   t/addressbook.c.data \
		   t/broken_parse.data \
		   t/broken_parse.out \
		   t/tutorial_test.c.data \
		   t/tutorial_test.c.text \
		   t/tutorial_test.protoc_c.text
