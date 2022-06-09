#include <elf/image.h>
#include <handover/builder.h>
#include <karm-base/size.h>
#include <karm-sys/chan.h>
#include <karm-sys/file.h>
#include <karm-sys/mmap.h>

#include <arch-x86_64/com.h>

#include "fw.h"
#include "loader.h"

namespace Loader {

Error load(Sys::Path kernelPath) {
    auto vmm = try$(Fw::createVmm());

    Sys::println("Preparing payload...");
    auto payloadMem = try$(Sys::mmap().read().size(16 * 1024).mapMut());
    Handover::Builder payload{payloadMem.slice<uint8_t>()};
    payload.agent("loader");
    payload.add(Handover::SELF, 0, payloadMem.prange());

    Sys::println("Loading kernel file...");
    Sys::File kernelFile = try$(Sys::File::open(kernelPath));
    auto kernelMem = try$(Sys::mmap().map(kernelFile));
    Elf::Image image{kernelMem.slice<uint8_t>()};
    payload.add(Handover::FILE, 0, kernelMem.prange());

    if (!image.valid()) {
        return {"invalid kernel image"};
    }

    Sys::println("Setting up stack...");
    auto stackMap = try$(Sys::mmap().stack().size(16 * 1024).mapMut());
    payload.add(Handover::STACK, 0, stackMap.prange());

    Sys::println("Loading kernel image...");
    for (auto prog : image.programs()) {
        if (prog.type() != Elf::Program::LOAD) {
            continue;
        }

        size_t paddr = prog.vaddr() - Handover::KERNEL_BASE;
        size_t remaining = prog.memsz() - prog.filez();
        memcpy((void *)paddr, prog.buf(), prog.filez());
        memset((void *)(paddr + prog.filez()), 0, remaining);

        payload.add(Handover::KERNEL, 0, {paddr, prog.memsz()});
    }

    Sys::println("Handling kernel requests...");
    auto requests = try$(image.sectionByName(Handover::REQUEST_SECTION)).slice<Handover::Request>();

    for (auto const &request : requests) {
        Sys::println("Handing over {}...", request.name());
    }

    Sys::println("Mapping kernel space...");
    try$(vmm->map({Handover::KERNEL_BASE, Handover::KERNEL_BASE + gib(2)}, {0, gib(2)}, Hal::Vmm::READ | Hal::Vmm::WRITE | Hal::Vmm::EXEC));

    Sys::println("Mapping upper space...");
    try$(vmm->map({Handover::UPPER_HALF, Handover::UPPER_HALF + gib(4)}, {0, gib(4)}, Hal::Vmm::READ | Hal::Vmm::WRITE));

    Sys::println("Mapping loader space...");
    try$(vmm->map({0, mib(512)}, {0, mib(512)}, Hal::Vmm::READ | Hal::Vmm::WRITE));

    Sys::println("Finishing up...");
    payloadMem.leak();
    kernelMem.leak();
    stackMap.leak();

    x86_64::Com com{x86_64::Com::COM1};

    Sys::println("Entering kernel, see on the other side...");
    try$(Fw::finalizeHandover(payload));
    vmm->activate();
    com.write("Still alive!\n", 13);

    Handover::EntryPoint entryPoint = (Handover::EntryPoint)(size_t)image.header().entry;
    entryPoint(Handover::COOLBOOT, payload.finalize());
    panic("unreachable");
}

} // namespace Loader
