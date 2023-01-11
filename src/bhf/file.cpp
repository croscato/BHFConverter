// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Gustavo Ribeiro Croscato

#include "file.hpp"

namespace BHF {

struct ControlCode {
    enum : u8 {
        NewLine = 0x00,
        DocumentEnd = 0x01,
        KeywordMark = 0x02,
        SourceCode = 0x05,
        CharRaw = 0x0f,
        CharCount = 0x0e,
    };

    static bool isValid(u8 code)
    {
        return code == NewLine
            || code == DocumentEnd
            || code == KeywordMark
            || code == SourceCode
            || code == CharRaw
            || code == CharCount;
    }
};

struct FileData {
    FILE *file = nullptr;

    std::string stamp;
    std::string signature;
    Version version{Version::Invalid, 0};
    FileHeader file_header{0, 0, 0, 0, 0, 0};
    Compression compression{Compression::Invalid, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    File::ContextContainer context;
    File::IndexContainer index;

    std::string last_error;
};

static std::string kHtmlSpace = "&nbsp;";
static constexpr int kAsciiSpace = 0x20;

static std::string BHF_CP437toUTF8(u8 character);
static std::string BHF_HTMLEncoding(u8 character);
static void BHF_InsertHtmlKeyword(std::string &text, std::string::size_type position, File::ContextType context);

File::File() noexcept
    : d{std::make_unique<FileData>()}
{}

File::File(std::string_view filepath) noexcept
    : File()
{
    open(filepath);
}

File::~File() noexcept {
    if (d->file) {
        fclose(d->file);
    }
}

bool
File::open(std::string_view filepath) noexcept
{
    if (d->file) {
        fclose(d->file);
    }

    d->file = fopen(filepath.data(), "rb");
    // TODO: error handling

    if (d->file) {
        parse();
    }

    return d->file != nullptr;
}

const std::string &
File::stamp() const noexcept
{
    return d->stamp;
}

const std::string &
File::signature() const noexcept
{
    return d->signature;
}

const Version &
File::version() const noexcept
{
    return d->version;
}

const FileHeader &
File::fileHeader() const noexcept
{
    return d->file_header;
}

const Compression &
File::compression() const noexcept
{
    return d->compression;
}

const File::ContextContainer &
File::context() const noexcept
{
    return d->context;
}

const File::IndexContainer &
File::index() const noexcept
{
    return d->index;
}

std::string
File::BHF_FormatAsText(const std::string &text) const
{
    std::string result;
    result.reserve(text.size());

    for (std::string::size_type i = 0; i < text.size(); ++i) {
        u8 value = static_cast<u8>(text[i]);

        if (ControlCode::isValid(value)) {
            if (value == ControlCode::NewLine) {
                result += "\n";
            } else if (value == ControlCode::DocumentEnd) {
                break;
            }
        } else {
            result += BHF_CP437toUTF8(value);
        }
    }

    return result;
}

std::string
File::BHF_FormatAsHTML(const std::string &text)
{
    std::string result;
    result.reserve(text.size());

    BHF::File::ContextContainer::size_type keyword = 0;
    std::string::size_type keyword_start = 0;
    std::string::size_type keyword_end = 0;

    const KeywordData keywords = readKeywords();

    bool in_keyword = false;
    bool in_code = false;

    result += "<pre>";

    for (std::string::size_type i = 0; i < text.size(); ++i) {
        u8 value = static_cast<u8>(text[i]);

        if (ControlCode::isValid(value)) {
            if (value == ControlCode::NewLine) {
                result += "<br>";
            } else if (value == ControlCode::KeywordMark) {
                in_keyword = !in_keyword;

                if (in_keyword) {
                    keyword_start = 0;
                    keyword_end = 0;
                } else {
                    result.insert(keyword_end, "</a>");
                    result.insert(keyword_start, fmt::format("<a href=\"{}\">", keywords.contexts.at(keyword++)));
                }
            } else if (value == ControlCode::SourceCode) {
                in_code = !in_code;

                if (in_code) {
                    result += "<code>";
                } else {
                    result += "</code>";
                }
            } else if (value == ControlCode::DocumentEnd) {
                break;
            }
        } else {
            if (in_keyword && value != kAsciiSpace && keyword_start == 0) {
                keyword_start = result.size();
            }

            result += BHF_HTMLEncoding(value);

            if (in_keyword && value != kAsciiSpace) {
                keyword_end = result.size();
            }
        }
    }

    result += "</pre>";

    return result;
}

std::string
File::text(ContextType offset, TextFormat format) noexcept
{
    // TODO: error handling
    fseek(d->file, offset, SEEK_SET);

    RecordHeader record = readType<RecordHeader>();

    if (record.type != RecordHeader::Text) {
        // TODO: error handling
        return "";
    }

    std::string uncompressed_text = uncompress(record);

    if (format == PlainText) {
        return BHF_FormatAsText(uncompressed_text);
    } else if (format == HTML) {
        return BHF_FormatAsHTML(uncompressed_text);
    }

    return uncompressed_text;
}

const std::string &
File::lastError() const noexcept
{
    return d->last_error;
}

struct NibbleStream {
    NibbleStream(FILE *_f, int _l)
    {
        this->file = _f;
        this->length = _l;
    }

