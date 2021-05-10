// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 Bitcoin Association
// Distributed under the Open BSV software license, see the accompanying file LICENSE.

#ifndef BITCOIN_SERIALIZE_H
#define BITCOIN_SERIALIZE_H

#include <sv/compat/endian.h>

#include <boost/uuid/uuid.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <ios>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <sv/prevector.h>

static const uint64_t MAX_SIZE = std::numeric_limits<uint32_t>::max();

/**
 * Dummy data type to identify deserializing constructors.
 *
 * By convention, a constructor of a type T with signature
 *
 *   template <typename Stream> T::T(deserialize_type, Stream& s)
 *
 * is a deserializing constructor, which builds the type by deserializing it
 * from s. If T contains const fields, this is likely the only way to do so.
 */
struct deserialize_type {};
constexpr deserialize_type deserialize{};

/**
 * Used to bypass the rule against non-const reference to temporary where it
 * makes sense with wrappers such as CFlatData or CTxDB
 */
template <typename T> inline T &REF(const T &val) {
    return const_cast<T &>(val);
}

/**
 * Used to acquire a non-const pointer "this" to generate bodies of const
 * serialization operations from a template
 */
template <typename T> inline T *NCONST_PTR(const T *val) {
    return const_cast<T *>(val);
}

/*
 * Lowest-level serialization and conversion.
 * @note Sizes of these types are verified in the tests
 */
template <typename Stream> inline void ser_writedata8(Stream &s, uint8_t obj) {
    s.write((char *)&obj, 1);
}
template <typename Stream>
inline void ser_writedata16(Stream &s, uint16_t obj) {
    obj = htole16(obj);
    s.write((char *)&obj, 2);
}
template <typename Stream>
inline void ser_writedata32(Stream &s, uint32_t obj) {
    obj = htole32(obj);
    s.write((char *)&obj, 4);
}
template <typename Stream>
inline void ser_writedata64(Stream &s, uint64_t obj) {
    obj = htole64(obj);
    s.write((char *)&obj, 8);
}
template <typename Stream> inline uint8_t ser_readdata8(Stream &s) {
    uint8_t obj;
    s.read((char *)&obj, 1);
    return obj;
}
template <typename Stream> inline uint16_t ser_readdata16(Stream &s) {
    uint16_t obj;
    s.read((char *)&obj, 2);
    return le16toh(obj);
}
template <typename Stream> inline uint32_t ser_readdata32(Stream &s) {
    uint32_t obj;
    s.read((char *)&obj, 4);
    return le32toh(obj);
}
template <typename Stream> inline uint64_t ser_readdata64(Stream &s) {
    uint64_t obj;
    s.read((char *)&obj, 8);
    return le64toh(obj);
}
inline uint64_t ser_double_to_uint64(double x) {
    union {
        double x;
        uint64_t y;
    } tmp;
    tmp.x = x;
    return tmp.y;
}
inline uint32_t ser_float_to_uint32(float x) {
    union {
        float x;
        uint32_t y;
    } tmp;
    tmp.x = x;
    return tmp.y;
}
inline double ser_uint64_to_double(uint64_t y) {
    union {
        double x;
        uint64_t y;
    } tmp;
    tmp.y = y;
    return tmp.x;
}
inline float ser_uint32_to_float(uint32_t y) {
    union {
        float x;
        uint32_t y;
    } tmp;
    tmp.y = y;
    return tmp.x;
}

/////////////////////////////////////////////////////////////////
//
// Templates for serializing to anything that looks like a stream,
// i.e. anything that supports .read(char*, size_t) and .write(char*, size_t)
//
class CSizeComputer;

enum {
    // primary actions
    SER_NETWORK = (1 << 0),
    SER_DISK = (1 << 1),
    SER_GETHASH = (1 << 2),
};

#define READWRITE(obj) (::SerReadWrite(s, (obj), ser_action))
#define READWRITECOMPACTSIZE(obj) (::SerReadWriteCompactSize(s, (obj), ser_action))
#define READWRITEMANY(...) (::SerReadWriteMany(s, ser_action, __VA_ARGS__))

/**
 * Implement three methods for serializable objects. These are actually wrappers
 * over "SerializationOp" template, which implements the body of each class'
 * serialization code. Adding "ADD_SERIALIZE_METHODS" in the body of the class
 * causes these wrappers to be added as members.
 */
