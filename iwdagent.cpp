#include "./iwdagent.hpp"
#include <QDBusConnection>
#include <QDebug>

IwdAgent::IwdAgent(QObject *parent) : QDBusAbstractAdaptor(parent) {}

QString IwdAgent::RequestPassphrase(const QDBusObjectPath &network,
                                    const QDBusMessage &msg) {
  qDebug() << "IWD requesting passphrase for: " << network.path();
  msg.setDelayedReply(true);
  m_pendingMessage = msg;
  emit passphraseRequested(network.path());
  return QString();
};

void IwdAgent::sendPassphrase(const QString &passphrase) {
  if (m_pendingMessage.type() == QDBusMessage::InvalidMessage) {
    return;
  }
  QDBusMessage reply = m_pendingMessage.createReply(passphrase);
  QDBusConnection::systemBus().send(reply);
  m_pendingMessage = QDBusMessage();
}

void IwdAgent::Cancel(const QString &reason) {
  Q_UNUSED(reason);
  m_pendingMessage = QDBusMessage();
  emit passphraseRequested("");
  qDebug() << "IWD agent cancelled: " << reason;
}

void IwdAgent::Release() {
  m_pendingMessage = QDBusMessage();
  qDebug() << "IWD agent released";
}
