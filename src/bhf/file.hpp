// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Gustavo Ribeiro Croscato

#ifndef BHFCONVERTER_SRC_BHF_FILE_HPP
#define BHFCONVERTER_SRC_BHF_FILE_HPP 1

#include "types.hpp"

namespace BHF {

struct FileData;

class File
{
public:
    enum TextFormat {
          PlainText
        , HTML
    };

    using ContextType = int;
    using ContextContainer = std::vector<ContextType>;

    using IndexType = struct {std::string index; std::vector<ContextType> contexts; };
    using IndexContainer = std::vector<IndexType>;

    File() noexcept;
    File(std::string_view filepath) noexcept;
    ~File() noexcept;

    bool open(std::string_view filepath) noexcept;

    const std::string &stamp() const noexcept;
    const std::string &signature() const noexcept;
    const Version &version() const noexcept;
    const FileHeader &fileHeader() const noexcept;
    const Compression &compression() const noexcept;
    const ContextContainer &context() const noexcept;
    const IndexContainer &index() const noexcept;
    std::string text(ContextType offset, TextFormat format = PlainText) noexcept;

    const std::string &lastError() const noexcept;

private:
    struct KeywordData {
        ContextType up;
        ContextType down;
        ContextContainer contexts;
    };

    std::string BHF_FormatAsText(const std::string &text) const;
    std::string BHF_FormatAsHTML(const std::string &text);

    std::string uncompress(const RecordHeader &record);
    void readString(std::string &str) noexcept;
    KeywordData readKeywords() noexcept;

    template<typename T>
    T readType() noexcept;

    void parse() noexcept;

    std::unique_ptr<FileData> d;
};

} // namespace BHF

#endif // BHFCONVERTER_SRC_BHF_FILE_HPP
