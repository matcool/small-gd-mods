#include <cstdint>
#include <unordered_map>
#include <MinHook.h>

namespace MHook {
    using std::uintptr_t;

    inline std::unordered_map<void*, void*> hooks;

    auto registerHook(uintptr_t address, void* hook) {
        void* trampoline;
        auto status = MH_CreateHook(reinterpret_cast<void**>(address), hook, &trampoline);
        if (status == MH_OK)
            hooks[hook] = trampoline;
        return status;
    }

    template <typename F>
    inline auto getOriginal(F self) {
        return reinterpret_cast<F>(hooks[self]);
    }
}
