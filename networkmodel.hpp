#pragma once
#include <QAbstractListModel>
#include <QList>

struct NetworkEntry {
  QString ssid;
  QString path;

  int signal;
  bool connected;
};

class NetworkModel : public QAbstractListModel {
  Q_OBJECT

public:
  enum Roles {
    SsidRole = Qt::UserRole + 1,
    PathRole,
    SignalRole,
    ConnectedRole,
  };
  explicit NetworkModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  void setNetworks(const QList<NetworkEntry> &networks);
  void updateConnected(const QString &path, bool connected);
  void clear();

private:
  QList<NetworkEntry> m_networks;
};
