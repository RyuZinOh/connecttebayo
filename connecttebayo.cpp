#include "./connecttebayo.hpp"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcess>

Connecttebayo::Connecttebayo(QObject *parent) : QObject(parent) {}

// scanning networks
void Connecttebayo::fetchNetworks() {
  QDBusConnection bus = QDBusConnection::systemBus();
  QDBusInterface station("net.connman.iwd", "/net/connman/iwd/0/5",
                         "net.connman.iwd.Station", bus);

  station.call("Scan");

  // ordered network viw busctl
  QProcess p;
  p.start("busctl", {"call", "net.connman.iwd", "/net/connman/iwd/0/5",
                     "net.connman.iwd.Station", "GetOrderedNetworks"});
  p.waitForFinished(5000);
  QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();

  // first splitting by space
  QStringList tokens = out.split(' ', Qt::SkipEmptyParts);

  /*
    start from 2, prior  is kinda ignored  where the first or 2 is the path
   of network, 3 is signal, 4 is secondary path, and 5 is seconary network
   signal and so on...
  */
  for (int i = 2; i < tokens.size() - 1; i = i + 2) {
    QString path = tokens[i].remove('"');
    QString signal = tokens[i + 1];

    QDBusInterface network("net.connman.iwd", path,
                           "org.freedesktop.DBus.Properties", bus);

    QDBusReply<QVariant> name =
        network.call("Get", "net.connman.iwd.Network", "Name");
    QDBusReply<QVariant> connected =
        network.call("Get", "net.connman.iwd.Network", "Connected");

    qDebug() << "\nSSID:" << name.value().toString() << "\nSignal: " << signal
             << "\nConnected: " << connected.value().toBool()
             << "\nPath: " << path;
  }
}

void Connecttebayo::connectNetwork(const QString &networkPath) {
  QDBusConnection bus = QDBusConnection::systemBus();
  QDBusInterface network("net.connman.iwd", networkPath,
                         "net.connman.iwd.Network", bus);

  QDBusReply<void> reply = network.call("Connect");
  if (!reply.isValid()) {
    qWarning() << "Connect failure: " << reply.error().message();
  } else {
    qDebug() << "Connected to: " << networkPath;
  }
}

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
    qDebug() << "Station state: " << state.value().toString();
  } else {
    qWarning() << "Failed to get state: " << state.error().message();
  }

  //  connected network name
  QDBusReply<QVariant> connectedNet =
      station.call("Get", "net.connman.iwd.Station", "ConnectedNetwork");

  if (connectedNet.isValid()) {
    QDBusObjectPath netPath = connectedNet.value().value<QDBusObjectPath>();
    qDebug() << "Connected Network Path: " << netPath.path();

    QDBusInterface network("net.connman.iwd", netPath.path(),
                           "org.freedesktop.DBus.Properties", bus);
    QDBusReply<QVariant> name =
        network.call("Get", "net.connman.iwd.Network", "Name");

    if (name.isValid()) {
      qDebug() << "Connected SSID: " << name.value().toString();
    }
  }
}

void Connecttebayo::disconnect() {
  QDBusConnection bus = QDBusConnection::systemBus();
  QDBusInterface station{"net.connman.iwd", "/net/connman/iwd/0/5",
                         "net.connman.iwd.Station", bus};

  QDBusReply<void> reply = station.call("Disconnect");
  if (!reply.isValid()) {
    qWarning() << "Disconnect failure: " << reply.error().message();
  } else {
    qDebug() << "Disconnected!";
  }
}