#define ADD_SERIALIZE_METHODS                                                  \
    template <typename Stream> void Serialize(Stream &s) const {               \
        NCONST_PTR(this)->SerializationOp(s, CSerActionSerialize());           \
    }                                                                          \
    template <typename Stream> void Unserialize(Stream &s) {                   \
        SerializationOp(s, CSerActionUnserialize());                           \
    }

template <typename Stream> inline void Serialize(Stream &s, char a) {
    ser_writedata8(s, a);
} // TODO Get rid of bare char
template <typename Stream> inline void Serialize(Stream &s, int8_t a) {
    ser_writedata8(s, a);
}
template <typename Stream> inline void Serialize(Stream &s, uint8_t a) {
    ser_writedata8(s, a);
}
template <typename Stream> inline void Serialize(Stream &s, int16_t a) {
    ser_writedata16(s, a);
}
template <typename Stream> inline void Serialize(Stream &s, uint16_t a) {
    ser_writedata16(s, a);
}
template <typename Stream> inline void Serialize(Stream &s, int32_t a) {
    ser_writedata32(s, a);
}
template <typename Stream> inline void Serialize(Stream &s, uint32_t a) {
    ser_writedata32(s, a);
}
template <typename Stream> inline void Serialize(Stream &s, int64_t a) {
    ser_writedata64(s, a);
}
template <typename Stream> inline void Serialize(Stream &s, uint64_t a) {
    ser_writedata64(s, a);
}
template <typename Stream> inline void Serialize(Stream &s, float a) {
    ser_writedata32(s, ser_float_to_uint32(a));
}
template <typename Stream> inline void Serialize(Stream &s, double a) {
    ser_writedata64(s, ser_double_to_uint64(a));
}
// TODO Get rid of bare char
template <typename Stream> inline void Unserialize(Stream &s, char &a) {
    a = ser_readdata8(s);
}
template <typename Stream> inline void Unserialize(Stream &s, int8_t &a) {
    a = ser_readdata8(s);
}
template <typename Stream> inline void Unserialize(Stream &s, uint8_t &a) {
    a = ser_readdata8(s);
}
template <typename Stream> inline void Unserialize(Stream &s, int16_t &a) {
    a = ser_readdata16(s);
}
template <typename Stream> inline void Unserialize(Stream &s, uint16_t &a) {
    a = ser_readdata16(s);
}
template <typename Stream> inline void Unserialize(Stream &s, int32_t &a) {
    a = ser_readdata32(s);
}
template <typename Stream> inline void Unserialize(Stream &s, uint32_t &a) {
    a = ser_readdata32(s);
}
template <typename Stream> inline void Unserialize(Stream &s, int64_t &a) {
    a = ser_readdata64(s);
}
template <typename Stream> inline void Unserialize(Stream &s, uint64_t &a) {
    a = ser_readdata64(s);
}
template <typename Stream> inline void Unserialize(Stream &s, float &a) {
    a = ser_uint32_to_float(ser_readdata32(s));
}
template <typename Stream> inline void Unserialize(Stream &s, double &a) {
    a = ser_uint64_to_double(ser_readdata64(s));
}

template <typename Stream> inline void Serialize(Stream &s, bool a) {
    char f = a;
    ser_writedata8(s, f);
}
template <typename Stream> inline void Unserialize(Stream &s, bool &a) {
    char f = ser_readdata8(s);
    a = f;
}

/**
 * Compact Size
 * size <  253        -- 1 byte
 * size <= USHRT_MAX  -- 3 bytes  (253 + 2 bytes)
 * size <= UINT_MAX   -- 5 bytes  (254 + 4 bytes)
 * size >  UINT_MAX   -- 9 bytes  (255 + 8 bytes)
 */
inline uint32_t GetSizeOfCompactSize(uint64_t nSize) {
    if (nSize < 253) {
        return sizeof(uint8_t);
    }
    if (nSize <= std::numeric_limits<uint16_t>::max()) {
        return sizeof(uint8_t) + sizeof(uint16_t);
    }
    if (nSize <= std::numeric_limits<uint32_t>::max()) {
        return sizeof(uint8_t) + sizeof(uint32_t);
    }

    return sizeof(uint8_t) + sizeof(uint64_t);
}

