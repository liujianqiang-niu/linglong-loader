/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <sys/stat.h>
#include <unistd.h>
#include <elf.h>

#include <iostream>
#include <cstdlib>
#include <cstdio>

namespace linglong::elf {

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ELFDATANATIVE ELFDATA2LSB
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ELFDATANATIVE ELFDATA2MSB
#else
#error "Unknown machine endian"
#endif

#define bswap_16(value) ((((value)&0xff) << 8) | ((value) >> 8))

#define bswap_32(value) \
    (((uint32_t)bswap_16((uint16_t)((value)&0xffff)) << 16) | (uint32_t)bswap_16((uint16_t)((value) >> 16)))

#define bswap_64(value) \
    (((uint64_t)bswap_32((uint32_t)((value)&0xffffffff)) << 32) | (uint64_t)bswap_32((uint32_t)((value) >> 32)))

template<typename P>
inline uint16_t file16_to_cpu(uint16_t val, const P &ehdr)
{
    if (ehdr.e_ident[EI_DATA] != ELFDATANATIVE)
        val = bswap_16(val);
    return val;
}

template<typename P>
uint32_t file32_to_cpu(uint32_t val, const P &ehdr)
{
    if (ehdr.e_ident[EI_DATA] != ELFDATANATIVE)
        val = bswap_32(val);
    return val;
}

template<typename P>
uint64_t file64_to_cpu(uint64_t val, const P &ehdr)
{
    if (ehdr.e_ident[EI_DATA] != ELFDATANATIVE)
        val = bswap_64(val);
    return val;
}

auto read_elf64(FILE *fd, Elf64_Ehdr &ehdr) -> decltype(ehdr.e_shoff + (ehdr.e_shentsize * ehdr.e_shnum))
{
    Elf64_Ehdr ehdr64;
    off_t ret = -1;

    fseeko(fd, 0, SEEK_SET);
    ret = fread(&ehdr64, 1, sizeof(ehdr64), fd);
    if (ret < 0 || (size_t)ret != sizeof(ehdr64)) {
        return -1;
    }

    ehdr.e_shoff = file64_to_cpu<Elf64_Ehdr>(ehdr64.e_shoff, ehdr);
    ehdr.e_shentsize = file16_to_cpu<Elf64_Ehdr>(ehdr64.e_shentsize, ehdr);
    ehdr.e_shnum = file16_to_cpu<Elf64_Ehdr>(ehdr64.e_shnum, ehdr);

    return (ehdr.e_shoff + (ehdr.e_shentsize * ehdr.e_shnum));
}

long GetELFSize(const std::string &path)
{
    FILE *fd;
    off_t size = -1;
    Elf64_Ehdr ehdr;

    fd = fopen(path.c_str(), "rb");
    if (fd == nullptr) {
        return -1;
    }

    do {
        auto ret = fread(ehdr.e_ident, 1, EI_NIDENT, fd);
        if (ret != EI_NIDENT) {
            break;
        }
        if ((ehdr.e_ident[EI_DATA] != ELFDATA2LSB) && (ehdr.e_ident[EI_DATA] != ELFDATA2MSB)) {
            break;
        }
        if (ehdr.e_ident[EI_CLASS] == ELFCLASS32) {
            // size = read_elf32(fd);
        } else if (ehdr.e_ident[EI_CLASS] == ELFCLASS64) {
            size = read_elf64(fd, ehdr);
        } else {
            break;
        }
    } while (false);

    fclose(fd);
    return size;
}

} // namespace linglong::elf