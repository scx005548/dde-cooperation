#include "MachineModel.h"

MachineModel::MachineModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_roles({std::pair{NameRole, "name"}, std::pair{OSRole, "os"}}) {
}

void MachineModel::clear() {
    beginResetModel();
    m_machines.clear();
    endResetModel();
}

void MachineModel::addMachine(const Machine &machine) {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_machines << machine;
    endInsertRows();
}

QHash<int, QByteArray> MachineModel::roleNames() const {
    return m_roles;
}

int MachineModel::rowCount([[maybe_unused]] const QModelIndex &parent) const {
    return m_machines.count();
}

QVariant MachineModel::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_machines.count()) {
        return QVariant();
    }

    const auto &machine = m_machines[index.row()];
    if (role == NameRole) {
        return machine.name;
    } else if (role == OSRole) {
        return machine.os;
    }

    return QVariant();
}
