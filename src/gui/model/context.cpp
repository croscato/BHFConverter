// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Gustavo Ribeiro Croscato

#include "context.hpp"

namespace GUI {
namespace Model {

struct ContextData {
    BHF::File::ContextContainer container;
};

Context::Context(QObject *parent) noexcept
    : QAbstractTableModel{parent}
    , d{std::make_unique<ContextData>()}
{}

Context::~Context() noexcept = default;

QVariant
Context::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch (section) {
                case 0: return tr("Context");
                case 1: return tr("Offset");
            }
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

int
Context::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return static_cast<int>(d->container.size());
}

int
Context::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 2;
}

QVariant
Context::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        int row = index.row();
        int col = index.column();

        switch (role) {
            case Qt::DisplayRole: {
                if (col == 0) {
                    return row;
                } else if (col == 1) {
                    return QVariant::fromValue<BHF::File::ContextType>(d->container.at(static_cast<BHF::File::ContextContainer::size_type>(row)));
                }
            }
        }
    }

    return {};
}

void Context::update(const BHF::File::ContextContainer &container) noexcept
{
    beginResetModel();
    d->container = container;
    endResetModel();
}

ContextFilter::ContextFilter(QObject *parent) noexcept
    : QSortFilterProxyModel(parent)
{}

} // namespace Model
} // namespace GUI
