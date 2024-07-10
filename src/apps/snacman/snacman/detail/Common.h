#include <utility>

template <class T_type>
using serialNvp = std::pair<const char *, T_type *>;

#define SERIAL_PARAM(name)                                                     \
    serialNvp<std::decay_t<decltype(name)>> { #name, &name }
#define SERIAL_FN_PARAM(name)                                                  \
    serialNvp<std::decay_t<decltype(name())>> { #name, &name() }

