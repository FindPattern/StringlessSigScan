#include <bitset>
#include <array>
#include "fnv.hpp"

constexpr static auto k_bitness = 8 * sizeof(void*);
using bitset_ptr = std::bitset<k_bitness>;

void* find_signature(
    const void* begin,
    const void* end,
    bitset_ptr mask,
    fnv::hash hash
)
{
    const auto begin_it = (uint8_t*)(begin);
    const auto end_it = (uint8_t*)(end);

    std::array<fnv::hash, k_bitness> hashes;
    for (auto& h : hashes)
        h = fnv::hash_init();

    for (auto it = begin_it; it != end; ++it)
    {
        if (it - begin_it >= k_bitness)
        {
            auto& current = hashes[(k_bitness - std::uintptr_t(it)) % k_bitness];
            if (current == hash)
                return it - k_bitness;
            current = fnv::hash_init();
        }

        for (auto i = 0u; i < k_bitness; ++i)
        {
            const auto e = std::uintptr_t(it + i) % k_bitness;
            if (mask[e])
                hashes[i] = fnv::hash_byte(hashes[i], *it);
        }
    }

    auto last_set = 0;
    for (auto i = 0u; i < k_bitness; ++i)
        if (mask[i % mask.size()]) // FUCKING VS STOP PUTTING RANGECHECKS
            last_set = i;

    for (auto i = last_set; i < k_bitness; ++i)
        if (hashes[(k_bitness - std::uintptr_t(end_it + i - last_set)) % k_bitness] == hash)
            return end_it + i - last_set - k_bitness;

    return nullptr;
}

template<std::size_t N>
__forceinline constexpr bitset_ptr mask_from_str(const char(&str)[N])
{
    std::uintptr_t b = 0;
    for (auto i = 0u; i < N - 1; ++i)
        b |= std::uintptr_t(str[i] == 'x') << i;
    return { b };
}

int main()
{
    static const char scan[] = "ABCDEFGHIJKLM";
    // Looks for 'B' ? ? ? ? 'G' 'H' 'I' 'J' ? ? 'M'
    auto res = find_signature(
        std::begin(scan) + 1,
        std::end(scan) - 1,
        mask_from_str("x????xxxx??x"),
        FNV("BGHIJM")
    );


    // Looks for 'A' ? 'C' ? ? ? 'G' 'H' 'I' ? ? 'L'
    auto res2 = find_signature(
        std::begin(scan) + 1,
        std::end(scan) - 1,
        mask_from_str("x?x???xxx??x"),
        FNV("ACGHIL")
    );

    printf("%zd %zd\n", (char*)res - scan, (char*)res2 - scan);
    return 0;
}