inline void WriteCompactSize(CSizeComputer &os, uint64_t nSize);

template <typename Stream> void WriteCompactSize(Stream &os, uint64_t nSize) {
    if (nSize > MAX_SIZE) {
          throw std::ios_base::failure("WriteCompactSize(): size too large");
    }
    if (nSize < 253) {
        ser_writedata8(os, nSize);
    } else if (nSize <= std::numeric_limits<uint16_t>::max()) {
        ser_writedata8(os, 253);
        ser_writedata16(os, nSize);
    } else if (nSize <= std::numeric_limits<uint32_t>::max()) {
        ser_writedata8(os, 254);
        ser_writedata32(os, nSize);
    } else {
        ser_writedata8(os, 255);
        ser_writedata64(os, nSize);
    }
    return;
}

template <typename Stream> uint64_t ReadCompactSize(Stream &is) {
    uint8_t chSize = ser_readdata8(is);
    uint64_t nSizeRet = 0;
    if (chSize < 253) {
        nSizeRet = chSize;
    } else if (chSize == 253) {
        nSizeRet = ser_readdata16(is);
        if (nSizeRet < 253) {
            throw std::ios_base::failure("non-canonical ReadCompactSize()");
        }
    } else if (chSize == 254) {
        nSizeRet = ser_readdata32(is);
        if (nSizeRet < 0x10000u) {
            throw std::ios_base::failure("non-canonical ReadCompactSize()");
        }
    } else {
        nSizeRet = ser_readdata64(is);
        if (nSizeRet < 0x100000000ULL) {
            throw std::ios_base::failure("non-canonical ReadCompactSize()");
        }
    }
    if (nSizeRet > MAX_SIZE) {
        throw std::ios_base::failure("ReadCompactSize(): size too large");
    }
    return nSizeRet;
}

/**
 * Variable-length integers: bytes are a MSB base-128 encoding of the number.
 * The high bit in each byte signifies whether another digit follows. To make
 * sure the encoding is one-to-one, one is subtracted from all but the last
 * digit. Thus, the byte sequence a[] with length len, where all but the last
 * byte has bit 128 set, encodes the number:
 *
 *  (a[len-1] & 0x7F) + sum(i=1..len-1, 128^i*((a[len-i-1] & 0x7F)+1))
 *
 * Properties:
 * * Very small (0-127: 1 byte, 128-16511: 2 bytes, 16512-2113663: 3 bytes)
 * * Every integer has exactly one encoding
 * * Encoding does not depend on size of original integer type
 * * No redundancy: every (infinite) byte sequence corresponds to a list
 *   of encoded integers.
 *
 * 0:         [0x00]  256:        [0x81 0x00]
 * 1:         [0x01]  16383:      [0xFE 0x7F]
 * 127:       [0x7F]  16384:      [0xFF 0x00]
 * 128:  [0x80 0x00]  16511:      [0xFF 0x7F]
 * 255:  [0x80 0x7F]  65535: [0x82 0xFE 0x7F]
 * 2^32:           [0x8E 0xFE 0xFE 0xFF 0x00]
 */
template <typename I> inline unsigned int GetSizeOfVarInt(I n) {
    int nRet = 0;
    while (true) {
        nRet++;
        if (n <= 0x7F) {
            return nRet;
        }
        n = (n >> 7) - 1;
    }
}

template <typename I> inline void WriteVarInt(CSizeComputer &os, I n);

template <typename Stream, typename I, class = typename std::enable_if<std::is_integral<I>::value>::type>
void WriteVarInt(Stream &os, I n) {
    uint8_t tmp[(sizeof(n) * 8 + 6) / 7];
    int len = 0;
    while (true) {
        tmp[len] = (n & 0x7F) | (len ? 0x80 : 0x00);
        if (n <= 0x7F) {
            break;
        }
        n = (n >> 7) - 1;
        len++;
    }
    do {
        ser_writedata8(os, tmp[len]);
    } while (len--);
}

