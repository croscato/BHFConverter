// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Gustavo Ribeiro Croscato

#include "index.hpp"

namespace GUI {
namespace Model {

struct IndexData {
    BHF::File::IndexContainer container;
};

Index::Index(QObject *parent) noexcept
    : QAbstractTableModel{parent}
      , d{std::make_unique<IndexData>()}
{}

Index::~Index() noexcept = default;

QVariant
Index::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch (section) {
                case 0: return tr("Index");
                case 1: return tr("Context");
            }
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);

}

int
Index::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return static_cast<int>(d->container.size());
}

int
Index::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 2;
}

QVariant
Index::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        int row = index.row();
        int col = index.column();

        switch (role) {
            case Qt::DisplayRole: {
                BHF::File::IndexType data = d->container.at(static_cast<BHF::File::IndexContainer::size_type>(row));

                if (col == 0) {
                    return QString::fromUtf8(data.index.c_str(), static_cast<qsizetype>(data.index.size()));
                } else if (col == 1) {
                    return QVariant::fromValue<decltype(data.context)>(data.context);
                }
            }
        }
    }

    return {};
}

void
Index::update(const BHF::File::IndexContainer &container) noexcept
{
    beginResetModel();
    d->container = container;
    endResetModel();
}

IndexFilter::IndexFilter(QObject *parent) noexcept
    : QSortFilterProxyModel(parent)
{}

} // namespace Model
} // namespace GUI
