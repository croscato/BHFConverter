// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Gustavo Ribeiro Croscato

#ifndef BHFCONVERTER_SRC_GUI_MODEL_CONTEXT_HPP
#define BHFCONVERTER_SRC_GUI_MODEL_CONTEXT_HPP 1

#include "bhf/file.hpp"

namespace GUI {
namespace Model {

struct ContextData;

class Context : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit Context(QObject *parent = nullptr) noexcept;
    ~Context() noexcept;

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    void update(const BHF::File::ContextContainer &container) noexcept;

private:
    std::unique_ptr<ContextData> d;
};

class ContextFilter : public QSortFilterProxyModel {
public:
    explicit ContextFilter(QObject *parent = nullptr) noexcept;
};

} // namespace Model
} // namespace GUI

#endif // BHFCONVERTER_SRC_GUI_MODEL_CONTEXT_HPP
