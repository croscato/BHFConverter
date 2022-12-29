// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Gustavo Ribeiro Croscato

#include <sys/stat.h>

#define EXTRACT_CONTEXT 0
#define EXTRACT_INDEX 0
#define EXTRACT_TEXT 0

constexpr u8 kNibbleRaw = 0x0F;
constexpr u8 kNibbleRep = 0x0E;

#pragma pack(push,1)

struct BHFVersion {
    enum Format : u8 {
          BP7 = 0x34
        , TP6 = 0x33
        , TP4 = 0x04 // Turbo C++ 3.0
        , TP2 = 0x02
    };

    Format format;
    u8 text;
};

struct BHFHeader {
    enum Type : u8 {
          FileHeader
        , Context
        , Text
        , Keyword
        , Index
        , Compression
        , IndexTags
    };

    Type type;
    u16 length;
};

struct BHFRecordFile {
    u16 options;
    u16 main_index;
    u16 largest_record;
    u8 height;
    u8 width;
    u8 left_margin;
};

struct BHFRecordCompression {
    enum Type : u8 {
        Nibble = 2
    };

    Type type;
    u8 table[14];
};

struct BHFKeyword {
    u16 up_context;
    u16 down_context;
    u16 count;
};

#pragma pack(pop)

static std::string_view BHF_ReadString(char **buffer, size_t size = 0);
template<typename T> static T * BHF_ReadPointer(char **buffer);
template<typename T> static T BHF_ReadValue(char **buffer);
const char * BHF_CP437toUTF8(u8 character);
static std::string BHF_StringNormalize(const char *buffer, size_t size);
static std::string BHF_Uncompress(char **buffer, size_t size, const BHFRecordCompression *compression);

struct FileData {
    i64 size;
    std::unique_ptr<char, decltype(free)*> data;
};

FileData File_Read(std::string_view filepath);

