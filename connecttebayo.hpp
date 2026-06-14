#pragma once
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

class Connecttebayo : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QVariantList networks READ networks NOTIFY networksChanged)
  Q_PROPERTY(QString state READ state NOTIFY stateChanged)
  Q_PROPERTY(QString connectedSsid READ connectedSsid NOTIFY stateChanged)

public:
  explicit Connecttebayo(QObject *parent = nullptr);

  Q_INVOKABLE void connectNetwork(const QString &networkPath);
  Q_INVOKABLE void fetchDevices();
  Q_INVOKABLE void disconnect();
  Q_INVOKABLE void fetchNetworks();

  QVariantList networks() const { return m_networks; }
  QString state() const { return m_state; }
  QString connectedSsid() const { return m_connectedSsid; }

signals:
  void networksChanged();
  void stateChanged();

private:
  QStringList m_devices;
  QVariantList m_networks;
  QString m_state;
  QString m_connectedSsid;
private slots:
  void onPropertiesChanged(const QString &interface, const QVariantMap &changed,
                           const QStringList &invalidated);
};
