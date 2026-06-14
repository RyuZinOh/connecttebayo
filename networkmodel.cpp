#include "./networkmodel.hpp"

NetworkModel::NetworkModel(QObject *parent) : QAbstractListModel(parent) {}
int NetworkModel::rowCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  return m_networks.size();
}

QVariant NetworkModel::data(const QModelIndex &index, int role) const {

  if (!index.isValid() || index.row() >= m_networks.size())
    return {};

  const NetworkEntry &e = m_networks[index.row()];
  switch (role) {
  case SsidRole:
    return e.ssid;
  case PathRole:
    return e.path;
  case SignalRole:
    return e.signal;
  case ConnectedRole:
    return e.connected;
  default:
    return {};
  }
}

QHash<int, QByteArray> NetworkModel::roleNames() const {
  return {{SsidRole, "ssid"},
          {PathRole, "path"},
          {SignalRole, "signal"},
          {ConnectedRole, "connected"}};
}

void NetworkModel::updateConnected(const QString &path, bool connected) {
  for (int i = 0; i < m_networks.size(); i++) {
    if (m_networks[i].path == path) {
      m_networks[i].connected = connected;
      QModelIndex idx = index(i);
      emit dataChanged(idx, idx, {ConnectedRole});
      return;
    }
  }
}

void NetworkModel::clear() {
  beginResetModel();
  m_networks.clear();
  endResetModel();
}
void NetworkModel::setNetworks(const QList<NetworkEntry> &networks) {
  beginResetModel();
  m_networks = networks;
  endResetModel();
}
