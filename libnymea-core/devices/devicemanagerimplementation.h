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

#ifndef DEVICEMANAGERIMPLEMENTATION_H
#define DEVICEMANAGERIMPLEMENTATION_H

#include "libnymea.h"

#include "devices/device.h"
#include "devices/devicedescriptor.h"
#include "devices/pluginmetadata.h"

#include "types/deviceclass.h"
#include "types/interface.h"
#include "types/event.h"
#include "types/action.h"
#include "types/vendor.h"

#include <QObject>
#include <QTimer>
#include <QLocale>
#include <QPluginLoader>
#include <QTranslator>

#include "hardwaremanager.h"

#include "devices/devicemanager.h"

class Device;
class DevicePlugin;
class DevicePairingInfo;
class HardwareManager;
class Translator;

class DeviceManagerImplementation: public DeviceManager
{
    Q_OBJECT

    friend class DevicePlugin;

public:
    explicit DeviceManagerImplementation(HardwareManager *hardwareManager, const QLocale &locale, QObject *parent = nullptr);
    ~DeviceManagerImplementation() override;

    static QStringList pluginSearchDirs();
    static QList<QJsonObject> pluginsMetadata();
    void registerStaticPlugin(DevicePlugin* plugin, const PluginMetadata &metaData);

    DevicePlugins plugins() const override;
    DevicePlugin *plugin(const PluginId &pluginId) const override;
    Device::DeviceError setPluginConfig(const PluginId &pluginId, const ParamList &pluginConfig) override;

    Vendors supportedVendors() const override;
    Interfaces supportedInterfaces() const override;
    DeviceClasses supportedDevices(const VendorId &vendorId = VendorId()) const override;

    Devices configuredDevices() const override;
    Device* findConfiguredDevice(const DeviceId &id) const override;
    Devices findConfiguredDevices(const DeviceClassId &deviceClassId) const override;
    Devices findConfiguredDevices(const QString &interface) const override;
    Devices findChildDevices(const DeviceId &id) const override;
    DeviceClass findDeviceClass(const DeviceClassId &deviceClassId) const override;

    DeviceDiscoveryInfo* discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;

    DeviceSetupInfo* addConfiguredDevice(const DeviceClassId &deviceClassId, const ParamList &params, const QString &name = QString()) override;
    DeviceSetupInfo* addConfiguredDevice(const DeviceDescriptorId &deviceDescriptorId, const ParamList &params = ParamList(), const QString &name = QString()) override;

    DeviceSetupInfo* reconfigureDevice(const DeviceId &deviceId, const ParamList &params, const QString &name = QString()) override;
    DeviceSetupInfo* reconfigureDevice(const DeviceDescriptorId &deviceDescriptorId, const ParamList &params = ParamList(), const QString &name = QString()) override;

    DevicePairingInfo* pairDevice(const DeviceClassId &deviceClassId, const ParamList &params, const QString &name = QString()) override;
    DevicePairingInfo* pairDevice(const DeviceDescriptorId &deviceDescriptorId, const ParamList &params = ParamList(), const QString &name = QString()) override;
    DevicePairingInfo* pairDevice(const DeviceId &deviceId, const ParamList &params, const QString &name = QString()) override;
    DevicePairingInfo* confirmPairing(const PairingTransactionId &pairingTransactionId, const QString &username, const QString &secret) override;

    Device::DeviceError editDevice(const DeviceId &deviceId, const QString &name) override;
    Device::DeviceError setDeviceSettings(const DeviceId &deviceId, const ParamList &settings) override;

    Device::DeviceError removeConfiguredDevice(const DeviceId &deviceId) override;

    DeviceActionInfo* executeAction(const Action &action) override;

    BrowseResult* browseDevice(const DeviceId &deviceId, const QString &itemId, const QLocale &locale) override;
    BrowserItemResult* browserItemDetails(const DeviceId &deviceId, const QString &itemId, const QLocale &locale) override;
    BrowserActionInfo *executeBrowserItem(const BrowserAction &browserAction) override;
    BrowserItemActionInfo *executeBrowserItemAction(const BrowserItemAction &browserItemAction) override;

    QString translate(const PluginId &pluginId, const QString &string, const QLocale &locale) override;
    ParamType translateParamType(const PluginId &pluginId, const ParamType &paramType, const QLocale &locale) override;
    DeviceClass translateDeviceClass(const DeviceClass &deviceClass, const QLocale &locale) override;
    Vendor translateVendor(const Vendor &vendor, const QLocale &locale) override;

signals:
    void loaded();

private slots:
    void loadPlugins();
    void loadPlugin(DevicePlugin *pluginIface, const PluginMetadata &metaData);
    void loadConfiguredDevices();
    void storeConfiguredDevices();
    void startMonitoringAutoDevices();
    void onAutoDevicesAppeared(const DeviceDescriptors &deviceDescriptors);
    void onAutoDeviceDisappeared(const DeviceId &deviceId);
    void onLoaded();
    void cleanupDeviceStateCache();
    void onEventTriggered(const Event &event);

    // Only connect this to Devices. It will query the sender()
    void slotDeviceStateValueChanged(const StateTypeId &stateTypeId, const QVariant &value);
    void slotDeviceSettingChanged(const ParamTypeId &paramTypeId, const QVariant &value);
    void slotDeviceNameChanged();

private:
    // Builds a list of params ready to create a device.
    // Template is deviceClass.paramtypes, "first" has highest priority. If a param is not found neither in first nor in second, defaults apply.
    ParamList buildParams(const ParamTypes &types, const ParamList &first, const ParamList &second = ParamList());
    void pairDeviceInternal(DevicePairingInfo *info);
    DeviceSetupInfo *addConfiguredDeviceInternal(const DeviceClassId &deviceClassId, const QString &name, const ParamList &params, const DeviceId &parentDeviceId = DeviceId());
    DeviceSetupInfo *reconfigureDeviceInternal(Device *device, const ParamList &params, const QString &name = QString());
    DeviceSetupInfo *setupDevice(Device *device);
    void postSetupDevice(Device *device);
    void storeDeviceStates(Device *device);
    void loadDeviceStates(Device *device);

private:
    HardwareManager *m_hardwareManager;

    QLocale m_locale;
    Translator *m_translator = nullptr;
    QHash<VendorId, Vendor> m_supportedVendors;
    QHash<QString, Interface> m_supportedInterfaces;
    QHash<VendorId, QList<DeviceClassId> > m_vendorDeviceMap;
    QHash<DeviceClassId, DeviceClass> m_supportedDevices;
    QHash<DeviceId, Device*> m_configuredDevices;
    QHash<DeviceDescriptorId, DeviceDescriptor> m_discoveredDevices;

    QHash<PluginId, DevicePlugin*> m_devicePlugins;

    class PairingContext {
    public:
        DeviceId deviceId;
        DeviceClassId deviceClassId;
        DeviceId parentDeviceId;
        ParamList params;
        QString deviceName;
    };
    QHash<PairingTransactionId, PairingContext> m_pendingPairings;
};

#endif // DEVICEMANAGERIMPLEMENTATION_H
