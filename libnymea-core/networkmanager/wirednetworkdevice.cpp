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

/*!
    \class nymeaserver::WiredNetworkDevice
    \brief Represents an ethernet device in the \l{NetworkManager}.

    \ingroup networkmanager
    \inmodule core
*/

/*! \fn void WiredNetworkDevice::propertiesChanged(const QVariantMap &properties);
    This signal will be emitted when the \a properties of this \l{WiredNetworkDevice} have changed.
*/

#include "wirednetworkdevice.h"
#include "loggingcategories.h"

namespace nymeaserver {

/*! Constructs a new \l{WiredNetworkDevice} with the given dbus \a objectPath and \a parent. */
WiredNetworkDevice::WiredNetworkDevice(const QDBusObjectPath &objectPath, QObject *parent) :
    NetworkDevice(objectPath, parent)
{
    QDBusConnection systemBus = QDBusConnection::systemBus();
    if (!systemBus.isConnected()) {
        qCWarning(dcNetworkManager()) << "WiredNetworkDevice: System DBus not connected";
        return;
    }

    m_wiredInterface = new QDBusInterface(serviceString, this->objectPath().path(), wiredInterfaceString, systemBus, this);
    if(!m_wiredInterface->isValid()) {
        qCWarning(dcNetworkManager()) << "WiredNetworkDevice: Invalid wired dbus interface";
        return;
    }

    setMacAddress(m_wiredInterface->property("HwAddress").toString());
    setBitRate(m_wiredInterface->property("Bitrate").toInt());
    setPluggedIn(m_wiredInterface->property("Carrier").toBool());

    QDBusConnection::systemBus().connect(serviceString, this->objectPath().path(), wiredInterfaceString, "PropertiesChanged", this, SLOT(propertiesChanged(QVariantMap)));
}

/*! Returns the mac address of this \l{WiredNetworkDevice}. */
QString WiredNetworkDevice::macAddress() const
{
    return m_macAddress;
}

/*! Returns the current bit rate [Mb/s] of this \l{WiredNetworkDevice}. */
int WiredNetworkDevice::bitRate() const
{
    return m_bitRate;
}

/*! Returns true if this \l{WiredNetworkDevice} has a cable plugged in. */
bool WiredNetworkDevice::pluggedIn() const
{
    return m_pluggedIn;
}

void WiredNetworkDevice::setMacAddress(const QString &macAddress)
{
    m_macAddress = macAddress;
}

void WiredNetworkDevice::setBitRate(const int &bitRate)
{
    m_bitRate = bitRate;
}

void WiredNetworkDevice::setPluggedIn(const bool &pluggedIn)
{
    m_pluggedIn = pluggedIn;
}

void WiredNetworkDevice::propertiesChanged(const QVariantMap &properties)
{
    if (properties.contains("Carrier"))
        setPluggedIn(properties.value("Carrier").toBool());

}

QDebug operator<<(QDebug debug, WiredNetworkDevice *networkDevice)
{
    debug.nospace() << "WiredNetworkDevice(" << networkDevice->interface() << ", ";
    debug.nospace() << networkDevice->macAddress() <<  ", ";
    debug.nospace() << networkDevice->bitRate() <<  " [Mb/s], ";
    debug.nospace() << networkDevice->pluggedIn() <<  ", ";
    debug.nospace() << networkDevice->deviceStateString() <<  ") ";
    return debug;
}

}
