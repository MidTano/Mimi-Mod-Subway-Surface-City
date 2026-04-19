#include "hooks.h"
#include "menu.h"
#include "mod_features.h"

#include <EGL/egl.h>
#include <android/native_window.h>
#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <sys/mman.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <cstring>

namespace {

using EglSwapBuffersFn = EGLBoolean (*)(EGLDisplay display, EGLSurface surface);
using EglCreateWindowSurfaceFn = EGLSurface (*)(EGLDisplay, EGLConfig, ANativeWindow*, const EGLint*);

std::atomic<bool> g_modInitialized{false};
std::atomic<bool> g_hookInstalled{false};
std::uint64_t g_frameCounter = 0;
int g_hookAttempts = 0;
EglSwapBuffersFn g_originalEglSwapBuffers = nullptr;
EglCreateWindowSurfaceFn g_originalEglCreateWindowSurface = nullptr;
std::atomic<ANativeWindow*> g_capturedNativeWindow{nullptr};

EGLSurface hookedEglCreateWindowSurface(EGLDisplay display, EGLConfig config, ANativeWindow* window, const EGLint* attribs) {
    g_capturedNativeWindow.store(window);
    if (g_originalEglCreateWindowSurface) {
        return g_originalEglCreateWindowSurface(display, config, window, attribs);
    }
    auto realFn = reinterpret_cast<EglCreateWindowSurfaceFn>(
        dlsym(RTLD_NEXT, "eglCreateWindowSurface"));
    if (realFn) return realFn(display, config, window, attribs);
    return EGL_NO_SURFACE;
}

template <typename T>
T dynPtr(ElfW(Addr) base, ElfW(Addr) value) {
    if (value < base) {
        return reinterpret_cast<T>(base + value);
    }
    return reinterpret_cast<T>(value);
}

void initializeModFromRenderThread() {
    if (g_modInitialized.exchange(true)) {
        return;
    }
    Menu::MenuRenderer::getInstance().initialize();
}

EGLBoolean hookedEglSwapBuffers(EGLDisplay display, EGLSurface surface) {
    if (g_originalEglSwapBuffers == nullptr) {
        return EGL_FALSE;
    }

    ++g_frameCounter;

    initializeModFromRenderThread();
    Features::FeatureManager::getInstance().update();
    Menu::MenuRenderer::getInstance().render();

    return g_originalEglSwapBuffers(display, surface);
}

bool patchGotEntry(ElfW(Addr) base, const char* strtab, ElfW(Sym)* symtab, const ElfW(Rela)& reloc, const char* moduleName) {
    const std::size_t symbolIndex = static_cast<std::size_t>(ELF64_R_SYM(reloc.r_info));
    const char* symbolName = strtab + symtab[symbolIndex].st_name;

    void* newFn = nullptr;
    void** savedOriginal = nullptr;
    if (std::strcmp(symbolName, "eglSwapBuffers") == 0) {
        newFn = reinterpret_cast<void*>(hookedEglSwapBuffers);
        savedOriginal = reinterpret_cast<void**>(&g_originalEglSwapBuffers);
    } else if (std::strcmp(symbolName, "eglCreateWindowSurface") == 0) {
        newFn = reinterpret_cast<void*>(hookedEglCreateWindowSurface);
        savedOriginal = reinterpret_cast<void**>(&g_originalEglCreateWindowSurface);
    } else {
        return false;
    }

    auto* gotEntry = reinterpret_cast<void**>(base + reloc.r_offset);
    void* current = *gotEntry;

    if (current != newFn && current != nullptr) {
        *savedOriginal = current;
    }

    if (current == newFn) {
        return true;
    }

    const long pageSize = sysconf(_SC_PAGESIZE);
    const uintptr_t pageStart = reinterpret_cast<uintptr_t>(gotEntry) & ~(static_cast<uintptr_t>(pageSize - 1));

    if (mprotect(reinterpret_cast<void*>(pageStart), static_cast<size_t>(pageSize), PROT_READ | PROT_WRITE) != 0) {
        return false;
    }

    *gotEntry = newFn;
    __builtin___clear_cache(reinterpret_cast<char*>(pageStart), reinterpret_cast<char*>(pageStart + pageSize));

    mprotect(reinterpret_cast<void*>(pageStart), static_cast<size_t>(pageSize), PROT_READ);

    return true;
}

bool patchModulePlt(const dl_phdr_info* info, const char* moduleName) {
    if (info->dlpi_name == nullptr || std::strstr(info->dlpi_name, moduleName) == nullptr) {
        return false;
    }

    ElfW(Addr) base = info->dlpi_addr;
    const ElfW(Phdr)* phdr = info->dlpi_phdr;
    ElfW(Dyn)* dynamic = nullptr;

    for (ElfW(Half) i = 0; i < info->dlpi_phnum; ++i) {
        if (phdr[i].p_type == PT_DYNAMIC) {
            dynamic = reinterpret_cast<ElfW(Dyn)*>(base + phdr[i].p_vaddr);
            break;
        }
    }

    if (dynamic == nullptr) {
        return false;
    }

    const char* strtab = nullptr;
    ElfW(Sym)* symtab = nullptr;
    ElfW(Rela)* jmprel = nullptr;
    std::size_t pltrelSize = 0;
    std::size_t pltType = DT_RELA;

    for (ElfW(Dyn)* d = dynamic; d->d_tag != DT_NULL; ++d) {
        switch (d->d_tag) {
            case DT_STRTAB:
                strtab = dynPtr<const char*>(base, d->d_un.d_ptr);
                break;
            case DT_SYMTAB:
                symtab = dynPtr<ElfW(Sym)*>(base, d->d_un.d_ptr);
                break;
            case DT_JMPREL:
                jmprel = dynPtr<ElfW(Rela)*>(base, d->d_un.d_ptr);
                break;
            case DT_PLTRELSZ:
                pltrelSize = static_cast<std::size_t>(d->d_un.d_val);
                break;
            case DT_PLTREL:
                pltType = static_cast<std::size_t>(d->d_un.d_val);
                break;
            default:
                break;
        }
    }

    if (strtab == nullptr || symtab == nullptr || jmprel == nullptr || pltrelSize == 0) {
        return false;
    }

    if (pltType != DT_RELA) {
        return false;
    }

    const std::size_t relocCount = pltrelSize / sizeof(ElfW(Rela));
    bool patched = false;
    for (std::size_t i = 0; i < relocCount; ++i) {
        if (patchGotEntry(base, strtab, symtab, jmprel[i], moduleName)) {
            patched = true;
        }
    }

    return patched;
}

struct ScanContext {
    const char* moduleName;
    bool patched;
};

int scanAndPatchCallback(dl_phdr_info* info, size_t size, void* data) {
    (void)size;
    auto* ctx = reinterpret_cast<ScanContext*>(data);
    if (ctx->patched) {
        return 1;
    }
    ctx->patched = patchModulePlt(info, ctx->moduleName);
    return ctx->patched ? 1 : 0;
}

void tryInstallPltHook() {
    if (g_hookInstalled.load()) {
        return;
    }

    ++g_hookAttempts;

    const char* modules[] = {"libmain.so", "libunity.so"};
    bool patchedAny = false;
    for (const char* module : modules) {
        ScanContext ctx{module, false};
        dl_iterate_phdr(scanAndPatchCallback, &ctx);
        if (ctx.patched) {
            patchedAny = true;
        }
    }

    if (g_originalEglSwapBuffers == nullptr) {
        g_originalEglSwapBuffers = reinterpret_cast<EglSwapBuffersFn>(dlsym(RTLD_NEXT, "eglSwapBuffers"));
    }
    if (g_originalEglCreateWindowSurface == nullptr) {
        g_originalEglCreateWindowSurface = reinterpret_cast<EglCreateWindowSurfaceFn>(
            dlsym(RTLD_NEXT, "eglCreateWindowSurface"));
    }

    if (patchedAny && g_originalEglSwapBuffers != nullptr) {
        g_hookInstalled.store(true);
    }
}

}  // namespace

namespace Hooks {

HookManager::HookManager() {
}

HookManager::~HookManager() {
}

HookManager& HookManager::getInstance() {
    static HookManager instance;
    return instance;
}

void HookManager::initialize() {
    tryInstallPltHook();
}

void HookManager::shutdown() {
}

}  // namespace Hooks
