#pragma once
#include "./iwdagent.hpp"
#include "./networkmodel.hpp"
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

class Connecttebayo : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(NetworkModel *networks READ networks NOTIFY networksChanged)
  Q_PROPERTY(QString state READ state NOTIFY stateChanged)
  Q_PROPERTY(
      QString connectedSsid READ connectedSsid NOTIFY connectedSsidChanged)
  Q_PROPERTY(
      QString promptingPath READ promptingPath NOTIFY promptingPathChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
  explicit Connecttebayo(QObject *parent = nullptr);
  ~Connecttebayo() override;

  Q_INVOKABLE void connectNetwork(const QString &networkPath);
  Q_INVOKABLE void disconnect();
  Q_INVOKABLE void providePassphrase(const QString &passphrase);
  Q_INVOKABLE void cancelPassphrase();
  Q_INVOKABLE void fetchNetworks(bool triggerTheScan = true);

  NetworkModel *networks() { return &m_model; }
  QString state() const { return m_state; }
  QString connectedSsid() const { return m_connectedSsid; }
  QString errorMessage() const { return m_errorMessage; }
  QString promptingPath() const { return m_promptingPath; }

signals:
  void networksChanged();
  void stateChanged();
  void promptingPathChanged();
  void connectedSsidChanged();
  void errorMessageChanged();

private:
  NetworkModel m_model;
  QString m_state;
  QString m_connectedSsid;
  QString m_stationPath;
  QString m_promptingPath;
  QString m_errorMessage;
  bool m_connecting = false;

  QObject m_agentObject;
  IwdAgent *m_agent;

  QString getStationPath();
  void updateStationPath();
  void setPromptingPath(const QString &path);
  void setConnectedSsid(const QString &ssid);
  void setErrorMessage(const QString &error);

private slots:
  void onPropertiesChanged(const QString &interface, const QVariantMap &changed,
                           const QStringList &invalidated);
};
