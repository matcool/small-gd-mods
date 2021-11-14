#include <Windows.h>

DWORD WINAPI func(void* me) {
    auto proc = GetCurrentProcess();
    auto base = reinterpret_cast<char*>(GetModuleHandle(NULL));
    using u8 = unsigned char;
    static const u8 patch1[] = {0xEB, 0x19};
    WriteProcessMemory(proc, base + 0x1518d7, patch1, 2, NULL);
    static const u8 patch2[] = {0xEB, 0x09};
    WriteProcessMemory(proc, base + 0x15191a, patch2, 2, NULL);
    static const u8 patch3[] = {0xEB, 0x11};
    WriteProcessMemory(proc, base + 0x15192b, patch3, 2, NULL);

    FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(me), 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH)
        if (auto h = CreateThread(NULL, 0, &func, mod, 0, NULL))
            CloseHandle(h);
    return TRUE;
}