template <typename Stream, typename I,class = typename std::enable_if<std::is_integral<I>::value>::type>
I ReadVarInt(Stream &is) {
    uintmax_t n {0};
    // VarInt encoding is only defined for unsigned integers. However there are places in source code
    // where ReadVarInt is called with a signed integer type (such as when serializing CDiskBlockPos)
    // Those places need to make sure that the actual values are always non-negative. 
    // Static cast in the following line if required to make MSVC compiler happy.
    // It is safe, because the value that is being casted is always positive and will always
    // fit in the unsigned version of type. 
    static uintmax_t overflow { static_cast<uintmax_t>(std::numeric_limits<I>::max() >> 7) };

    unsigned int maxSize = (sizeof(n) * 8 + 6) / 7;
    for (unsigned int i = 0; i<maxSize; ++i){
        if (n > overflow){
            throw std::runtime_error ("Deserialisation Error ReadVarInt");
        }

        uint8_t chData = ser_readdata8(is);
        n = (n << 7) | (chData & 0x7F);
        if ((chData & 0x80) == 0) {
            return n ;
        }
        n++;
    }
    // If we make it to hear its a deserialisation error
    // throw an exception
    throw std::runtime_error ("Deserialisation Error ReadVarInt");
}

#define FLATDATA(obj)                                                          \
    REF(CFlatData((char *)&(obj), (char *)&(obj) + sizeof(obj)))
#define VARINT(obj) REF(WrapVarInt(REF(obj)))
#define COMPACTSIZE(obj) REF(CCompactSize(REF(obj)))
#define LIMITED_STRING(obj, n) REF(LimitedBytes<n, std::string>(REF(obj)))
#define LIMITED_BYTE_VEC(obj, n) REF(LimitedBytes<n, std::vector<uint8_t>>(REF(obj)))

/**
 * Wrapper for serializing arrays and POD.
 */
class CFlatData {
protected:
    char *pbegin;
    char *pend;

public:
    CFlatData(void *pbeginIn, void *pendIn)
        : pbegin((char *)pbeginIn), pend((char *)pendIn) {}
    template <class T, class TAl> explicit CFlatData(std::vector<T, TAl> &v) {
        pbegin = (char *)v.data();
        pend = (char *)(v.data() + v.size());
    }
    template <unsigned int N, typename T, typename S, typename D>
    explicit CFlatData(prevector<N, T, S, D> &v) {
        pbegin = (char *)v.data();
        pend = (char *)(v.data() + v.size());
    }
    char *begin() { return pbegin; }
    const char *begin() const { return pbegin; }
    char *end() { return pend; }
    const char *end() const { return pend; }

    template <typename Stream> void Serialize(Stream &s) const {
        s.write(pbegin, pend - pbegin);
    }

    template <typename Stream> void Unserialize(Stream &s) {
        s.read(pbegin, pend - pbegin);
    }
};

template <typename I> class CVarInt {
protected:
    I &n;

public:
    CVarInt(I &nIn) : n(nIn) {}

    template <typename Stream> void Serialize(Stream &s) const {
        WriteVarInt<Stream, I>(s, n);
    }

    template <typename Stream> void Unserialize(Stream &s) {
        n = ReadVarInt<Stream, I>(s);
    }
};

class CCompactSize {
protected:
    uint64_t &n;

public:
    CCompactSize(uint64_t &nIn) : n(nIn) {}

    template <typename Stream> void Serialize(Stream &s) const {
        WriteCompactSize<Stream>(s, n);
    }

    template <typename Stream> void Unserialize(Stream &s) {
        n = ReadCompactSize<Stream>(s);
    }
};

template <size_t Limit, typename ArrayType> class LimitedBytes {
protected:
    ArrayType& array;

public:
    LimitedBytes(ArrayType& _array) : array(_array) {}

    template <typename Stream> void Unserialize(Stream &s) {
        size_t size = ReadCompactSize(s);
        if (size > Limit) {
            throw std::ios_base::failure("Array length limit exceeded");
        }
        array.resize(size);
        if (size != 0) {
            s.read((char*)&array[0], size);
        }
    }

    template <typename Stream> void Serialize(Stream &s) const {
        WriteCompactSize(s, array.size());
        if (!array.empty()) {
            s.write((char*)&array[0], array.size());
        }
    }
};

template <typename I> CVarInt<I> WrapVarInt(I &n) {
    return CVarInt<I>(n);
}

