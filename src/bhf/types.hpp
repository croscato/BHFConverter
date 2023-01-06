// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Gustavo Ribeiro Croscato

#ifndef BHFCONVERTER_SRC_BHF_TYPES_HPP
#define BHFCONVERTER_SRC_BHF_TYPES_HPP 1

namespace BHF {

#pragma pack(push,1)

struct Version {
    enum Format : u8 {
          TP2 = 0x02
        , TP4 = 0x04
        , TP6 = 0x33
        , BP7 = 0x34
        , Invalid
    };

    Format format;
    u8 text;
};

struct FileHeader {
    u16 options;
    u16 main_index;
    u16 largest_record;
    u8 height;
    u8 width;
    u8 left_margin;
};

struct RecordHeader {
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

struct Compression {
    enum Type : u8 {
          Nibble = 2
        , Invalid
    };

    Type type;
    u8 table[14];
};

struct Keyword {
    u16 up_context;
    u16 down_context;
    u16 count;
};

#pragma pack(pop)

} // namespace BHF

#endif // BHFCONVERTER_SRC_BHF_TYPES_HPP
