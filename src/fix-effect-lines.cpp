#include <Windows.h>

DWORD WINAPI func(void* me) {
    auto proc = GetCurrentProcess();
    auto base = reinterpret_cast<char*>(GetModuleHandleA(nullptr));
    
    static const unsigned char patch1[] = {0x90, 0x90};
    WriteProcessMemory(proc, base + 0x16daba, patch1, sizeof(patch1), nullptr);

    FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(me), 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH)
        if (auto h = CreateThread(nullptr, 0, &func, mod, 0, nullptr))
            CloseHandle(h);
    return TRUE;
}