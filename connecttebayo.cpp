#include "./connecttebayo.hpp"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

Connecttebayo::Connecttebayo(QObject *parent) : QObject(parent) {

  // agent register
  m_agent = new IwdAgent(&m_agentObject);
  QDBusConnection::systemBus().registerObject(
      "/com/ryuzinoh/connecttebayo/agent", &m_agentObject);

  connect(m_agent, &IwdAgent::passphraseRequested, this,
          &Connecttebayo::setPromptingPath);
  updateStationPath();
}

Connecttebayo::~Connecttebayo() {
  QDBusConnection::systemBus().unregisterObject(
      "/com/ryuzinoh/connecttebayo/agent");
  qDebug() << "agent unregistered...";
}

void Connecttebayo::onPropertiesChanged(const QString &interface,
                                        const QVariantMap &changed,
                                        const QStringList &invalidated) {
  Q_UNUSED(invalidated)
  qDebug() << "PropertiesChanged on: " << interface << changed.keys();

  if (interface == "net.connman.iwd.Station" && changed.contains("State")) {
    m_state = changed["State"].toString();
    emit stateChanged();

    // if (m_state == "disconnected") {
    //   setConnectedSsid("");
    // }

    if (m_state == "connected") {
      setPromptingPath("");
      setErrorMessage("");
    }
    if (m_state == "connected" || m_state == "disconnected") {
      fetchNetworks(false);
    }
  }
}
// scanning networks
void Connecttebayo::fetchNetworks(bool triggerTheScan) {
  // updateStationPath();  // in case iwd somehow changes in runtime
  if (m_stationPath.isEmpty()) {
    return;
  }
  QDBusConnection bus = QDBusConnection::systemBus();
  QDBusInterface station("net.connman.iwd", m_stationPath,
                         "net.connman.iwd.Station", bus);

  if (triggerTheScan) {
    station.call("Scan");
  }

  // ordered network viw busctl
  QDBusMessage reply = station.call("GetOrderedNetworks");

  if (reply.type() == QDBusMessage::ErrorMessage ||
      reply.arguments().isEmpty()) {
    return;
  }

  /*
  parsing the array returned by GetOrderedNetworks instead of token
  guessing/stripping
  */
  const QDBusArgument &arg = reply.arguments().at(0).value<QDBusArgument>();
  QList<NetworkEntry> entries;
  QString currentSsid = "";

  arg.beginArray();
  while (!arg.atEnd()) {
    arg.beginStructure();
    QDBusObjectPath netPath;
    qint16 signal;
    arg >> netPath >> signal;
    arg.endStructure();
    QDBusInterface network("net.connman.iwd", netPath.path(),
                           "org.freedesktop.DBus.Properties", bus);
    // QDBusReply<QVariant> name =
    //     network.call("Get", "net.connman.iwd.Network", "Name");
    // QDBusReply<QVariant> connected =
    //     network.call("Get", "net.connman.iwd.Network", "Connected");
    //
    // QString ssid = name.isValid() ? name.value().toString() : "Unknown";
    // bool isConnected = connected.isValid() ? connected.value().toBool() :
    // false;

    QDBusReply<QVariantMap> all =
        network.call("GetAll", "net.connman.iwd.Network");
    if (!all.isValid()) {
      qWarning() << "getting all failed: " << netPath.path()
                 << all.error().message();
      continue;
    }
    QVariantMap props = all.value();
    qDebug() << "Network:" << props["Name"].toString()
             << "Connected:" << props["Connected"].toBool()
             << "Signal:" << signal;
    QString ssid = props["Name"].toString();
    bool isConnected = props["Connected"].toBool();
    if (isConnected) {
      currentSsid = ssid;
    }
    entries.append({ssid, netPath.path(), signal, isConnected});
    /*
    qDebug() << "\nSSID:" << ssid << "\nSignal: " << signal
             << "\nConnected: " << isConnected << "\nPath: " << netPath.path();
    */
  }
  arg.endArray();
  m_model.setNetworks(entries);
  setConnectedSsid(currentSsid);
  emit networksChanged();
}

void Connecttebayo::connectNetwork(const QString &networkPath) {
  setErrorMessage("");
  if (m_connecting) {
    return;
  }
  m_connecting = true;

  QDBusConnection bus = QDBusConnection::systemBus();
  qDebug() << "Attempt: " << networkPath;
  QDBusInterface *network = new QDBusInterface(
      "net.connman.iwd", networkPath, "net.connman.iwd.Network", bus, this);

  QDBusPendingCall call = network->asyncCall("Connect");
  QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);

  connect(watcher, &QDBusPendingCallWatcher::finished, this,
          [this, networkPath, network](QDBusPendingCallWatcher *watcher) {
            QDBusPendingReply<> reply = *watcher;
            if (reply.isError()) {
              qWarning() << "Connect failure: " << reply.error().message();
              setErrorMessage(reply.error().message());

              QDBusConnection bus = QDBusConnection::systemBus();
              QDBusInterface props("net.connman.iwd", networkPath,
                                   "org.freedesktop.DBus.Properties", bus);
              QDBusReply<QVariant> knownReply =
                  props.call("Get", "net.connman.iwd.Network", "KnownNetwork");

              if (knownReply.isValid()) {
                QDBusObjectPath path =
                    knownReply.value().value<QDBusObjectPath>();
                if (path.path() != "/" && !path.path().isEmpty()) {
                  QDBusInterface knownNet("net.connman.iwd", path.path(),
                                          "net.connman.iwd.KnownNetwork", bus);
                  knownNet.call("Forget");
                }
              }
              setPromptingPath("");
            } else {
              qDebug() << "Connected to: " << networkPath;
            }
            watcher->deleteLater();
            network->deleteLater();
            m_connecting = false;
          });
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
void Connecttebayo::setConnectedSsid(const QString &ssid) {
  if (m_connectedSsid != ssid) {
    m_connectedSsid = ssid;
    emit connectedSsidChanged();
  }
}

void Connecttebayo::setErrorMessage(const QString &error) {
  if (m_errorMessage != error) {
    m_errorMessage = error;
    emit errorMessageChanged();
  }
}

void Connecttebayo::providePassphrase(const QString &passphrase) {
  m_agent->sendPassphrase(passphrase);
}

void Connecttebayo::cancelPassphrase() {
  m_agent->cancelPassphrase();
  setPromptingPath("");
  setErrorMessage("");
}
