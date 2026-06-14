#pragma once
#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QObject>
#include <QString>

class IwdAgent : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "net.connman.iwd.Agent")

public:
  explicit IwdAgent(QObject *parent = nullptr);

  Q_INVOKABLE QString RequestPassphrase(const QDBusObjectPath &network);
  Q_INVOKABLE void Cancel(const QString &reason);
  Q_INVOKABLE void Release();

signals:
  void passphraseRequested(const QString &networkPath);
};
