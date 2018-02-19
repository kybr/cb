
#define ensure(x, ...)                                                  \
  do {                                                                  \
    if (!x) {                                                           \
      fprintf(stderr, "ERROR| in %s at line %d\n", __FILE__, __LINE__); \
      fprintf(stderr, "ERROR| trying " #x);                             \
    }                                                                   \
  } while (0)
