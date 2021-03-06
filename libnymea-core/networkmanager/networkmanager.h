/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU General Public License as published by the Free Software
* Foundation, GNU version 3. This project is distributed in the hope that it
* will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusContext>
#include <QDBusArgument>

#include "dbus-interfaces.h"
#include "wirednetworkdevice.h"
#include "wirelessnetworkdevice.h"
#include "networksettings.h"

// Docs: https://developer.gnome.org/NetworkManager/unstable/spec.html

namespace nymeaserver {

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    enum NetworkManagerState {
        NetworkManagerStateUnknown = 0,
        NetworkManagerStateAsleep = 10,
        NetworkManagerStateDisconnected = 20,
        NetworkManagerStateDisconnecting = 30,
        NetworkManagerStateConnecting = 40,
        NetworkManagerStateConnectedLocal = 50,
        NetworkManagerStateConnectedSite = 60,
        NetworkManagerStateConnectedGlobal = 70
    };
    Q_ENUM(NetworkManagerState)

    enum NetworkManagerConnectivityState {
        NetworkManagerConnectivityStateUnknown = 0,
        NetworkManagerConnectivityStateNone = 1,
        NetworkManagerConnectivityStatePortal = 2,
        NetworkManagerConnectivityStateLimited = 3,
        NetworkManagerConnectivityStateFull = 4
    };
    Q_ENUM(NetworkManagerConnectivityState)

    enum NetworkManagerError {
        NetworkManagerErrorNoError,
        NetworkManagerErrorUnknownError,
        NetworkManagerErrorWirelessNotAvailable,
        NetworkManagerErrorAccessPointNotFound,
        NetworkManagerErrorNetworkInterfaceNotFound,
        NetworkManagerErrorInvalidNetworkDeviceType,
        NetworkManagerErrorWirelessNetworkingDisabled,
        NetworkManagerErrorWirelessConnectionFailed,
        NetworkManagerErrorNetworkingDisabled,
        NetworkManagerErrorNetworkManagerNotAvailable
    };
    Q_ENUM(NetworkManagerError)

    explicit NetworkManager(QObject *parent = nullptr);

    bool available();
    bool wifiAvailable();

    QList<NetworkDevice *> networkDevices() const;
    QList<WirelessNetworkDevice *> wirelessNetworkDevices() const;
    QList<WiredNetworkDevice *> wiredNetworkDevices() const;

    NetworkDevice *getNetworkDevice(const QString &interface);

    // Properties
    QString version() const;
    NetworkManagerState state() const;
    QString stateString() const;
    NetworkManagerConnectivityState connectivityState() const;

    NetworkManagerError connectWifi(const QString &interface, const QString &ssid, const QString &password);

    // Networking
    bool networkingEnabled() const;
    bool enableNetworking(const bool &enabled);

    // Wireless Networking
    bool wirelessEnabled() const;
    bool enableWireless(const bool &enabled);

private:
    QDBusInterface *m_networkManagerInterface;

    QHash<QDBusObjectPath, NetworkDevice *> m_networkDevices;
    QHash<QDBusObjectPath, WirelessNetworkDevice *> m_wirelessNetworkDevices;
    QHash<QDBusObjectPath, WiredNetworkDevice *> m_wiredNetworkDevices;

    NetworkSettings *m_networkSettings;

    bool m_available;
    bool m_wifiAvailable;

    QString m_version;

    NetworkManagerState m_state;
    NetworkManagerConnectivityState m_connectivityState;
    bool m_networkingEnabled;
    bool m_wirelessEnabled;

    void loadDevices();

    static QString networkManagerStateToString(const NetworkManagerState &state);
    static QString networkManagerConnectivityStateToString(const NetworkManagerConnectivityState &state);

    void setVersion(const QString &version);
    void setNetworkingEnabled(const bool &enabled);
    void setWirelessEnabled(const bool &enabled);
    void setConnectivityState(const NetworkManagerConnectivityState &connectivityState);
    void setState(const NetworkManagerState &state);

signals:
    void versionChanged();
    void networkingEnabledChanged();
    void wirelessEnabledChanged();
    void stateChanged();
    void connectivityStateChanged();

    void wirelessDeviceAdded(WirelessNetworkDevice *wirelessDevice);
    void wirelessDeviceRemoved(const QString &interface);
    void wirelessDeviceChanged(WirelessNetworkDevice *wirelessDevice);

    void wiredDeviceAdded(WiredNetworkDevice *wiredDevice);
    void wiredDeviceRemoved(const QString &interface);
    void wiredDeviceChanged(WiredNetworkDevice *wiredDevice);

private slots:
    void onDeviceAdded(const QDBusObjectPath &deviceObjectPath);
    void onDeviceRemoved(const QDBusObjectPath &deviceObjectPath);
    void onPropertiesChanged(const QVariantMap &properties);

    void onWirelessDeviceChanged();
    void onWiredDeviceChanged();
};

}

Q_DECLARE_METATYPE(nymeaserver::NetworkManager::NetworkManagerState)
Q_DECLARE_METATYPE(nymeaserver::NetworkManager::NetworkManagerError)

#endif // NETWORKMANAGER_H