int
main(void)
{
    // -- Database --
    sqlite3 *db = nullptr;

    if (sqlite3_open("database.db", &db) != SQLITE_OK) {
        fmt::print("SQlite3: {}\n", sqlite3_errmsg(db));
        exit(1);
    }

    {
        FileData sql_file = File_Read("database.sql");

        char *error_message;

        if (sqlite3_exec(db, sql_file.data.get(), nullptr, nullptr, &error_message) != SQLITE_OK) {
            fmt::print("SQlite3: {}\n", error_message);
            sqlite3_free(error_message);
            exit(1);
        }
    }

    // -- Input --
    FileData help_file = File_Read("data/tchelp.tch");

    // -- Parsing --
    char *cursor = help_file.data.get();
    char *end = cursor + help_file.size;

    std::string_view stamp = BHF_ReadString(&cursor);
    fmt::print("stamp.........: {}\n", stamp);

    std::string_view signature = BHF_ReadString(&cursor);
    fmt::print("signature.....: {}\n", signature);

    const BHFVersion *version = BHF_ReadPointer<BHFVersion>(&cursor);
    fmt::print("version.......: {} {:x}\n", version->text, static_cast<u32>(version->format));

    const BHFRecordCompression *compression = nullptr;

    i64 context_id = 0;

    while (cursor < end) {
        i64 offset = cursor - help_file.data.get();

        const BHFHeader *header = BHF_ReadPointer<BHFHeader>(&cursor);

        if (header->length == 0) {
            fmt::print("cursor {:p} {:p}\n", cursor, help_file.data.get() + help_file.size);
            exit(0);
        }

        if (header->type == BHFHeader::FileHeader) {
            const BHFRecordFile *file = BHF_ReadPointer<BHFRecordFile>(&cursor);

            fmt::print("--{{ FileHeader }}--\n");
            fmt::print("  options.....: {}\n", file->options);
            fmt::print("  main index..: {}\n", file->main_index);
            fmt::print("  largest rec.: {}\n", file->largest_record);
            fmt::print("  screen size.: {} x {}\n", file->height, file->width);
            fmt::print("  left margin.: {}\n", file->left_margin);

            continue;
        }

        if (header->type == BHFHeader::Compression) {
            compression = BHF_ReadPointer<BHFRecordCompression>(&cursor);

            std::string table = BHF_StringNormalize(reinterpret_cast<const char *>(compression->table), sizeof(compression->table));

            fmt::print("--{{ Compression }}--\n");
            fmt::print("  type........: {}\n", static_cast<u32>(compression->type));
            fmt::print("  table.......: |{}| ", table);

            for (int i = 0; i < 14; ++i) {
                fmt::print("{:02x} ", compression->table[i]);
            }

            fmt::print("\n");

            continue;
        }

        if (header->type == BHFHeader::Context) {
#if EXTRACT_CONTEXT 
            fmt::print("--{{ Context }}--\n");
            u16 count = BHF_ReadValue<u16>(&cursor);

            fmt::print("  count.......: {}\n", count);

            sqlite3_stmt *stmt = nullptr;

            sqlite3_exec(db, "BEGIN_TRANSACTION", nullptr, nullptr, nullptr);
            sqlite3_prepare(db, "INSERT INTO tbl_context (context_offset) VALUES (?)", -1, &stmt, nullptr);

            for (u16 i = 0; i < count; ++i) {
                u32 index = static_cast<u8>(*cursor++);
                index |= static_cast<u32>(static_cast<u8>((*cursor++)) << 8u);
                index |= static_cast<u32>(static_cast<u8>(*cursor++) << 16u);

                sqlite3_reset(stmt);
                sqlite3_clear_bindings(stmt);
                sqlite3_bind_int64(stmt, 1, index);

                if (sqlite3_step(stmt) == SQLITE_ERROR) {
                    fmt::print("Context insert error: {}\n", sqlite3_errmsg(db));
                    fmt::print("Current context: {} {}\n", i, index);
                }
            }

            sqlite3_finalize(stmt);
            sqlite3_exec(db, "END_TRANSACTION", nullptr, nullptr, nullptr);
#else
            cursor += header->length;
#endif
            continue;
        }

        if (header->type == BHFHeader::Index) {
#if EXTRACT_INDEX 
            fmt::print("--{{ Index }}--\n");
            u16 count = BHF_ReadValue<u16>(&cursor);
            fmt::print("  count.......: {}\n", count);

            sqlite3_stmt *stmt = nullptr;

            sqlite3_exec(db, "BEGIN_TRANSACTION", nullptr, nullptr, nullptr);
            sqlite3_prepare(db, "INSERT INTO tbl_index (context_id, index_value) VALUES (?, ?)", -1, &stmt, nullptr);

            std::string previous_index;

            for (u16 i = 0; i < count; ++i) {
                u8 length = BHF_ReadValue<u8>(&cursor)  ;
                u8 carry = static_cast<u8>(length >> 5);

                length &= 0x1f;

                std::string chars;

                if (carry) {
                    chars = previous_index.substr(0, carry);
                }

                chars += BHF_StringNormalize(cursor, length);

                cursor += length;

                u16 context = BHF_ReadValue<u16>(&cursor);

                sqlite3_reset(stmt);
                sqlite3_clear_bindings(stmt);
                sqlite3_bind_int64(stmt, 1, context);
                sqlite3_bind_text(stmt, 2, chars.c_str(), static_cast<int>(chars.size()), nullptr);
                
                if (sqlite3_step(stmt) == SQLITE_ERROR) {
                    fmt::print("Index insert error: {}\n", sqlite3_errmsg(db));
                    fmt::print("Current index: {} {}\n", i, chars);
                }

                previous_index = chars;
            }

            sqlite3_finalize(stmt);
            sqlite3_exec(db, "END_TRANSACTION", nullptr, nullptr, nullptr);
#else
            cursor += header->length;
#endif
            continue;
        }

        if (header->type == BHFHeader::Text) {
#if EXTRACT_TEXT
            fmt::print("--{{ Text }}--\n");
            sqlite3_stmt *stmt = nullptr;

            sqlite3_prepare(db, "SELECT context_id FROM tbl_context WHERE context_offset = ?", -1, &stmt, nullptr);
            sqlite3_bind_int64(stmt, 1, offset);
            sqlite3_step(stmt);

            context_id = sqlite3_column_int64(stmt, 0);

            sqlite3_finalize(stmt);

            sqlite3_exec(db, "BEGIN_TRANSACTION", nullptr, nullptr, nullptr);
            sqlite3_prepare(db, "INSERT INTO tbl_text (context_id, text_value) VALUES (?, ?)", -1, &stmt, nullptr);

            std::string text = BHF_Uncompress(&cursor, header->length, compression);

            sqlite3_bind_int64(stmt, 1, context_id);
            sqlite3_bind_blob(stmt, 2, text.c_str(), static_cast<int>(text.size()), nullptr);

            if (sqlite3_step(stmt) == SQLITE_ERROR) {
                fmt::print("Text insert error: {}\n", sqlite3_errmsg(db));
                fmt::print("Current context: {}\n", context_id);
            }

            sqlite3_finalize(stmt);

            sqlite3_exec(db, "END_TRANSACTION", nullptr, nullptr, nullptr);
#else
            cursor += header->length;
#endif
            continue;
        }

        if (header->type == BHFHeader::Keyword) {
#if EXTRACT_TEXT
            fmt::print("--{{ Keyword }}--\n");
            BHFKeyword *keyword = BHF_ReadPointer<BHFKeyword>(&cursor);
            fmt::print("  count.......: {}\n", keyword->count);

            sqlite3_stmt *stmt = nullptr;

            sqlite3_exec(db, "BEGIN_TRANSACTION", nullptr, nullptr, nullptr);
            sqlite3_prepare(db, "INSERT INTO tbl_keyword (context_id, keyword_up_context, keyword_down_context) VALUES (?, ?, ?)", -1, &stmt, nullptr);

            sqlite3_bind_int64(stmt, 1, context_id);

            if (keyword->up_context > 0) {
                sqlite3_bind_int64(stmt, 2, keyword->up_context);
            }

            if (keyword->down_context > 0) {
                sqlite3_bind_int64(stmt, 3, keyword->down_context);
            }

            if (sqlite3_step(stmt) == SQLITE_ERROR) {
                fmt::print("Keyword insert error: {}\n", sqlite3_errmsg(db));
            }
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "END_TRANSACTION", nullptr, nullptr, nullptr);

            sqlite3_exec(db, "BEGIN_TRANSACTION", nullptr, nullptr, nullptr);
            if (sqlite3_prepare(db, "INSERT INTO tbl_keyword_list (context_id, keyword_index, keyword_context) VALUES (?, ?, ?)", -1, &stmt, nullptr) == SQLITE_ERROR) {
                fmt::print("PREPARE ERROR: {}\n", sqlite3_errmsg(db));
                exit(0);
            }

            for (i32 index = 0; index < keyword->count; ++index) {
                sqlite3_reset(stmt);
                sqlite3_clear_bindings(stmt);
                sqlite3_bind_int64(stmt, 1, context_id);
                sqlite3_bind_int64(stmt, 2, index);
                sqlite3_bind_int64(stmt, 3, BHF_ReadValue<u16>(&cursor));
                    
                if (sqlite3_step(stmt) == SQLITE_ERROR) {
                    fmt::print("Keyword index insert error: {}\n", sqlite3_errmsg(db));
                }
            }

            sqlite3_finalize(stmt);
            sqlite3_exec(db, "END_TRANSACTION", nullptr, nullptr, nullptr);
#else
            cursor += header->length;
#endif
            continue;
        }

        if (header->type == BHFHeader::IndexTags) {
            fmt::print("--{{ IndexTags }}--\n");

            cursor += header->length;

            continue;
        }

        // ERROR: should not be reachable
        fmt::print("header........: {} {}\n", static_cast<u32>(header->type), header->length);
    }

    sqlite3_close(db);

    return 0;
}

