#pragma once
#include <QDBusAbstractAdaptor>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QObject>
#include <QString>

class IwdAgent : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "net.connman.iwd.Agent")

public:
  explicit IwdAgent(QObject *parent = nullptr);
  void sendPassphrase(const QString &passphrase);

public slots:
  QString RequestPassphrase(const QDBusObjectPath &network,
                            const QDBusMessage &msg);

  void Cancel(const QString &reason);
  void Release();

signals:
  void passphraseRequested(const QString &networkPath);

private:
  QDBusMessage m_pendingMessage;
};
