#include "./connecttebayo.hpp"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcess>

Connecttebayo::Connecttebayo(QObject *parent) : QObject(parent) {

  // agent register
  m_agent = new IwdAgent(&m_agentObject);
  QDBusConnection::systemBus().registerObject(
      "/com/ryuzinoh/connecttebayo/agent", &m_agentObject);

  connect(m_agent, &IwdAgent::passphraseRequested, this,
          &Connecttebayo::setPromptingPath);
  updateStationPath();
}

void Connecttebayo::onPropertiesChanged(const QString &interface,
                                        const QVariantMap &changed,
                                        const QStringList &invalidated) {
  Q_UNUSED(invalidated)
  qDebug() << "PropertiesChanged on: " << interface << changed.keys();

  if (interface == "net.connman.iwd.Station") {
    if (changed.contains("State")) {
      m_state = changed["State"].toString();
      emit stateChanged();
      fetchDevices();
      fetchNetworks();
    }
    if (changed.contains("ConnectedNetwork")) {
      fetchDevices();
      fetchNetworks();
    }
  }
}
// scanning networks
void Connecttebayo::fetchNetworks() {
  // updateStationPath();  // in case iwd somehow changes in runtime
  if (m_stationPath.isEmpty()) {
    return;
  }
  QDBusConnection bus = QDBusConnection::systemBus();
  QDBusInterface station("net.connman.iwd", m_stationPath,
                         "net.connman.iwd.Station", bus);

  station.call("Scan");

  // ordered network viw busctl
  QProcess p;
  p.start("busctl", {"call", "net.connman.iwd", m_stationPath,
                     "net.connman.iwd.Station", "GetOrderedNetworks"});
  p.waitForFinished(5000);
  QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();

  // first splitting by space
  QStringList tokens = out.split(' ', Qt::SkipEmptyParts);
  QList<NetworkEntry> entries;

  /*
    start from 2, prior  is kinda ignored  where the first or 2 is the path
   of network, 3 is signal, 4 is secondary path, and 5 is seconary network
   signal and so on...
  */
  for (int i = 2; i < tokens.size() - 1; i = i + 2) {
    QString path = tokens[i].remove('"');
    int signal = tokens[i + 1].toInt();

    QDBusInterface network("net.connman.iwd", path,
                           "org.freedesktop.DBus.Properties", bus);

    QDBusReply<QVariant> name =
        network.call("Get", "net.connman.iwd.Network", "Name");
    QDBusReply<QVariant> connected =
        network.call("Get", "net.connman.iwd.Network", "Connected");
    entries.append(
        {name.value().toString(), path, signal, connected.value().toBool()});

    qDebug() << "\nSSID:" << name.value().toString() << "\nSignal: " << signal
             << "\nConnected: " << connected.value().toBool()
             << "\nPath: " << path;
  }
  m_model.setNetworks(entries);
  emit networksChanged();
}

void Connecttebayo::connectNetwork(const QString &networkPath) {
  QDBusConnection bus = QDBusConnection::systemBus();
  qDebug() << "Attempt: " << networkPath;
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
  if (m_stationPath.isEmpty()) {
    return;
  }
  QDBusConnection bus = QDBusConnection::systemBus();
  if (!bus.isConnected()) {
    qWarning() << "Can't connect to system bus";
    return;
  }
  QDBusInterface station("net.connman.iwd", m_stationPath,
                         "org.freedesktop.DBus.Properties", bus);

  QDBusReply<QVariant> state =
      station.call("Get", "net.connman.iwd.Station", "State");

  if (state.isValid()) {
    m_state = state.value().toString();
    emit stateChanged();
    qDebug() << "Station state: " << state.value().toString();
  } else {
    m_connectedSsid = "";
    emit stateChanged();
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
      m_connectedSsid = name.value().toString();
      emit stateChanged();
      qDebug() << "Connected SSID: " << name.value().toString();
    }
  }
}

void Connecttebayo::disconnect() {
  if (m_stationPath.isEmpty()) {
    return;
  }
  QDBusConnection bus = QDBusConnection::systemBus();
  QDBusInterface station{"net.connman.iwd", m_stationPath,
                         "net.connman.iwd.Station", bus};

  QDBusReply<void> reply = station.call("Disconnect");
  if (!reply.isValid()) {
    qWarning() << "Disconnect failure: " << reply.error().message();
  } else {
    qDebug() << "Disconnected!";
  }
}

// dynamic state detection
QString Connecttebayo::getStationPath() {
  QDBusConnection bus = QDBusConnection::systemBus();

  QDBusInterface manager("net.connman.iwd", "/",
                         "org.freedesktop.DBus.ObjectManager", bus);

  QDBusMessage reply = manager.call("GetManagedObjects");

  if (reply.type() == QDBusMessage::ErrorMessage ||
      reply.arguments().isEmpty()) {
    return "";
  }

  const QDBusArgument &arg = reply.arguments().at(0).value<QDBusArgument>();
  if (arg.currentType() != QDBusArgument::MapType) {
    return "";
  }

  QString foundPath;

  arg.beginMap();
  while (!arg.atEnd()) {
    arg.beginMapEntry();
    QDBusObjectPath path;
    arg >> path;

    arg.beginMap();
    while (!arg.atEnd()) {
      arg.beginMapEntry();
      QString interfaceName;
      arg >> interfaceName;

      QVariant properties;
      arg >> properties;

      if (interfaceName == "net.connman.iwd.Station" && foundPath.isEmpty()) {
        foundPath = path.path();
      }
      arg.endMapEntry();
    }
    arg.endMap();
    arg.endMapEntry();
  }
  arg.endMap();
  return foundPath;
}
void Connecttebayo::updateStationPath() {

  QString newPath = getStationPath();
  if (newPath.isEmpty() || newPath == m_stationPath) {
    return;
  }

  QDBusConnection bus = QDBusConnection::systemBus();

  if (!m_stationPath.isEmpty()) {
    bus.disconnect(
        "net.connman.iwd", m_stationPath, "org.freedesktop.DBus.Properties",
        "PropertiesChanged", this,
        SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));
  }

  m_stationPath = newPath;

  bus.connect("net.connman.iwd", m_stationPath,
              "org.freedesktop.DBus.Properties", "PropertiesChanged", this,
              SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));

  QDBusInterface agentManager("net.connman.iwd", "/net/connman/iwd",
                              "net.connman.iwd.AgentManager", bus);
  QDBusReply<void> agent = agentManager.call(
      "RegisterAgent", QVariant::fromValue(QDBusObjectPath(
                           "/com/ryuzinoh/connecttebayo/agent")));
  if (!agent.isValid()) {
    qWarning() << "Agent registration failure: " << agent.error().message();
  } else {
    qDebug() << "IWD agent registered";
  }
}

void Connecttebayo::setPromptingPath(const QString &path) {
  if (m_promptingPath != path) {
    m_promptingPath = path;
    emit promptingPathChanged();
  }
}
