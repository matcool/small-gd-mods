#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fstream>
#include <iostream>
#include <string>
#include <errhandlingapi.h>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <DbgHelp.h>
#include <filesystem>

std::string getModuleName(HMODULE module, bool fullPath = true) {
	char buffer[MAX_PATH];
	if (!GetModuleFileNameA(module, buffer, MAX_PATH))
		return "Unknown";
	if (fullPath)
		return buffer;
	return std::filesystem::path(buffer).filename().string();
}

std::string getExceptionCodeString(DWORD code) {
	#define _a(c) case c: return #c;
	// the _a stands for _amazing
	switch (code) {
		_a(EXCEPTION_ACCESS_VIOLATION)
		_a(EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
		_a(EXCEPTION_STACK_OVERFLOW)
		_a(EXCEPTION_ILLEGAL_INSTRUCTION)
		_a(EXCEPTION_IN_PAGE_ERROR)
		_a(EXCEPTION_BREAKPOINT)
		_a(EXCEPTION_DATATYPE_MISALIGNMENT)
		_a(EXCEPTION_FLT_DENORMAL_OPERAND)
		_a(EXCEPTION_FLT_DIVIDE_BY_ZERO)
		_a(EXCEPTION_FLT_INEXACT_RESULT)
		_a(EXCEPTION_FLT_INVALID_OPERATION)
		_a(EXCEPTION_FLT_OVERFLOW)
		_a(EXCEPTION_INT_DIVIDE_BY_ZERO)
		default: return "idk lol";
	}

	#undef _a
}

void printAddr(std::ostream& stream, const void* addr, bool fullPath = true) {
	HMODULE module = NULL;

	if(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCTSTR)addr, &module)) {
		const auto diff = reinterpret_cast<uintptr_t>(addr) - reinterpret_cast<uintptr_t>(module);
		stream << getModuleName(module, fullPath) << " + " << std::hex << diff << std::dec;
	} else {
		stream << addr;
	}
}

// https://stackoverflow.com/a/50208684/9124836
void walkStack(std::ostream& stream, PCONTEXT context) {
    STACKFRAME64 stack;
	memset(&stack, 0, sizeof(STACKFRAME64));

	auto process = GetCurrentProcess();
	auto thread = GetCurrentThread();
	stack.AddrPC.Offset = context->Eip;
	stack.AddrPC.Mode = AddrModeFlat;
	stack.AddrStack.Offset = context->Esp;
	stack.AddrStack.Mode = AddrModeFlat;
	stack.AddrFrame.Offset = context->Ebp;
	stack.AddrFrame.Mode = AddrModeFlat;

    // size_t frame = 0;
    while (true) {
        auto result = StackWalk64(
			IMAGE_FILE_MACHINE_I386,
			process,
			thread,
			&stack,
			context,
			NULL,
			SymFunctionTableAccess64,
			SymGetModuleBase64,
			NULL);

		if (!result) break;

		stream << " - ";
        printAddr(stream, reinterpret_cast<void*>(stack.AddrPC.Offset));
        stream << std::endl;
    }
}

LONG WINAPI handler(EXCEPTION_POINTERS* info) {
    std::ofstream file;
    file.open(std::filesystem::path(getModuleName(GetModuleHandle(0), true)).parent_path() / "crash-log.log", std::ios::app);

    const auto now = std::time(nullptr);
	const auto tm = *std::localtime(&now);
    file << std::put_time(&tm, "[%d/%m/%Y %H:%M:%S]") << std::endl;
	file << std::showbase;
	file << "Oh no! an exception has occurred. anyways heres some info" << std::endl;
	file << "Module Name: " << getModuleName(GetModuleHandle(0), false) << std::endl;
	file << "Exception Code: " << std::hex << info->ExceptionRecord->ExceptionCode \
     << " (" << getExceptionCodeString(info->ExceptionRecord->ExceptionCode) << ")" << std::dec << std::endl;
	file << "Exception Flags: " << info->ExceptionRecord->ExceptionFlags << std::endl;
	file << "Exception Address: " << info->ExceptionRecord->ExceptionAddress << " (";
	printAddr(file, info->ExceptionRecord->ExceptionAddress, false);
    file << ")" << std::endl;
	file << "Number Parameters: " << info->ExceptionRecord->NumberParameters << std::endl;
	file << "Stack Trace:" << std::endl;
    walkStack(file, info->ContextRecord);
	return EXCEPTION_CONTINUE_SEARCH;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
	    SetUnhandledExceptionFilter(handler);
    }
    return TRUE;
}