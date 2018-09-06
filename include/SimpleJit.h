#include <functional>
#include <sys/mman.h>

template <typename R, typename... PS> struct FunctionWrapper {
  unsigned char push = 0x68;
  std::function<R(PS...)> *func;
  unsigned char call = 0xe8;
  unsigned offset    = ((char *)wrapper - (char *)this - offsetof(FunctionWrapper<int>, offset) - 4);
  unsigned addesp    = 0xc304c483;

  FunctionWrapper(std::function<R(PS...)> func)
      : func(new std::function<R(PS...)>(func)) {}

  auto as_pointer() { return (R(*)(PS...))this; }

  static auto wrapper(std::function<R(PS...)> &func, int s0, PS... ps) { return func(ps...); }
} __attribute__((packed));

void *alloc_executable_memory(size_t size) {
  void *ptr = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == (void *)-1) {
    perror("mmap");
    return NULL;
  }
  return ptr;
}

static char *buffer = (char *)alloc_executable_memory(200 * sizeof(FunctionWrapper<int>));
static size_t pos   = 0;

template <typename R, typename... PS> auto gen_function(std::function<R(PS...)> from) {
  return new (buffer + (sizeof(FunctionWrapper<int>) * pos++)) FunctionWrapper<R, PS...>(from);
}