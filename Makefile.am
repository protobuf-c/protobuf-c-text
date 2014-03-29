ACLOCAL_AMFLAGS = -I m4 --install
PROTOBUF_C_TEST_SRCS = t/addressbook.pb-c.c
PROTOBUF_C_TEST_HDRS = $(patsubst %.c,%.h,$(PROTOBUF_C_TEST_SRCS) \
				          $(PROTOBUF_C_TEST_SRCS))

lib_LTLIBRARIES = protobuf-c-text/libprotobuf-c-text.la
TESTS = t/test_generation.sh t/test_parse.sh
check_PROGRAMS = t/c-dump t/c-parse
check_SCRIPTS = t/test_generation.sh
BUILT_SOURCES = $(PROTOBUF_C_TEST_SRCS) $(PROTOBUF_C_TEST_SRCS) \
		protobuf-c-text/parse.c
noinst_HEADERS = protobuf-c-text/protobuf-c-util.h

#dist_man3_MANS = man/protobuf-c-text.3

valgrind: $(check_PROGRAMS)
	@t/valgrind.test_generation.sh

# Libraries.
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
#    release to 0 and increment release and age (c:r:a becomes c+1:0:a+1).
protobuf_c_text_libprotobuf_c_text_la_LDFLAGS = $(COVERAGE_LDFLAGS) -version-info 0:0:0

# Headers for libraries.
libprotobuf_c_textdir = $(includedir)/google/protobuf-c
libprotobuf_c_text_HEADERS = protobuf-c-text/protobuf-c-text.h
pkgconfig_DATA = protobuf-c-text/libprotobuf-c-text.pc

# Static analysis.
analyze_srcs = $(protobuf_c_text_libprotobuf_c_text_la_SOURCES)
.PHONY: analyze
analyze: all
	@for f in $(analyze_srcs); do \
	  if test "$(AM_V)" = 1; then \
	    echo clang --analyze -I proto -I src $$f -o /dev/null; \
	  else \
	    echo "  CCSA  " $$f; \
	  fi; \
	  clang --analyze -I proto -I src $$f -o /dev/null; \
	done

# Test programs.
t_c_dump_SOURCES = $(PROTOBUF_C_TEST_SRCS) t/c-dump.c
t_c_dump_CFLAGS = $(AM_CFLAGS) $(COVERAGE_CFLAGS) @CHECK_CFLAGS@
t_c_dump_LDADD = protobuf-c-text/libprotobuf-c-text.la $(COVERAGE_LDFLAGS) @CHECK_LIBS@
t_c_dump_LDFLAGS = -static

t_c_parse_SOURCES = $(PROTOBUF_C_TEST_SRCS) t/c-parse.c
t_c_parse_CFLAGS = $(AM_CFLAGS) $(COVERAGE_CFLAGS) @CHECK_CFLAGS@
t_c_parse_LDADD = protobuf-c-text/libprotobuf-c-text.la $(COVERAGE_LDFLAGS) @CHECK_LIBS@
t_c_parse_LDFLAGS = -static

EXTRA_DIST = t/addressbook.proto \
	     t/addressbook.data \
	     protobuf-c-text/libprotobuf-c-text.pc.in

# Define macros to have quiet protoc-c statements.
AM_V_PROTOC_C = $(AM_V_PROTOC_C_@AM_V@)
AM_V_PROTOC_C_ = $(AM_V_PROTOC_C_@AM_DEFAULT_V@)
AM_V_PROTOC_C_0 = @echo "  PBC_C " $<;
%.pb-c.c %.pb-c.h: %.proto
	$(AM_V_PROTOC_C)$(PROTOC_C) --proto_path=$(dir $<) --c_out=$(dir $@) $<
	@indent -kr -nut -i2 $(patsubst %.proto,%.pb-c.c,$<)
	@indent -kr -nut -i2 $(patsubst %.proto,%.pb-c.h,$<)
	@rm -f $(patsubst %.proto,%.pb-c.c~,$<) $(patsubst %.proto,%.pb-c.h~,$<)

# Define macros to have quiet re2c statements.
AM_V_RE2C = $(AM_V_RE2C_@AM_V@)
AM_V_RE2C_ = $(AM_V_RE2C_@AM_DEFAULT_V@)
AM_V_RE2C_0 = @echo "  RE2C  " $<;
%.c: %.re
	$(AM_V_RE2C)$(RE2C) -s -o $(patsubst %.re,%.c,$<) $<

include am/aminclude_coverage.am
include am/aminclude_doxygen.am

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
		   t/addressbook.c.data
