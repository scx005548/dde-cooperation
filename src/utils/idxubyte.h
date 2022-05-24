#ifndef IDX_UBYTE_H
#define IDX_UBYTE_H

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <vector>

template <uint8_t N>
struct IdxUbyteData {
    uint8_t *data = nullptr;
    uint32_t dims[N];

    IdxUbyteData() noexcept = default;

    ~IdxUbyteData() noexcept {
        if (data != nullptr) {
            delete[] data;
            data = nullptr;
        }
    }

    IdxUbyteData(IdxUbyteData &&src) noexcept {
        data = src.data;
        src.data = nullptr;
        memcpy(dims, src.dims, sizeof(dims));
    }

    IdxUbyteData(const IdxUbyteData &src) noexcept {
        memcpy(dims, src.dims, sizeof(dims));

        size_t bytes = 1;
        for (uint32_t i = 0; i < N; i++) {
            bytes *= dims[i];
        }

        data = new uint8_t[bytes];
        memcpy(data, src.data, bytes);
    }
};

template <uint8_t N>
class IdxUbyte {
public:
    IdxUbyte() noexcept = default;
    ~IdxUbyte() noexcept = default;

    bool write(const char *file, const std::vector<IdxUbyteData<N - 1>> &dataset) const noexcept {
        if (dataset.size() == 0) return false;

        FILE *fp = fopen(file, "wb");
        if (fp == nullptr) {
            fprintf(stderr, "%s\n", strerror(errno));
            return false;
        }

        this->m_write<32>(fp, MagicNumber);
        this->m_write<32>(fp, dataset.size());

        size_t bytes = 1;
        for (uint32_t i = 0; i < N - 1; i++) {
            this->m_write<32>(fp, dataset[0].dims[i]);
            bytes *= dataset[0].dims[i];
        }

        for (const auto &data : dataset) {
            if (fwrite(data.data, 1, bytes, fp) < bytes) {
                fprintf(stderr, "%s\n", strerror(errno));
            }
        }

        fclose(fp);
        return true;
    }

    std::vector<IdxUbyteData<N - 1>> read(const char *file) const noexcept {
        std::vector<IdxUbyteData<N - 1>> ret(0);

        FILE *fp = fopen(file, "rb");
        if (fp == nullptr) {
            fprintf(stderr, "%s\n", strerror(errno));
            return ret;
        }

        uint32_t magic = this->m_read<32>(fp);
        if (magic != MagicNumber) {
            fprintf(stderr, "magic number mismatch: 0x%08x != 0x%08x\n", magic, MagicNumber);
            fclose(fp);
            return ret;
        }

        uint32_t dims[N];
        for (size_t i = 0; i < N; i++) {
            dims[i] = this->m_read<32>(fp);
            printf("dim %zu: %u\n", i, dims[i]);
        }

        for (uint32_t i = 0; i < dims[0]; i++) {
            size_t bytes = 1;
            IdxUbyteData<N - 1> &data = ret.emplace_back();
            for (size_t j = 1; j < N; j++) {
                data.dims[j - 1] = dims[j];
                bytes *= dims[j];
            }

            data.data = new uint8_t[bytes];
            if (fread(data.data, 1, bytes, fp) < bytes) {
                fprintf(stderr, "%s\n", strerror(errno));
            }
        }

        fclose(fp);
        return ret;
    }

private:
    constexpr static const uint32_t MagicNumber = 0x00000800 | N;

    // 大端读
    template <size_t bits>
    uint32_t m_read(FILE *fp) const noexcept {
        uint32_t ret = 0;
        uint8_t byte = 0;

        for (size_t i = 0; i < bits / 8; i++) {
            ret <<= 8;
            if (fread(&byte, 1, 1, fp) < 1) {
                fprintf(stderr, "%s\n", strerror(errno));
            }
            ret |= byte;
        }

        return ret;
    }

    // 大端写
    template <size_t bits>
    void m_write(FILE *fp, uintmax_t value) const noexcept {
        constexpr const size_t bytes = bits / 8;
        uint8_t byte = 0;

        for (size_t i = 1; i <= bytes; i++) {
            byte = static_cast<uint8_t>(value >> (8 * (bytes - i)));
            fwrite(&byte, 1, 1, fp);
        }
    }
};

#endif // !IDX_UBYTE_H
