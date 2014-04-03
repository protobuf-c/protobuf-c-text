[![Build Status](https://travis-ci.org/protobuf-c/protobuf-c-text.png?branch=master)](https://travis-ci.org/protobuf-c/protobuf-c-text)

# Protobuf Text Format Support for C

Python, C++ and Java protobufs support text format but C protobufs do not.
This is a project to try and fix that. The end goal is to merge this
into the [protobuf-c](https://github.com/protobuf-c) project.

## Contents

The `protobuf-c-text/` directory has the code for the library.  Tests
are in `t/`.

## Dependencies

The `re2c` parser is required to generate the lexer (`parser.re`).

## Testing

The `t/c-*` programs use the `BROKEN_MALLOC` and `BROKEN_MALLOC_SEGV`
environment vars to control when and how malloc will fail.
The `BROKEN_MALLOC` var is set to the number of times for malloc to
succeed until it fails.  When the `BROKEN_MALLOC_SEGV` var is set the
test program will segfault on the first failure.  This is useful for
tracking down errors.

Note that the error message will print out the `gdb` line and the `run`
command you need to issue to reproduce the error.

## Useful make Targets

Beyond the normal autotools make targets, the following useful targets
exist:

* `coverage-html`: If you ran `./configure --enable-gcov` this will
  generate test code coverage reports along with marked up source
  files to show what is missing.
* `analyze`: If `clang` is on your system the static analyzer from the
  llvm project will be run.
* `doxygen-doc`: This will generate local versions of the
  [docs](http://text.protobuf-c.io/) available online.  They'll be found
  in `docs/html`.

## Maintainer Notes

`make gh-pages` notes:

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
./configure --enable-gcov
make clean doxygen-doc coverage-html
GIT_INDEX_FILE=$PWD/.git/index.gh-pages git --work-tree $PWD/docs/html checkout gh-pages
GIT_INDEX_FILE=$PWD/.git/index.gh-pages git --work-tree $PWD/docs/html checkout .nojekyll
GIT_INDEX_FILE=$PWD/.git/index.gh-pages git --work-tree $PWD/docs/html checkout CNAME
GIT_INDEX_FILE=$PWD/.git/index.gh-pages git --work-tree $PWD/docs/html add .
GIT_INDEX_FILE=$PWD/.git/index.gh-pages git --work-tree $PWD/docs/html ls-files --deleted | GIT_INDEX_FILE=$PWD/.git/index.gh-pages xargs git --work-tree $PWD/docs/html rm
GIT_INDEX_FILE=$PWD/.git/index.gh-pages git --work-tree $PWD/docs/html commit -m "Update docs."
git checkout master
git push
```

Note that all references to files are done relative to the dir specified
in `--work-tree`.  Changing dir into that would make things easier, but
the `--work-tree` flag still needs to be set as would `GIT_INDEX_FILE`.

Also for reference,
[github pages docs](https://help.github.com/categories/20/articles)
are quite handy.