std::string_view
BHF_ReadString(char **buffer, size_t size)
{
    if (size == 0) {
        size = strlen(*buffer) + 1;
    }

    std::string_view result(*buffer, size);
    *buffer += result.length();

    return result;
}

template<typename T>
T *
BHF_ReadPointer(char **buffer)
{
    T *result = reinterpret_cast<T *>(*buffer);
    *buffer += sizeof(T);

    return result;
}

template<typename T>
T
BHF_ReadValue(char **buffer)
{
    T result = *reinterpret_cast<T *>(*buffer);
    *buffer += sizeof(T);

    return result;
}

// reference: https://en.wikipedia.org/wiki/Code_page_437
const char *
BHF_CP437toUTF8(u8 character)
{
    static char default_char[2] = {0, 0};

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

        default: {
            if (character == 0x00) {
                default_char[0] = '\n';
            } else if (character < 0x20 || character > 0x7e) {
                // ERROR
            } else {
                default_char[0] = static_cast<char>(character);
            }

            return default_char;
        }
    }
}

std::string
BHF_StringNormalize(const char *buffer, size_t size)
{
    std::string result;

    result.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        result += BHF_CP437toUTF8(static_cast<u8>(buffer[i]));
    }

    return result;

}

std::string
BHF_Uncompress(char **buffer, size_t size, const BHFRecordCompression *compression)
{
    std::string result;

    const u8 *nibbles = reinterpret_cast<const u8 *>(*buffer);
    const u8 *end = nibbles + size;

    i32 count = 0;
    i64 i = 0;

    while (nibbles < end) {
        u8 nibble = (++i & 0x01) ? *nibbles & 0x0f : (*nibbles++ >> 4) & 0x0f;
        u8 value = 0;

        if (nibble == kNibbleRaw) {
            u8 n1 = (++i & 0x01) ? *nibbles & 0x0f : (*nibbles++ >> 4) & 0x0f;
            u8 n2 = (++i & 0x01) ? *nibbles & 0x0f : (*nibbles++ >> 4) & 0x0f;

            value = static_cast<u8>((n2 << 4) | n1);
            count += 1;
        } else if (nibble == kNibbleRep) {
            count = ((++i & 0x01) ? *nibbles & 0x0f : (*nibbles++ >> 4) & 0x0f) + 1;

            continue;
        } else {
            value = compression->table[nibble];
            count += 1;
        }

        if (value == 0x01 || value == 0x02) {
            count = 0;

            continue;
        }

        const char *out = BHF_CP437toUTF8(value);

        while (count > 0) {
            result += out;

            --count;
        }
    }

    *buffer += size;

    return result;
}

FileData
File_Read(std::string_view filepath)
{
    FileData result {0, {nullptr, free}};

    struct stat file_stat;

    if (stat(filepath.data(), &file_stat) != 0) {
        fmt::print("Can't get file information for '{}'\n", filepath);
        exit(1);
    }

    result.size = file_stat.st_size;
    result.data.reset(reinterpret_cast<char *>(malloc(static_cast<size_t>(file_stat.st_size + 1))));

    if (!result.data) {
        fmt::print("Can't allocate {} bytes", file_stat.st_size);
        exit(2);
    }

    FILE *input_file = fopen(filepath.data(), "rb");

    if (!input_file) {
        fmt::print("Can't open input file '{}'\n", filepath);
        exit(3);
    }

    u64 bytes_read = fread(result.data.get(), 1, static_cast<size_t>(file_stat.st_size), input_file);

    fclose(input_file);

    if (bytes_read != static_cast<u64>(file_stat.st_size)) {
        fmt::print("Trying to read {} bytes from input but got {} bytes", file_stat.st_size, bytes_read);
        exit(4);
    }

    result.data.get()[result.size] = '\0';

    return result;
}

