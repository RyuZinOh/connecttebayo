#include "./iwdagent.hpp"
#include <QDebug>

IwdAgent::IwdAgent(QObject *parent) : QDBusAbstractAdaptor(parent) {}

QString IwdAgent::RequestPassphrase(const QDBusObjectPath &network) {
  qDebug() << "IWD requesting passphrase for: " << network.path();
  emit passphraseRequested(network.path());
  return QString();
};

void IwdAgent::Cancel(const QString &reason) {
  qDebug() << "IWD agent cancelled: " << reason;
}

void IwdAgent::Release() { qDebug() << "IWD agent released"; }
