#pragma once
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

class Connecttebayo : public QObject {
  Q_OBJECT
  QML_ELEMENT
public:
  explicit Connecttebayo(QObject *parent = nullptr);

  Q_INVOKABLE void connectNetwork(const QString &networkPath);
  Q_INVOKABLE void fetchDevices();
  Q_INVOKABLE void disconnect();
  Q_INVOKABLE void fetchNetworks();

signals:
  void devicesChanged();

private:
  QStringList m_devices;
  QVariantList m_networks;
};
