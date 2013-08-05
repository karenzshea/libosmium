#ifndef OSMIUM_INDEX_MAP_MMAP_ANON_HPP
#define OSMIUM_INDEX_MAP_MMAP_ANON_HPP

/*

This file is part of Osmium (http://osmcode.org/osmium).

Copyright 2013 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#ifdef __linux__

#include <cstdlib>
#include <fcntl.h>
#include <new>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <osmium/index/map.hpp>

namespace osmium {

    namespace index {

        namespace map {

            /**
            * MmapAnon stores data in memory using the mmap() system call.
            * It will grow automatically.
            *
            * This does not work on Mac OS X, because it doesn't support mremap().
            * Use MmapFile or FixedArray on Mac OS X instead.
            *
            * If you have enough memory it is preferred to use this in-memory
            * version. If you don't have enough memory or want the data to
            * persist, use the file-backed version MmapFile. Note that in any
            * case you need substantial amounts of memory for this to work
            * efficiently.
            */
            template <typename TKey, typename TValue>
            class MmapAnon : public osmium::index::map::Map<TKey, TValue> {

                size_t m_size;

                TValue* m_items;

            public:

                static constexpr size_t size_increment = 10 * 1024 * 1024;

                /**
                * Create anonymous mapping without a backing file.
                * @exception std::bad_alloc Thrown when there is not enough memory.
                */
                MmapAnon() :
                    m_size(size_increment) {
                    m_items = static_cast<TValue*>(mmap(NULL, sizeof(TValue) * m_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
                    if (m_items == MAP_FAILED) {
                        throw std::bad_alloc();
                    }
                    new (m_items) TValue[m_size];
                }

                ~MmapAnon() noexcept override final {
                }

                void set(const TKey id, const TValue value) override final {
                    if (static_cast<size_t>(id) >= m_size) {
                        size_t new_size = id + size_increment;

                        m_items = static_cast<TValue*>(mremap(m_items, sizeof(TValue) * m_size, sizeof(TValue) * new_size, MREMAP_MAYMOVE));
                        if (m_items == MAP_FAILED) {
                            throw std::bad_alloc();
                        }
                        new (m_items + m_size) TValue[new_size - m_size];
                        m_size = new_size;
                    }
                    m_items[id] = value;
                }

                const TValue get(const TKey id) const override final {
                    if (static_cast<size_t>(id) >= m_size) {
                        throw std::out_of_range("ID outside of allowed range");
                    }
                    if (m_items[id] == TValue()) {
                        throw std::out_of_range("Unknown ID");
                    }
                    return m_items[id];
                }

                size_t size() const override final {
                    return m_size;
                }

                size_t used_memory() const override final {
                    return m_size * sizeof(TValue);
                }

                void clear() override final {
                    munmap(m_items, sizeof(TValue) * m_size);
                    m_items = nullptr;
                    m_size = 0;
                }

            }; // class MmapAnon

        } // namespace map

    } // namespace index

} // namespace osmium

#else
#  warning "osmium::index::map::MmapAnon only works on Linux!"
#endif // __linux__

#endif // OSMIUM_INDEX_MAP_MMAP_ANON_HPP
