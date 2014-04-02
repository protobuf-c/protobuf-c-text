[![Build Status](https://travis-ci.org/lyda/protobuf-c-text.png?branch=master)](https://travis-ci.org/lyda/protobuf-c-text)

# Protobuf Text Format Support for C

Python, C++ and Java protobufs support text format but C protobufs do not.
This is a project to try and fix that. The end goal is to merge this
into the [protobuf-c](https://github.com/protobuf-c/protobuf-c) project.

## Contents

The `protobuf-c-text/` directory has the code for the library.  Tests
are in `t/`.

## Dependencies

The `re2c` parser is required to generate the lexer (`parser.re`).

## Testing

The `t/c-*` programs use the `BROKEN_MALLOC` and `BROKEN_MALLOC_SEGV`
environment vars to control when and how malloc will fail.  The
`BROKEN_MALLOC` is set to the number of times for malloc to succeed until
it fails.  When the `BROKEN_MALLOC_SEGV` var is set the test program will
segfault on the first failure.  This is useful for tracking down errors.

Note that the error message will print out the `gdb` line and the `run`
command you need to issue to reproduce the error.

## Doxygen Docs

These can be generated locally in `docs/doxygen/html`. From time to time
they are
[published to the `gh-pages` branch](https://github.com/lyda/protobuf-c-text).
Unfortunately I haven't yet automated this so the steps are rather
involved.

The initial steps was done as follows.  This should not need to be repeated
but is documented here for future projects.

```bash
mkdir foo
git checkout --orphan gh-pages
GIT_INDEX_FILE=$PWD/.git/index.gh-pages git --work-tree foo status
touch foo/.nojekyll
GIT_INDEX_FILE=$PWD/.git/index.gh-pages git --work-tree foo add .nojekyll
GIT_INDEX_FILE=$PWD/.git/index.gh-pages git --work-tree foo commit -m 'Turn off Jekyll'
git checkout master
rm -rf foo
```

Subsequent updates are done like so (starting in `master`):

```bash
make doxygen-doc
GIT_INDEX_FILE=$PWD/.git/index.gh-pages git --work-tree $PWD/docs/doxygen/html co gh-pages
GIT_INDEX_FILE=$PWD/.git/index.gh-pages git --work-tree $PWD/docs/doxygen/html co .nojekyl
GIT_INDEX_FILE=$PWD/.git/index.gh-pages git --work-tree $PWD/docs/doxygen/html add .
# In subsequent edits we might need to remove things?
GIT_INDEX_FILE=$PWD/.git/index.gh-pages git --work-tree $PWD/docs/doxygen/html ci -m "Update doxygen-docs."
```

Note that all references to files are done relative to the dir specified
in `--work-tree`.  Changing dir into that would make things easier, but
the `--work-tree` flag still needs to be set as would `GIT_INDEX_FILE`.

Also for reference,
[github pages docs](https://help.github.com/categories/20/articles)
are quite handy.
