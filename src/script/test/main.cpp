#include <api.h>

#include <chaiscript/chaiscript.hpp>
#include <functional>
#include <log.h>
#include <sys/mman.h>

struct FunctionWrapper {
  unsigned char push = 0x68;
  std::function<int()> *func;
  unsigned char call = 0xe8;
  unsigned offset    = ((char *)wrapper - (char *)this - 10);
  unsigned leave     = 0xc304c483;

  FunctionWrapper(const std::function<int()> &func)
      : func(new std::function<int()>(func)) {}

  auto as_pointer() { return (int (*)())this; }

  static int wrapper(const std::function<int()> &func) { return func(); }
} __attribute__((packed));

void *alloc_executable_memory(size_t size) {
  void *ptr = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == (void *)-1) {
    perror("mmap");
    return NULL;
  }
  return ptr;
}

char *buffer = (char *)alloc_executable_memory(200 * sizeof(FunctionWrapper));
size_t pos   = 0;

void JITTest(std::function<int()> fun) {
  auto wrapper = new (buffer + (sizeof(FunctionWrapper) * pos++)) FunctionWrapper(fun);
  auto result  = wrapper->as_pointer()();
  Log::info("TM", "JIT: %08x", result);
}

extern "C" void mod_init() {
  chaiscript::ModulePtr m(new chaiscript::Module());
  m->add(chaiscript::fun([]() { Log::info("TM", "test module invoked"); }), "test");
  m->add(chaiscript::fun(JITTest), "jit");
  loadModule(m);
}