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

  Q_INVOKABLE void fetchDevices();

signals:
  void devicesChanged();

private:
  QStringList m_devices;
};