/**
 * Forward declarations
 */

/**
 *  string
 */
template <typename Stream, typename C>
void Serialize(Stream &os, const std::basic_string<C> &str);
template <typename Stream, typename C>
void Unserialize(Stream &is, std::basic_string<C> &str);

/**
 * prevector
 * prevectors of uint8_t are a special case and are intended to be serialized as
 * a single opaque blob.
 */
template <typename Stream, unsigned int N, typename T>
void Serialize_impl(Stream &os, const prevector<N, T> &v, const uint8_t &);
template <typename Stream, unsigned int N, typename T, typename V>
void Serialize_impl(Stream &os, const prevector<N, T> &v, const V &);
template <typename Stream, unsigned int N, typename T>
inline void Serialize(Stream &os, const prevector<N, T> &v);
template <typename Stream, unsigned int N, typename T>
void Unserialize_impl(Stream &is, prevector<N, T> &v, const uint8_t &);
template <typename Stream, unsigned int N, typename T, typename V>
void Unserialize_impl(Stream &is, prevector<N, T> &v, const V &);
template <typename Stream, unsigned int N, typename T>
inline void Unserialize(Stream &is, prevector<N, T> &v);



/**
 * vector
 * vectors of uint8_t are a special case and are intended to be serialized as a
 * single opaque blob.
 */
template <typename Stream, typename T, typename A>
void Serialize_impl(Stream &os, const std::vector<T, A> &v, const uint8_t &);
template <typename Stream, typename T, typename A, typename V>
void Serialize_impl(Stream &os, const std::vector<T, A> &v, const V &);
template <typename Stream, typename T, typename A>
inline void Serialize(Stream &os, const std::vector<T, A> &v);
template <typename Stream, typename T, typename A>
void Unserialize_impl(Stream &is, std::vector<T, A> &v, const uint8_t &);
template <typename Stream, typename T, typename A, typename V>
void Unserialize_impl(Stream &is, std::vector<T, A> &v, const V &);
template <typename Stream, typename T, typename A>
inline void Unserialize(Stream &is, std::vector<T, A> &v);

/**
 * pair
 */
template <typename Stream, typename K, typename T>
void Serialize(Stream &os, const std::pair<K, T> &item);
template <typename Stream, typename K, typename T>
void Unserialize(Stream &is, std::pair<K, T> &item);

/**
 * map
 */
template <typename Stream, typename K, typename T, typename Pred, typename A>
void Serialize(Stream &os, const std::map<K, T, Pred, A> &m);
template <typename Stream, typename K, typename T, typename Pred, typename A>
void Unserialize(Stream &is, std::map<K, T, Pred, A> &m);

/**
 * set
 */
template <typename Stream, typename K, typename Pred, typename A>
void Serialize(Stream &os, const std::set<K, Pred, A> &m);
template <typename Stream, typename K, typename Pred, typename A>
void Unserialize(Stream &is, std::set<K, Pred, A> &m);

/**
 * shared_ptr
 */
template <typename Stream, typename T>
void Serialize(Stream &os, const std::shared_ptr<const T> &p);
template <typename Stream, typename T>
void Unserialize(Stream &os, std::shared_ptr<const T> &p);

/**
 * UUID
 */
template <typename Stream>
void Serialize(Stream &os, const boost::uuids::uuid &v);
template <typename Stream>
void Unserialize(Stream &is, boost::uuids::uuid &v);

/**
 * unique_ptr
 */
template <typename Stream, typename T>
void Serialize(Stream &os, const std::unique_ptr<const T> &p);
template <typename Stream, typename T>
void Unserialize(Stream &os, std::unique_ptr<const T> &p);

/**
 * If none of the specialized versions above matched, default to calling member
 * function.
 */
template <typename Stream, typename T>
inline void Serialize(Stream &os, const T &a) {
    a.Serialize(os);
}

template <typename Stream, typename T>
inline void Unserialize(Stream &is, T &a) {
    a.Unserialize(is);
}

/**
 * string
 */
template <typename Stream, typename C>
void Serialize(Stream &os, const std::basic_string<C> &str) {
    WriteCompactSize(os, str.size());
    if (!str.empty()) {
        os.write((char *)&str[0], str.size() * sizeof(str[0]));
    }
}

