#include <functional>
#include <sys/mman.h>
#include <boost/callable_traits/function_type.hpp>

template <typename T> struct FunctionWrapper;

template <typename R, typename... PS> struct FunctionWrapper<R(PS...)> {
  short mov_r10 = 0xba49;
  std::function<R(PS...)> *func;
  short mov_rax = 0xb848;
  void *ptr = (void *)wrapper;
  short jmp = 0xe0ff;

  template<typename X>
  FunctionWrapper(X func)
      : func(new std::function<R(PS...)>(func)) {}

  auto as_pointer() { return (R(*)(PS...))this; }

  static auto wrapper(PS... ps) {
    std::function<R(PS...)> *func;
    asm("movq %%r10, %0" : "=r"(func));
    return (*func)(ps...);
  }
} __attribute__((packed));

void *alloc_executable_memory(size_t size) {
  void *ptr = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == (void *)-1) {
    perror("mmap");
    return NULL;
  }
  return ptr;
}

static char *buffer = (char *)alloc_executable_memory(200 * sizeof(FunctionWrapper<int()>));
static size_t pos   = 0;

template <typename T> auto gen_function(T from) {
  return new (buffer + (sizeof(FunctionWrapper<int()>) * pos++)) FunctionWrapper<boost::callable_traits::function_type_t<T>>(from);
}