// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Gustavo Ribeiro Croscato

#ifndef BHFCONVERTER_SRC_GUI_MODEL_INDEX_HPP
#define BHFCONVERTER_SRC_GUI_MODEL_INDEX_HPP 1

#include "bhf/file.hpp"

namespace GUI {
namespace Model {

struct IndexData;

class Index : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit Index(QObject *parent = nullptr) noexcept;
    ~Index() noexcept;

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    void update(const BHF::File::IndexContainer &container) noexcept;

private:
    std::unique_ptr<IndexData> d;
};

class IndexFilter : public QSortFilterProxyModel {
public:
    explicit IndexFilter(QObject *parent = nullptr) noexcept;
};

} // namespace Model
} // namespace GUI

#endif // BHFCONVERTER_SRC_GUI_MODEL_INDEX_HPP