template <typename Stream, typename C>
void Unserialize(Stream &is, std::basic_string<C> &str) {
    size_t nSize = ReadCompactSize(is);
    str.resize(nSize);
    if (nSize != 0) {
        is.read((char *)&str[0], nSize * sizeof(str[0]));
    }
}

/**
 * prevector
 */
template <typename Stream, unsigned int N, typename T>
void Serialize_impl(Stream &os, const prevector<N, T> &v, const uint8_t &) {
    WriteCompactSize(os, v.size());
    if (!v.empty()) {
        os.write((char *)&v[0], v.size() * sizeof(T));
    }
}

template <typename Stream, unsigned int N, typename T, typename V>
void Serialize_impl(Stream &os, const prevector<N, T> &v, const V &) {
    WriteCompactSize(os, v.size());
    for (const T &i : v) {
        ::Serialize(os, i);
    }
}

template <typename Stream, unsigned int N, typename T>
inline void Serialize(Stream &os, const prevector<N, T> &v) {
    Serialize_impl(os, v, T());
}

constexpr size_t STARTING_CHUNK_SIZE = 16000000; // 16MB
constexpr size_t CHUNK_GROWTH_RATE = 3;

template <typename Stream, unsigned int N, typename T>
void Unserialize_impl(Stream &is, prevector<N, T> &v, const uint8_t &) {
    // Limit size per read so bogus size value won't cause out of memory
    v.clear();
    size_t nSize = ReadCompactSize(is);
    size_t i = 0;
    size_t chunkSize = STARTING_CHUNK_SIZE;
    while (i < nSize) {
        size_t blk = std::min(nSize - i, size_t(1 + (chunkSize - 1) / sizeof(T)));
        chunkSize *= CHUNK_GROWTH_RATE;
        v.resize(i + blk);
        is.read((char *)&v[i], blk * sizeof(T));
        i += blk;
    }
}

template <typename Stream, unsigned int N, typename T, typename V>
void Unserialize_impl(Stream &is, prevector<N, T> &v, const V &) {
    v.clear();
    size_t nSize = ReadCompactSize(is);
    size_t i = 0;
    size_t nMid = 0;
    size_t chunkSize = STARTING_CHUNK_SIZE;
    while (nMid < nSize) {
        nMid += std::min(nSize, size_t(1 + (chunkSize - 1) / sizeof(T)));
        chunkSize *= CHUNK_GROWTH_RATE;
        if (nMid > nSize) {
            nMid = nSize;
        }
        v.resize(nMid);
        for (; i < nMid; i++) {
            Unserialize(is, v[i]);
        }
    }
}


template <typename Stream, unsigned int N, typename T>
inline void Unserialize(Stream &is, prevector<N, T> &v) {
    Unserialize_impl(is, v, T());
}

/**
 * vector
 */
template <typename Stream, typename T, typename A>
void Serialize_impl(Stream &os, const std::vector<T, A> &v, const uint8_t &) {
    WriteCompactSize(os, v.size());
    if (!v.empty()) {
        os.write((char *)&v[0], v.size() * sizeof(T));
    }
}

template <typename Stream, typename T, typename A, typename V>
void Serialize_impl(Stream &os, const std::vector<T, A> &v, const V &) {
    WriteCompactSize(os, v.size());
    for (const T &i : v) {
        ::Serialize(os, i);
    }
}

template <typename Stream, typename T, typename A>
inline void Serialize(Stream &os, const std::vector<T, A> &v) {
    Serialize_impl(os, v, T());
}

template <typename Stream, typename T, typename A>
void Unserialize_impl(Stream &is, std::vector<T, A> &v, const uint8_t &) {
    // Limit size per read so bogus size value won't cause out of memory
    v.clear();
    size_t nSize = ReadCompactSize(is);
    size_t i = 0;
    size_t chunkSize = STARTING_CHUNK_SIZE;
    while (i < nSize) {
        size_t blk = std::min(nSize - i, size_t(1 + (chunkSize - 1) / sizeof(T)));
        chunkSize *= CHUNK_GROWTH_RATE;
        v.resize(i + blk);
        is.read((char *)&v[i], blk * sizeof(T));
        i += blk;
    }
}