    u8 next() {
        --length;

        if (++index & 0x01) {
            nibble = fgetc(file);

            return nibble & 0x0f;
        }

        return (nibble >> 4u) & 0x0f;
    }


    bool isEmpty() const
    {
        return length <= 0;
    }

    int nibble = 0;
    FILE *file = nullptr;
    int length = 0;
    int index = 0;
};

void
BHF_InsertHtmlKeyword(std::string &text, std::string::size_type position, File::ContextType context)
{
    auto size = kHtmlSpace.size();

    if (text.size() > size) {
        while (text.compare(position, size, kHtmlSpace) == 0) {
            position += size;
        }
    }

    text.insert(position, fmt::format("<a href=\"{}\">", context));

    position = text.size();

    if (position > size) {
        while (text.compare(position - size, size, kHtmlSpace) == 0) {
            position -= size;
        }
    }

    text.insert(position, "</a>");
}

std::string
File::uncompress(const RecordHeader &record)
{
    std::string result;

    result.reserve(record.length);

    NibbleStream stream(d->file, record.length * 2);

    bool break_on_width = false;
    bool in_keyword = false;

    const std::string::size_type margin_width = static_cast<std::string::size_type>(d->file_header.left_margin);
    const std::string::size_type maximum_width = d->file_header.width - margin_width;
    std::string::size_type width = margin_width;
    std::string::size_type count = 0;
    std::string::size_type last_space = 0;

    u8 last_value;

    while (!stream.isEmpty()) {
        u8 nibble = stream.next();
        u8 value = 0;

        if (nibble == ControlCode::CharRaw) {
            u8 n1 = stream.next();
            u8 n2 = stream.next();

            value = static_cast<u8>((n2 << 4) | n1);
            count += 1;
        } else if (nibble == ControlCode::CharCount) {
            count = static_cast<std::string::size_type>(stream.next() + 1);

            continue;
        } else {
            value = d->compression.table[nibble];
            count += 1;
        }

        if (value == ControlCode::KeywordMark) {
            in_keyword = !in_keyword;
        }

        if (width == margin_width && value != kAsciiSpace && value != ControlCode::NewLine) {
            break_on_width = true;
        }

        if (break_on_width && last_value == ControlCode::NewLine && (value == ControlCode::NewLine || value == kAsciiSpace)) {
            break_on_width = false;

            result += static_cast<char>(ControlCode::NewLine);
        }

        if (break_on_width && value == ControlCode::NewLine) {
            if (width > maximum_width) {
                result.replace(last_space - 1, 1, std::string(1, static_cast<char>(ControlCode::NewLine)));
                width = result.size() - last_space;
            }

            last_value = value;

            value = kAsciiSpace;
        } else {
            last_value = value;
        }

        std::string converted = std::string(1, static_cast<char>(value));

        if (!ControlCode::isValid(value)) {
            width += count;
        }

        while (count > 0) {
            result += converted;

            --count;
        }

        if (value == ControlCode::NewLine) {
            width = margin_width;
            last_space = 0;
            break_on_width = false;
        } else if (value == kAsciiSpace && width < maximum_width && !in_keyword) {
            last_space = result.size();
        }
    }

    return result;
}

void
File::readString(std::string &str) noexcept
{
    // TODO: error handling
    i64 string_start = ftell(d->file);

    while (fgetc(d->file) != 0) {}

    // TODO: error handling
    i64 string_end = ftell(d->file);

    // TODO: error handling
    fseek(d->file, static_cast<int>(string_start), SEEK_SET);

    size_t string_size = static_cast<size_t>(string_end - string_start);

    str.resize(string_size - 1);

    size_t bytes_read = fread(str.data(), 1, string_size, d->file);

    if (bytes_read != string_size) {
        // TODO: better error handling (erro code?)
        d->last_error = fmt::format("Short read, trying to read {} bytes got {} bytes.", string_size, bytes_read);
    }
}

File::KeywordData
File::readKeywords() noexcept
{
    RecordHeader record = readType<RecordHeader>();

    if (record.type != RecordHeader::Keyword) {
        // TODO: error handling
        fmt::print("NO KEYWORD HEADER");
    }

    KeywordData result;

    BHF::Keyword keyword = readType<BHF::Keyword>();

    result.up = keyword.up_context;
    result.down = keyword.down_context;

    for (int i = 0; i < keyword.count; ++i) {
        result.contexts.push_back(readType<u16>());
    }

    return result;
}

template<typename T>
T File::readType() noexcept
{
    T result;

    size_t bytes_read = fread(&result, 1, sizeof(T), d->file);

    if (bytes_read != sizeof(T)) {
        // TODO: better error handling (erro code?)
        d->last_error = fmt::format("Short read, trying to read {} bytes got {} bytes.", sizeof(T), bytes_read);
    }

    return result;
}

void
File::parse() noexcept
{
    // [Stamp]
    readString(d->stamp);

    u8 end_of_stamp;

    fread(&end_of_stamp, 1, sizeof(end_of_stamp), d->file);

    if (end_of_stamp != 0x1a) {
        // TODO: error handling
        fmt::print("EOS ERROR\n");
    }

    // [Signature]
    readString(d->signature);

    // [Version]
    d->version = readType<Version>();

    // [File header]
    RecordHeader record = readType<RecordHeader>();

    if (record.type != RecordHeader::FileHeader) {
        // TODO: error handling
        fmt::print("NO FILE HEADER");
        std::exit(1);
    }

    d->file_header = readType<FileHeader>();

    // [Compression]
    record = readType<RecordHeader>();

    if (record.type != RecordHeader::Compression) {
        // TODO: error handling
        fmt::print("NO COMPRESSION");
        std::exit(1);
    }

    d->compression = readType<Compression>();

    // [Context]
    record = readType<RecordHeader>();

    if (record.type != RecordHeader::Context) {
        // TODO: error handling
        fmt::print("NOT CONTEXT");
        std::exit(1);
    }

    d->context.clear();

    u16 context_count = readType<u16>();

    for (u16 i = 0; i < context_count; ++i) {
        i32 offset = readType<u8>();
        offset |= readType<u8>() << 8u;
        offset |= readType<i8>() << 16u;

        d->context.push_back(offset);
    }

    // [Index]
    record = readType<RecordHeader>();

    if (record.type != RecordHeader::Index) {
        // TODO: error handling
        fmt::print("NOT INDEX");
        std::exit(1);
    }

    d->index.clear();

    u16 index_count = readType<u16>();

    std::string previous_index;

    for (u16 i = 0; i < index_count; ++i) {
        u8 length = readType<u8>();
        u8 carry = static_cast<u8>(length >> 5u);

        length &= 0x1f;

        std::string chars;

        if (carry) {
            chars = previous_index.substr(0, carry);
        }

        chars.reserve(chars.size() + static_cast<std::string::size_type>(length));

        while (length-- > 0) {
            chars += BHF_CP437toUTF8(readType<u8>());
        }

        File::ContextType context = readType<u16>();

        d->index.push_back({context, chars});

        previous_index = chars;
    }

    // TODO: Indextags Record {introduced in BP7}
}

std::string
BHF_CP437toUTF8(u8 character)
{
    switch (character) {
        case 0x01: return "\u263a";
        case 0x02: return "\u263b";
        case 0x03: return "\u2665";
        case 0x04: return "\u2666";
        case 0x05: return "\u2663";
        case 0x06: return "\u2660";
        case 0x07: return "\u2022";
        case 0x08: return "\u25d8";
        case 0x09: return "\u25cb";
        case 0x0a: return "\u25d9";
        case 0x0b: return "\u2642";
        case 0x0c: return "\u2640";
        case 0x0d: return "\u266a";
        case 0x0e: return "\u266b";
        case 0x0f: return "\u263c";

        case 0x10: return "\u25ba";
        case 0x11: return "\u25c4";
        case 0x12: return "\u2195";
        case 0x13: return "\u203c";
        case 0x14: return "\u00b6";
        case 0x15: return "\u00a7";
        case 0x16: return "\u25ac";
        case 0x17: return "\u21a8";
        case 0x18: return "\u2191";
        case 0x19: return "\u2193";
        case 0x1a: return "\u2192";
        case 0x1b: return "\u2190";
        case 0x1c: return "\u221f";
        case 0x1d: return "\u2194";
        case 0x1e: return "\u25b2";
        case 0x1f: return "\u25bc";

        case 0x7f: return "\u2302";

        case 0x80: return "\u00c7";
        case 0x81: return "\u00fc";
        case 0x82: return "\u00e9";
        case 0x83: return "\u00e2";
        case 0x84: return "\u00e4";
        case 0x85: return "\u00e0";
        case 0x86: return "\u00e5";
        case 0x87: return "\u00e7";
        case 0x88: return "\u00ea";
        case 0x89: return "\u00eb";
        case 0x8a: return "\u00e8";
        case 0x8b: return "\u00ef";
        case 0x8c: return "\u00ee";
        case 0x8d: return "\u00ec";
        case 0x8e: return "\u00c4";
        case 0x8f: return "\u00c5";

        case 0x90: return "\u00c9";
        case 0x91: return "\u00e6";
        case 0x92: return "\u00c6";
        case 0x93: return "\u00f4";
        case 0x94: return "\u00f6";
        case 0x95: return "\u00f2";
        case 0x96: return "\u00fb";
        case 0x97: return "\u00f9";
        case 0x98: return "\u00ff";
        case 0x99: return "\u00d6";
        case 0x9a: return "\u00dc";
        case 0x9b: return "\u00a2";
        case 0x9c: return "\u00a3";
        case 0x9d: return "\u00a5";
        case 0x9e: return "\u20a7";
        case 0x9f: return "\u0192";

        case 0xa0: return "\u00e1";
        case 0xa1: return "\u00ed";
        case 0xa2: return "\u00f3";
        case 0xa3: return "\u00fa";
        case 0xa4: return "\u00f1";
        case 0xa5: return "\u00d1";
        case 0xa6: return "\u00aa";
        case 0xa7: return "\u00ba";
        case 0xa8: return "\u00bf";
        case 0xa9: return "\u2310";
        case 0xaa: return "\u00ac";
        case 0xab: return "\u00bd";
        case 0xac: return "\u00bc";
        case 0xad: return "\u00a1";
        case 0xae: return "\u00ab";
        case 0xaf: return "\u00bb";

        case 0xb0: return "\u2591";
        case 0xb1: return "\u2592";
        case 0xb2: return "\u2593";
        case 0xb3: return "\u2502";
        case 0xb4: return "\u2524";
        case 0xb5: return "\u2561";
        case 0xb6: return "\u2562";
        case 0xb7: return "\u2556";
        case 0xb8: return "\u2555";
        case 0xb9: return "\u2563";
        case 0xba: return "\u2551";
        case 0xbb: return "\u2557";
        case 0xbc: return "\u255d";
        case 0xbd: return "\u255c";
        case 0xbe: return "\u255b";
        case 0xbf: return "\u2510";

        case 0xc0: return "\u2514";
        case 0xc1: return "\u2534";
        case 0xc2: return "\u252c";
        case 0xc3: return "\u251c";
        case 0xc4: return "\u2500";
        case 0xc5: return "\u253c";
        case 0xc6: return "\u255e";
        case 0xc7: return "\u255f";
        case 0xc8: return "\u255a";
        case 0xc9: return "\u2554";
        case 0xca: return "\u2569";
        case 0xcb: return "\u2566";
        case 0xcc: return "\u2560";
        case 0xcd: return "\u2550";
        case 0xce: return "\u256c";
        case 0xcf: return "\u2567";

        case 0xd0: return "\u2568";
        case 0xd1: return "\u2564";
        case 0xd2: return "\u2565";
        case 0xd3: return "\u2559";
        case 0xd4: return "\u2558";
        case 0xd5: return "\u2552";
        case 0xd6: return "\u2553";
        case 0xd7: return "\u256b";
        case 0xd8: return "\u256a";
        case 0xd9: return "\u2518";
        case 0xda: return "\u250c";
        case 0xdb: return "\u2588";
        case 0xdc: return "\u2584";
        case 0xdd: return "\u258c";
        case 0xde: return "\u2590";
        case 0xdf: return "\u2580";

        case 0xe0: return "\u03b1";
        case 0xe1: return "\u00df";
        case 0xe2: return "\u0393";
        case 0xe3: return "\u03c0";
        case 0xe4: return "\u03a3";
        case 0xe5: return "\u03c3";
        case 0xe6: return "\u00b5";
        case 0xe7: return "\u03c4";
        case 0xe8: return "\u03a6";
        case 0xe9: return "\u0398";
        case 0xea: return "\u03a9";
        case 0xeb: return "\u03b4";
        case 0xec: return "\u221e";
        case 0xed: return "\u03c6";
        case 0xee: return "\u03b5";
        case 0xef: return "\u2229";

        case 0xf0: return "\u2261";
        case 0xf1: return "\u00b1";
        case 0xf2: return "\u2265";
        case 0xf3: return "\u2264";
        case 0xf4: return "\u2320";
        case 0xf5: return "\u2321";
        case 0xf6: return "\u00f7";
        case 0xf7: return "\u2248";
        case 0xf8: return "\u00b0";
        case 0xf9: return "\u2219";
        case 0xfa: return "\u00b7";
        case 0xfb: return "\u221a";
        case 0xfc: return "\u207f";
        case 0xfd: return "\u00b2";
        case 0xfe: return "\u25a0";
        case 0xff: return "\u00a0";
    }

    if (character < 0x20 || character > 0x7e) {
        // TODO: error handling
        return fmt::format("[{}]", static_cast<i32>(character));
    }

    return std::string(1, static_cast<char>(character));
}

std::string
BHF_HTMLEncoding(u8 character)
{
    switch (character) {
        case 0x20: return kHtmlSpace;
        case 0x22: return "&quot;";
        case 0x26: return "&amp;";
        case 0x27: return "&#39;";
        case 0x2f: return "&#47;";
        case 0x3c: return "&lt;";
        case 0x3e: return "&gt;";
    }

    return BHF_CP437toUTF8(character);
}

} // namespace BHF
