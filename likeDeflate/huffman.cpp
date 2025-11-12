#include "huffman.h"
#include <queue>
#include <algorithm>
#include <limits>
#include <cstring>

namespace huff {

// ---------- util ----------

static inline uint32_t bitrev(uint32_t x, int n) {
    uint32_t r = 0;
    for (int i = 0; i < n; ++i) { r = (r << 1) | (x & 1u); x >>= 1; }
    return r;
}

// ---------- construir longitudes (heap + DFS iterativo) ----------

struct Node {
    uint32_t sym;
    uint32_t freq;
    int left = -1, right = -1;
};

static std::vector<uint8_t> buildCodeLengths(const std::vector<uint32_t>& freq, uint8_t maxLen) {
    const uint32_t N = (uint32_t)freq.size();
    // si todo cero -> un símbolo con longitud 1
    bool allZero = true;
    for (auto v : freq) if (v) { allZero = false; break; }
    if (allZero) {
        std::vector<uint8_t> len(N, 0);
        if (N) len[0] = 1;
        return len;
    }

    struct QItem { uint32_t f; int idx; bool operator>(const QItem& o) const { return f > o.f; } };
    std::priority_queue<QItem, std::vector<QItem>, std::greater<QItem>> pq;
    std::vector<Node> nodes; nodes.reserve(2*N);

    for (uint32_t i = 0; i < N; ++i) if (freq[i]) {
        nodes.push_back(Node{ i, freq[i], -1, -1 });
        pq.push(QItem{ freq[i], (int)nodes.size()-1 });
    }
    if (pq.size() == 1) {
        std::vector<uint8_t> len(N, 0);
        len[nodes[pq.top().idx].sym] = 1;
        return len;
    }
    while (pq.size() >= 2) {
        auto a = pq.top(); pq.pop();
        auto b = pq.top(); pq.pop();
        nodes.push_back(Node{ (uint32_t)-1, a.f + b.f, a.idx, b.idx });
        pq.push(QItem{ a.f + b.f, (int)nodes.size()-1 });
    }
    int root = pq.top().idx;

    // DFS iterativo para obtener profundidades
    std::vector<uint8_t> codeLen(N, 0);
    struct Frame { int u; int depth; };
    std::vector<Frame> st; st.push_back({root, 0});
    while (!st.empty()) {
        auto [u, d] = st.back(); st.pop_back();
        if (nodes[u].left == -1 && nodes[u].right == -1) {
            codeLen[nodes[u].sym] = (uint8_t)d;
        } else {
            if (nodes[u].left  != -1) st.push_back({nodes[u].left,  d + 1});
            if (nodes[u].right != -1) st.push_back({nodes[u].right, d + 1});
        }
    }

    // limita a maxLen (estrategia pragmática)
    for (auto& L : codeLen) if (L > maxLen) L = maxLen;

    // ajuste Kraft sencillo si hiciera falta
    auto kraftOverflow = [&]() {
        long long sum = 0;
        for (auto L : codeLen) if (L) sum += (1LL << (maxLen - L));
        return sum > (1LL << maxLen);
    };
    if (kraftOverflow()) {
        struct Item { uint8_t L; uint32_t f; uint32_t s; };
        std::vector<Item> items;
        for (uint32_t s = 0; s < N; ++s) if (codeLen[s]) items.push_back({ codeLen[s], freq[s], s });
        std::sort(items.begin(), items.end(), [](const Item& a, const Item& b){
            if (a.L != b.L) return a.L > b.L;   // más largos primero
            if (a.f != b.f) return a.f < b.f;   // menos frecuentes primero
            return a.s < b.s;
        });
        while (kraftOverflow()) {
            for (auto& it : items) { if (it.L > 1) { it.L--; if (!kraftOverflow()) break; } }
        }
        std::fill(codeLen.begin(), codeLen.end(), 0);
        for (auto& it : items) codeLen[it.s] = it.L;
    }

    return codeLen;
}

static void makeCanonicalCodes(const std::vector<uint8_t>& codeLen,
                               std::vector<Code>& outCodes,
                               uint8_t maxLen)
{
    const uint32_t N = (uint32_t)codeLen.size();
    outCodes.assign(N, {});
    std::vector<uint32_t> blCount(maxLen + 1, 0);
    for (auto L : codeLen) if (L) blCount[L]++;

    std::vector<uint32_t> nextCode(maxLen + 1, 0);
    uint32_t code = 0;
    for (uint32_t bits = 1; bits <= maxLen; ++bits) {
        code = (code + blCount[bits - 1]) << 1;
        nextCode[bits] = code;
    }
    for (uint32_t n = 0; n < N; ++n) {
        auto L = codeLen[n];
        if (L) {
            outCodes[n].len  = L;
            outCodes[n].code = nextCode[L]++;
        }
    }
}

// ---------- CanonicalHuffman ----------

void CanonicalHuffman::build(const std::vector<uint32_t>& frequencies, uint8_t maxCodeLen) {
    codeLen_ = buildCodeLengths(frequencies, maxCodeLen);
    uint8_t m = 0; for (auto L : codeLen_) m = std::max<uint8_t>(m, L);
    makeCanonicalCodes(codeLen_, codes_, m ? m : 1);
    buildDecoder();
}

void CanonicalHuffman::loadFromCodeLengths(const std::vector<uint8_t>& codeLengths) {
    codeLen_ = codeLengths;
    uint8_t m = 0; for (auto L : codeLen_) m = std::max<uint8_t>(m, L);
    makeCanonicalCodes(codeLen_, codes_, m ? m : 1);
    buildDecoder();
}

void CanonicalHuffman::buildDecoder() {
    // Prepara tablas de códigos revertidos (para LSB-first) y buckets por longitud.
    uint8_t maxLen = 0;
    for (auto L : codeLen_) if (L) maxLen = std::max<uint8_t>(maxLen, L);

    revCode_.assign(codes_.size(), 0);
    for (size_t s = 0; s < codes_.size(); ++s)
        if (codes_[s].len) revCode_[s] = bitrev(codes_[s].code, codes_[s].len);

    byLenSyms_.assign(maxLen + 1, {});
    for (size_t s = 0; s < codes_.size(); ++s)
        if (codes_[s].len) byLenSyms_[codes_[s].len].push_back((uint32_t)s);
}

void CanonicalHuffman::encodeSymbol(BitWriter& bw, uint32_t sym) const {
    if (sym >= codes_.size()) throw std::runtime_error("encodeSymbol: symbol out of range");
    const Code& c = codes_[sym];
    if (c.len == 0) throw std::runtime_error("encodeSymbol: zero-length code");
    // Emitimos en LSB-first -> escribir el código canónico **revertido**
    bw.writeBits(bitrev(c.code, c.len), c.len);
}

uint32_t CanonicalHuffman::decodeSymbol(BitReader& br) const {
    uint32_t code = 0;
    for (uint32_t L = 1; L < byLenSyms_.size(); ++L) {
        code |= (br.readBits(1) << (L - 1));      // LSB-first
        const auto& bucket = byLenSyms_[L];
        for (uint32_t s : bucket) {
            if (revCode_[s] == code) return s;    // match exacto
        }
    }
    throw std::runtime_error("decodeSymbol: invalid code");
}

// ---------- Stream simple (cabecera + bitstream) ----------

static void writeU16(BitWriter& bw, uint16_t v) {
    bw.flushZeroPadding();
    auto& out = const_cast<std::vector<uint8_t>&>(bw.data());
    out.push_back((uint8_t)(v & 0xFF));
    out.push_back((uint8_t)((v >> 8) & 0xFF));
}
static void writeU32(BitWriter& bw, uint32_t v) {
    bw.flushZeroPadding();
    auto& out = const_cast<std::vector<uint8_t>&>(bw.data());
    out.push_back((uint8_t)(v & 0xFF));
    out.push_back((uint8_t)((v >> 8) & 0xFF));
    out.push_back((uint8_t)((v >> 16) & 0xFF));
    out.push_back((uint8_t)((v >> 24) & 0xFF));
}
static uint16_t readU16(const uint8_t* p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static uint32_t readU32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

std::vector<uint8_t> encodeHuffmanStream(const std::vector<uint32_t>& symbols,
                                         uint32_t alphabetSize,
                                         uint8_t maxCodeLen)
{
    // 1) Frecuencias
    std::vector<uint32_t> freq(alphabetSize, 0);
    for (auto s : symbols) {
        if (s >= alphabetSize) throw std::runtime_error("encodeHuffmanStream: symbol out of range");
        freq[s]++;
    }

    // 2) Construir huffman
    CanonicalHuffman H;
    H.build(freq, maxCodeLen);
    const auto& lens = H.codeLengths();

    // 3) Cabecera
    BitWriter bw;
    writeU16(bw, (uint16_t)alphabetSize);
    bw.flushZeroPadding();
    auto& out = const_cast<std::vector<uint8_t>&>(bw.data());
    out.insert(out.end(), lens.begin(), lens.end());
    writeU32(bw, (uint32_t)symbols.size());

    // 4) Payload
    for (auto s : symbols) H.encodeSymbol(bw, s);
    bw.flushZeroPadding();
    return bw.data();
}

std::vector<uint32_t> decodeHuffmanStream(const uint8_t* data, size_t size)
{
    if (size < 2) throw std::runtime_error("decodeHuffmanStream: truncated header");
    uint16_t alphabetSize = readU16(data);
    size_t off = 2;
    if (size < off + alphabetSize) throw std::runtime_error("decodeHuffmanStream: truncated code lengths");
    std::vector<uint8_t> lens(alphabetSize);
    std::copy(data + off, data + off + alphabetSize, lens.begin());
    off += alphabetSize;
    if (size < off + 4) throw std::runtime_error("decodeHuffmanStream: truncated symbol count");
    uint32_t nsyms = readU32(data + off);
    off += 4;
    if (off > size) throw std::runtime_error("decodeHuffmanStream: bad offsets");

    CanonicalHuffman H;
    H.loadFromCodeLengths(lens);

    BitReader br(data + off, size - off);
    std::vector<uint32_t> out;
    out.reserve(nsyms);
    for (uint32_t i = 0; i < nsyms; ++i) out.push_back(H.decodeSymbol(br));
    return out;
}

} // namespace huff