template <typename Stream, typename T, typename A, typename V>
void Unserialize_impl(Stream &is, std::vector<T, A> &v, const V &) {
    v.clear();
    size_t nSize = ReadCompactSize(is);
    size_t i = 0;
    size_t nMid = 0;
    size_t chunkSize = STARTING_CHUNK_SIZE;
    while (nMid < nSize) {
        nMid += std::min(nSize, size_t(1 + (chunkSize - 1) / sizeof(T)));
        chunkSize *= CHUNK_GROWTH_RATE;
        if (nMid > nSize) {
            nMid = nSize;
        }
        v.resize(nMid);
        for (; i < nMid; i++) {
            Unserialize(is, v[i]);
        }
    }
}


template <typename Stream, typename T, typename A>
inline void Unserialize(Stream &is, std::vector<T, A> &v) {
    Unserialize_impl(is, v, T());
}

/**
 * pair
 */
template <typename Stream, typename K, typename T>
void Serialize(Stream &os, const std::pair<K, T> &item) {
    Serialize(os, item.first);
    Serialize(os, item.second);
}

template <typename Stream, typename K, typename T>
void Unserialize(Stream &is, std::pair<K, T> &item) {
    Unserialize(is, item.first);
    Unserialize(is, item.second);
}

/**
 * map
 */
template <typename Stream, typename K, typename T, typename Pred, typename A>
void Serialize(Stream &os, const std::map<K, T, Pred, A> &m) {
    WriteCompactSize(os, m.size());
    for (const auto& p : m) {
        Serialize(os, p);
    }
}

template <typename Stream, typename K, typename T, typename Pred, typename A>
void Unserialize(Stream &is, std::map<K, T, Pred, A> &m) {
    m.clear();
    size_t nSize = ReadCompactSize(is);
    typename std::map<K, T, Pred, A>::iterator mi = m.begin();
    for (size_t i = 0; i < nSize; i++) {
        std::pair<K, T> item;
        Unserialize(is, item);
        mi = m.insert(mi, item);
    }
}

/**
 * set
 */
template <typename Stream, typename K, typename Pred, typename A>
void Serialize(Stream &os, const std::set<K, Pred, A> &m) {
    WriteCompactSize(os, m.size());
    for (const K &i : m) {
        Serialize(os, i);
    }
}

template <typename Stream, typename K, typename Pred, typename A>
void Unserialize(Stream &is, std::set<K, Pred, A> &m) {
    m.clear();
    size_t nSize = ReadCompactSize(is);
    typename std::set<K, Pred, A>::iterator it = m.begin();
    for (size_t i = 0; i < nSize; i++) {
        K key;
        Unserialize(is, key);
        it = m.insert(it, key);
    }
}

/**
 * unique_ptr
 */
template <typename Stream, typename T>
void Serialize(Stream &os, const std::unique_ptr<const T> &p) {
    Serialize(os, *p);
}

template <typename Stream, typename T>
void Unserialize(Stream &is, std::unique_ptr<const T> &p) {
    p.reset(new T(deserialize, is));
}

/**
 * shared_ptr
 */
template <typename Stream, typename T>
void Serialize(Stream &os, const std::shared_ptr<const T> &p) {
    Serialize(os, *p);
}

template <typename Stream, typename T>
void Unserialize(Stream &is, std::shared_ptr<const T> &p) {
    p = std::make_shared<const T>(deserialize, is);
}

/**
 * UUID
 */
template <typename Stream>
void Serialize(Stream &os, const boost::uuids::uuid &v) {
    // The size of the UUID is fixed at 16 bytes.
    static constexpr auto uuid_size = boost::uuids::uuid::static_size();
    static_assert(uuid_size == 16);
    static_assert(sizeof(boost::uuids::uuid::value_type) == sizeof(char));
    os.write(reinterpret_cast<const char*>(v.data), uuid_size);
}

template <typename Stream>
void Unserialize(Stream &is, boost::uuids::uuid &v) {
    // The size of the UUID is fixed at 16 bytes.
    static constexpr auto uuid_size = boost::uuids::uuid::static_size();
    static_assert(uuid_size == 16);
    static_assert(sizeof(boost::uuids::uuid::value_type) == sizeof(char));
    is.read(reinterpret_cast<char*>(v.data), uuid_size);
}

