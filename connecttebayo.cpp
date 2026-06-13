#include "./connecttebayo.hpp"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

Connecttebayo::Connecttebayo(QObject *parent) : QObject(parent) {}

void Connecttebayo::fetchDevices() {
  QDBusConnection bus = QDBusConnection::systemBus();
  if (!bus.isConnected()) {
    qWarning() << "Can't connect to system bus";
    return;
  }
  QDBusInterface station("net.connman.iwd", "/net/connman/iwd/0/5",
                         "org.freedesktop.DBus.Properties", bus);

  QDBusReply<QVariant> state =
      station.call("Get", "net.connman.iwd.Station", "State");

  if (state.isValid()) {
    qDebug() << "Stateion state: " << state.value().toString();
  } else {
    qWarning() << "Failed to get state: " << state.error().message();
  }
}
