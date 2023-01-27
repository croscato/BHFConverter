// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Gustavo Ribeiro Croscato

#include "index.hpp"

namespace GUI {
namespace Model {

struct IndexData {
    const BHF::File::IndexContainer *container = nullptr;
};

Index::Index(QObject *parent) noexcept
    : QAbstractItemModel{parent}
    , d{std::make_unique<IndexData>()}
{}

Index::~Index() noexcept = default;

QModelIndex
Index::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return {};
    }

    if (!parent.isValid()) {
        return createIndex(row, column);
    }

    return createIndex(row, column, d->container);
}

QModelIndex
Index::parent(const QModelIndex &child) const
{
    if (child.isValid() && !child.parent().isValid()) {
        const BHF::File::IndexType *data = &d->container->at(static_cast<BHF::File::IndexContainer::size_type>(child.row()));

        return createIndex(child.row(), child.column(), data);
    }

    return {};
}

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

    return QAbstractItemModel::headerData(section, orientation, role);
}

int
Index::rowCount(const QModelIndex &parent) const
{
    if (d->container) {
        if (!parent.isValid()) {
            return static_cast<int>(d->container->size());
        } else if (parent.internalPointer() != nullptr) {
            //auto row = static_cast<BHF::File::IndexContainer::size_type>(parent.row());
            //return static_cast<int>(d->container->at(row).contexts.size());

            BHF::File::IndexType *data = reinterpret_cast<BHF::File::IndexType *>(parent.internalPointer());

            return static_cast<int>(data->contexts.size());
        }
    }

    return 0;
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
    if (!d->container) return {};

    if (index.isValid()) {
        int row = index.row();
        int col = index.column();

        switch (role) {
            case Qt::DisplayRole: {
                BHF::File::IndexType data = d->container->at(static_cast<BHF::File::IndexContainer::size_type>(row));

                if (col == 0) {
                    return QString::fromStdString(data.index);
                } else if (col == 1) {
                    return QVariant::fromValue<BHF::File::ContextType>(data.contexts.at(0));
                }

                return {};
            }
        }
    }

    return {};
}

void
Index::update(const BHF::File::IndexContainer *container) noexcept
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