/**
 * Support for ADD_SERIALIZE_METHODS and READWRITE macro
 */
struct CSerActionSerialize {
    constexpr bool ForRead() const { return false; }
};
struct CSerActionUnserialize {
    constexpr bool ForRead() const { return true; }
};

template <typename Stream, typename T>
inline void SerReadWrite(Stream &s, const T &obj,
                         CSerActionSerialize ser_action) {
    ::Serialize(s, obj);
}

template <typename Stream, typename T>
inline void SerReadWrite(Stream &s, T &obj, CSerActionUnserialize ser_action) {
    ::Unserialize(s, obj);
}

/**
 * Support for READWRITECOMPACTSIZE macro
 */

template <typename Stream>
inline void SerReadWriteCompactSize(Stream &s, const uint64_t &obj,
                         CSerActionSerialize ser_action) {
    ::WriteCompactSize(s, obj);
}

template <typename Stream>
inline void SerReadWriteCompactSize(Stream &s, uint64_t &obj, CSerActionUnserialize ser_action) {
    obj = ::ReadCompactSize(s);
}

/**
 * ::GetSerializeSize implementations
 *
 * Computing the serialized size of objects is done through a special stream
 * object of type CSizeComputer, which only records the number of bytes written
 * to it.
 *
 * If your Serialize or SerializationOp method has non-trivial overhead for
 * serialization, it may be worthwhile to implement a specialized version for
 * CSizeComputer, which uses the s.seek() method to record bytes that would
 * be written instead.
 */
class CSizeComputer {
protected:
    size_t nSize;

    const int nType;
    const int nVersion;

public:
    CSizeComputer(int nTypeIn, int nVersionIn)
        : nSize(0), nType(nTypeIn), nVersion(nVersionIn) {}

    void write(const char *psz, size_t _nSize) { this->nSize += _nSize; }

    /** Pretend _nSize bytes are written, without specifying them. */
    void seek(size_t _nSize) { this->nSize += _nSize; }

    template <typename T> CSizeComputer &operator<<(const T &obj) {
        ::Serialize(*this, obj);
        return (*this);
    }

    size_t size() const { return nSize; }

    int GetVersion() const { return nVersion; }
    int GetType() const { return nType; }
};

template <typename Stream> void SerializeMany(Stream &s) {}

template <typename Stream, typename Arg>
void SerializeMany(Stream &s, Arg &&arg) {
    ::Serialize(s, std::forward<Arg>(arg));
}

template <typename Stream, typename Arg, typename... Args>
void SerializeMany(Stream &s, Arg &&arg, Args &&... args) {
    ::Serialize(s, std::forward<Arg>(arg));
    ::SerializeMany(s, std::forward<Args>(args)...);
}

template <typename Stream> inline void UnserializeMany(Stream &s) {}

template <typename Stream, typename Arg>
inline void UnserializeMany(Stream &s, Arg &arg) {
    ::Unserialize(s, arg);
}

template <typename Stream, typename Arg, typename... Args>
inline void UnserializeMany(Stream &s, Arg &arg, Args &... args) {
    ::Unserialize(s, arg);
    ::UnserializeMany(s, args...);
}

template <typename Stream, typename... Args>
inline void SerReadWriteMany(Stream &s, CSerActionSerialize ser_action,
                             Args &&... args) {
    ::SerializeMany(s, std::forward<Args>(args)...);
}

template <typename Stream, typename... Args>
inline void SerReadWriteMany(Stream &s, CSerActionUnserialize ser_action,
                             Args &... args) {
    ::UnserializeMany(s, args...);
}

template <typename I> inline void WriteVarInt(CSizeComputer &s, I n) {
    s.seek(GetSizeOfVarInt<I>(n));
}

inline void WriteCompactSize(CSizeComputer &s, uint64_t nSize) {
    s.seek(GetSizeOfCompactSize(nSize));
}

template <typename T>
size_t GetSerializeSize(const T &t, int nType, int nVersion = 0) {
    return (CSizeComputer(nType, nVersion) << t).size();
}

template <typename S, typename T>
size_t GetSerializeSize(const S &s, const T &t) {
    return (CSizeComputer(s.GetType(), s.GetVersion()) << t).size();
}

#endif // BITCOIN_SERIALIZE_H
