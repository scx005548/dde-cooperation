#ifndef MACHINE_MODEL_H
#define MACHINE_MODEL_H

#include <QString>
#include <QAbstractListModel>

struct Machine {
    QString name;
    QString os;
};

class MachineModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum MachineRoles { NameRole = Qt::UserRole + 1, OSRole };

    MachineModel(QObject *parent = nullptr);

    void clear();
    void addMachine(const Machine &machine);

    virtual QHash<int, QByteArray> roleNames() const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private:
    QHash<int, QByteArray> m_roles;
    QList<Machine> m_machines;
};

#endif // !MACHINE_MODEL_H
