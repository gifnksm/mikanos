#include <cerrno>
#include <new>

void *operator new(size_t) { return nullptr; }

void operator delete(void *) noexcept {}

void operator delete(void *, std::align_val_t) noexcept {}

std::new_handler std::get_new_handler() noexcept { return nullptr; }

extern "C" int posix_memalign(void **, size_t, size_t) { return ENOMEM; }
