#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf-c/protobuf-c.h>

static void *
broken_alloc(void *allocator_data, size_t size)
{
  static int not_broken = -1;
  char *$BROKEN_MALLOC = getenv("BROKEN_MALLOC");

  if (not_broken < 0 && $BROKEN_MALLOC) {
    not_broken = atoi($BROKEN_MALLOC);
  }
  if ($BROKEN_MALLOC && not_broken > 0) {
    not_broken--;
  }
  if (not_broken) {
    return malloc(size);
  } else {
    char *$BROKEN_MALLOC_SENTINAL = getenv("BROKEN_MALLOC_SENTINAL");

    if ($BROKEN_MALLOC_SENTINAL) {
      unlink($BROKEN_MALLOC_SENTINAL);
    }
    if (getenv("BROKEN_MALLOC_SEGV")) {
      kill(getpid(), SIGSEGV);
    }
    return NULL;
  }
}

static void
broken_free(void *allocator_data, void *data)
{
  free(data);
}

ProtobufCAllocator broken_allocator = {
        .alloc = &broken_alloc,
        .free = &broken_free,
        .allocator_data = NULL,
};
