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

#include "devicemanagerimplementation.h"
#include "translator.h"
#if QT_VERSION >= QT_VERSION_CHECK(5,12,0)
#include "scriptdeviceplugin.h"
#endif

#include "loggingcategories.h"
#include "typeutils.h"
#include "nymeasettings.h"
#include "version.h"
#include "plugininfocache.h"

#include "devices/devicediscoveryinfo.h"
#include "devices/devicepairinginfo.h"
#include "devices/devicesetupinfo.h"
#include "devices/deviceactioninfo.h"
#include "devices/deviceplugin.h"
#include "devices/deviceutils.h"
#include "devices/browseresult.h"
#include "devices/browseritemresult.h"
#include "devices/browseractioninfo.h"
#include "devices/browseritemactioninfo.h"

//#include "unistd.h"

#include "plugintimer.h"

#include <QPluginLoader>
#include <QStaticPlugin>
#include <QtPlugin>
#include <QDebug>
#include <QStringList>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>

/*! Constructs the DeviceManager with the given \a{hardwareManager}, \a locale and \a parent. There should only be one DeviceManager in the system created by \l{nymeaserver::NymeaCore}.
 *  Use \c nymeaserver::NymeaCore::instance()->deviceManager() instead to access the DeviceManager. */
DeviceManagerImplementation::DeviceManagerImplementation(HardwareManager *hardwareManager, const QLocale &locale, QObject *parent) :
    DeviceManager(parent),
    m_hardwareManager(hardwareManager),
    m_locale(locale),
    m_translator(new Translator(this))
{
    qRegisterMetaType<DeviceClassId>();
    qRegisterMetaType<DeviceDescriptor>();

    foreach (const Interface &interface, DeviceUtils::allInterfaces()) {
        m_supportedInterfaces.insert(interface.name(), interface);
    }

    // Give hardware a chance to start up before loading plugins etc.
    QMetaObject::invokeMethod(this, "loadPlugins", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "loadConfiguredDevices", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "startMonitoringAutoDevices", Qt::QueuedConnection);

    // Make sure this is always emitted after plugins and devices are loaded
    QMetaObject::invokeMethod(this, "onLoaded", Qt::QueuedConnection);
}

/*! Destructor of the DeviceManagerImplementation. Each loaded \l{DevicePlugin} will be deleted. */
DeviceManagerImplementation::~DeviceManagerImplementation()
{
    delete m_translator;

    foreach (Device *device, m_configuredDevices) {
        storeDeviceStates(device);
        delete device;
    }

    foreach (DevicePlugin *plugin, m_devicePlugins) {
        if (plugin->parent() == this) {
            qCDebug(dcDeviceManager()) << "Deleting plugin" << plugin->pluginName();
            delete plugin;
        } else {
            qCDebug(dcDeviceManager()) << "Not deleting plugin" << plugin->pluginName();
        }
    }
}

/*! Returns the list of search direcorys where \l{DevicePlugin} will be searched. */
QStringList DeviceManagerImplementation::pluginSearchDirs()
{
    QStringList searchDirs;
    QByteArray envPath = qgetenv("NYMEA_PLUGINS_PATH");
    if (!envPath.isEmpty()) {
        searchDirs << QString(envPath).split(':');
    }

    foreach (QString libraryPath, QCoreApplication::libraryPaths()) {
        searchDirs << libraryPath.replace("qt5", "nymea");
    }
    foreach (QString libraryPath, QCoreApplication::libraryPaths()) {
        searchDirs << libraryPath.replace("plugins", "nymea/plugins");
    }
    searchDirs << QDir(QCoreApplication::applicationDirPath() + "/../lib/nymea/plugins/").absolutePath();
    searchDirs << QDir(QCoreApplication::applicationDirPath() + "/../plugins/").absolutePath();
    searchDirs << QDir(QCoreApplication::applicationDirPath() + "/../../../plugins/").absolutePath();
    searchDirs.removeDuplicates();
    return searchDirs;
}

/*! Returns the list of json objects containing the metadata of the installed plugins. */
QList<QJsonObject> DeviceManagerImplementation::pluginsMetadata()
{
    QList<QJsonObject> pluginList;
    foreach (const QString &path, pluginSearchDirs()) {
        QDir dir(path);
        foreach (const QString &entry, dir.entryList()) {
            QFileInfo fi;
            if (entry.startsWith("libnymea_deviceplugin") && entry.endsWith(".so")) {
                fi.setFile(path + "/" + entry);
            } else {
                fi.setFile(path + "/" + entry + "/libnymea_deviceplugin" + entry + ".so");
            }
            if (!fi.exists()) {
                continue;
            }
            QPluginLoader loader(fi.absoluteFilePath());
            pluginList.append(loader.metaData().value("MetaData").toObject());
        }
    }
    return pluginList;
}

/*! Register a DevicePlugin class. This can be used to create devices internally from the guh system without having to create a full plugin.
    The \a metaData contains the static plugin configurations. The DeviceManagerImplementation takes ownership of the object \a plugin and will clean it up when exiting. Do not delete the object yourself. */
void DeviceManagerImplementation::registerStaticPlugin(DevicePlugin *plugin, const PluginMetadata &metaData)
{
    if (!metaData.isValid()) {
        qCWarning(dcDeviceManager()) << "Plugin metadata not valid. Not loading static plugin:" << plugin->pluginName();
        return;
    }
    loadPlugin(plugin, metaData);
}

/*! Returns all the \l{DevicePlugin}{DevicePlugins} loaded in the system. */
DevicePlugins DeviceManagerImplementation::plugins() const
{
    return m_devicePlugins.values();
}

DevicePlugin *DeviceManagerImplementation::plugin(const PluginId &pluginId) const
{
    return m_devicePlugins.value(pluginId);
}

/*! Returns a certain \l{DeviceError} and sets the configuration of the plugin with the given \a pluginId
 *  and the given \a pluginConfig. */
Device::DeviceError DeviceManagerImplementation::setPluginConfig(const PluginId &pluginId, const ParamList &pluginConfig)
{
    DevicePlugin *plugin = m_devicePlugins.value(pluginId);
    if (!plugin) {
        qCWarning(dcDeviceManager()) << "Could not set plugin configuration. There is no plugin with id" << pluginId.toString();
        return Device::DeviceErrorPluginNotFound;
    }

    ParamList params = buildParams(plugin->configurationDescription(), pluginConfig);
    Device::DeviceError verify = DeviceUtils::verifyParams(plugin->configurationDescription(), params);
    if (verify != Device::DeviceErrorNoError)
        return verify;

    Device::DeviceError result = plugin->setConfiguration(params);
    if (result != Device::DeviceErrorNoError)
        return result;

    NymeaSettings settings(NymeaSettings::SettingsRolePlugins);
    settings.beginGroup("PluginConfig");
    settings.beginGroup(plugin->pluginId().toString());

    foreach (const Param &param, pluginConfig) {
        settings.beginGroup(param.paramTypeId().toString());
        settings.setValue("type", static_cast<int>(param.value().type()));
        settings.setValue("value", param.value());
        settings.endGroup();
    }

    settings.endGroup();
    settings.endGroup();
    emit pluginConfigChanged(plugin->pluginId(), pluginConfig);
    return result;
}

/*! Returns all the \l{Vendor}s loaded in the system. */
Vendors DeviceManagerImplementation::supportedVendors() const
{
    return m_supportedVendors.values();
}

/*! Returns the list of all supported \l{Interfaces for DeviceClasses}{interfaces}. */
Interfaces DeviceManagerImplementation::supportedInterfaces() const
{
    return m_supportedInterfaces.values();
}

/*! Returns all the supported \l{DeviceClass}{DeviceClasses} by all \l{DevicePlugin}{DevicePlugins} loaded in the system.
 *  Optionally filtered by \a vendorId. */
DeviceClasses DeviceManagerImplementation::supportedDevices(const VendorId &vendorId) const
{
    if (vendorId.isNull()) {
        return m_supportedDevices.values();
    }
    QList<DeviceClass> ret;
    foreach (const DeviceClass &deviceClass, m_supportedDevices) {
        if (!vendorId.isNull() && deviceClass.vendorId() != vendorId) {
            continue;
        }
        ret.append(deviceClass);
    }
    return ret;
}
/*! Returns a certain \l{DeviceError} and starts the discovering process of the \l{Device} with the given \a deviceClassId
 *  and the given \a params.*/
DeviceDiscoveryInfo* DeviceManagerImplementation::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    DeviceClass deviceClass = findDeviceClass(deviceClassId);
    if (!deviceClass.isValid()) {
        qCWarning(dcDeviceManager) << "Device discovery failed. Invalid device class id:" << deviceClassId.toString();
        DeviceDiscoveryInfo *discoveryInfo = new DeviceDiscoveryInfo(deviceClassId, params, this);
        discoveryInfo->finish(Device::DeviceErrorDeviceClassNotFound);
        return discoveryInfo;
    }
    if (!deviceClass.createMethods().testFlag(DeviceClass::CreateMethodDiscovery)) {
        qCWarning(dcDeviceManager) << "Device discovery failed. Device class" << deviceClass.name() << "cannot be discovered.";
        DeviceDiscoveryInfo *discoveryInfo = new DeviceDiscoveryInfo(deviceClassId, params, this);
        discoveryInfo->finish(Device::DeviceErrorCreationMethodNotSupported);
        return discoveryInfo;
    }
    DevicePlugin *plugin = m_devicePlugins.value(deviceClass.pluginId());
    if (!plugin) {
        qCWarning(dcDeviceManager) << "Device discovery failed. Plugin not found for device class" << deviceClass.name();
        DeviceDiscoveryInfo *discoveryInfo = new DeviceDiscoveryInfo(deviceClassId, params, this);
        discoveryInfo->finish(Device::DeviceErrorPluginNotFound, tr("The plugin for this device or service is not loaded."));
        return discoveryInfo;
    }

    ParamList effectiveParams = buildParams(deviceClass.discoveryParamTypes(), params);
    Device::DeviceError result = DeviceUtils::verifyParams(deviceClass.discoveryParamTypes(), effectiveParams);
    if (result != Device::DeviceErrorNoError) {
        qCWarning(dcDeviceManager) << "Device discovery failed. Parameter verification failed.";
        DeviceDiscoveryInfo *discoveryInfo = new DeviceDiscoveryInfo(deviceClassId, params, this);
        discoveryInfo->finish(result);
        return discoveryInfo;
    }

    DeviceDiscoveryInfo *discoveryInfo = new DeviceDiscoveryInfo(deviceClassId, effectiveParams, this, 30000);
    connect(discoveryInfo, &DeviceDiscoveryInfo::finished, this, [this, discoveryInfo](){
        if (discoveryInfo->status() != Device::DeviceErrorNoError) {
            qCWarning(dcDeviceManager()) << "Discovery failed:" << discoveryInfo->status() << discoveryInfo->displayMessage();
            return;
        }
        qCDebug(dcDeviceManager()) << "Discovery finished. Found devices:" << discoveryInfo->deviceDescriptors().count();
        foreach (const DeviceDescriptor &descriptor, discoveryInfo->deviceDescriptors()) {
            if (!descriptor.isValid()) {
                qCWarning(dcDeviceManager()) << "Descriptor is invalid. Not adding to results";
                continue;
            }
            m_discoveredDevices.insert(descriptor.id(), descriptor);
        }
    });

    qCDebug(dcDeviceManager) << "Device discovery for" << deviceClass.name() << "started...";
    plugin->discoverDevices(discoveryInfo);
    return discoveryInfo;
}

/*! Add a new configured device for the given \l{DeviceClass}, the given parameters, \a name and \a id.
 *  \a deviceClassId must refer to an existing \{DeviceClass} and \a params must match the parameter description in the \l{DeviceClass}.
 *  Optionally you can supply an id yourself if you must keep track of the added device. If you don't supply it, a new one will
 *  be generated. Only devices with \l{DeviceClass}{CreateMethodUser} can be created using this method.
 *  Returns \l{DeviceError} to inform about the result. */
DeviceSetupInfo* DeviceManagerImplementation::addConfiguredDevice(const DeviceClassId &deviceClassId, const ParamList &params, const QString &name)
{
    return addConfiguredDeviceInternal(deviceClassId, name, params);
}

/*! Add a new configured device for the given \l{DeviceClass} the given DeviceDescriptorId and \a deviceId. Only devices with \l{DeviceClass}{CreateMethodDiscovery}
 *  can be created using this method. The \a deviceClassId must refer to an existing \l{DeviceClass} and the \a deviceDescriptorId must refer to an existing DeviceDescriptorId
 *  from the discovery. The \a name parameter should contain the device name. Optionally device params can be passed. By default the descriptor's params as found by the discovery
 *  are used but can be overridden here.
 *
 *  Returns \l{DeviceError} to inform about the result. */
DeviceSetupInfo *DeviceManagerImplementation::addConfiguredDevice(const DeviceDescriptorId &deviceDescriptorId, const ParamList &params, const QString &name)
{
    DeviceDescriptor descriptor = m_discoveredDevices.value(deviceDescriptorId);
    if (!descriptor.isValid()) {
        qCWarning(dcDeviceManager()) << "Cannot add device. DeviceDescriptor" << deviceDescriptorId << "not found.";
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        info->finish(Device::DeviceErrorDeviceDescriptorNotFound);
        return info;
    }

    DeviceClass deviceClass = findDeviceClass(descriptor.deviceClassId());
    if (!deviceClass.isValid()) {
        qCWarning(dcDeviceManager()) << "Cannot add device. DeviceClass" << descriptor.deviceClassId() << "not found.";
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        info->finish(Device::DeviceErrorDeviceClassNotFound);
        return info;
    }
    if (!deviceClass.createMethods().testFlag(DeviceClass::CreateMethodDiscovery)) {
        qCWarning(dcDeviceManager()) << "Cannot add device. This device cannot be added via discovery.";
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        info->finish(Device::DeviceErrorCreationMethodNotSupported);
        return info;
    }

    // Merging params from descriptor and user provided ones
    ParamList finalParams = buildParams(deviceClass.paramTypes(), params, descriptor.params());

    return addConfiguredDeviceInternal(descriptor.deviceClassId(), name, finalParams, descriptor.parentDeviceId());
}


/*! Edit the \l{ParamList}{Params} of a configured device with the given \a deviceId to the new given \a params.
 *  The given parameter \a fromDiscovery specifies if the new \a params came
 *  from a discovery or if the user set them. If it came from discovery not writable parameters (readOnly) will be changed too.
 *
 *  Returns \l{DeviceError} to inform about the result. */
DeviceSetupInfo* DeviceManagerImplementation::reconfigureDevice(const DeviceId &deviceId, const ParamList &params, const QString &name)
{
    Device *device = findConfiguredDevice(deviceId);
    if (!device) {
        qCWarning(dcDeviceManager()) << "Cannot reconfigure device. Device with id" << deviceId.toString() << "not found.";
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        info->finish(Device::DeviceErrorDeviceNotFound);
        return info;
    }

    DeviceClass deviceClass = findDeviceClass(device->deviceClassId());
    if (deviceClass.id().isNull()) {
        qCWarning(dcDeviceManager()) << "Cannot reconfigure device. DeviceClass for device" << device->name() << deviceId.toString() << "not found.";
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        info->finish(Device::DeviceErrorDeviceClassNotFound);
        return info;
    }

    foreach (const ParamType &paramType, deviceClass.paramTypes()) {
        if (paramType.readOnly() && params.hasParam(paramType.id())) {
            DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
            qCWarning(dcDeviceManager()) << "Parameter" << paramType.name() << paramType.id() << "is not writable";
            info->finish(Device::DeviceErrorParameterNotWritable);
            return info;
        }
    }

    ParamList finalParams = buildParams(deviceClass.paramTypes(), params);

    return reconfigureDeviceInternal(device, finalParams, name);
}

/*! Edit the \l{Param}{Params} of a configured device to the \l{Param}{Params} of the DeviceDescriptor with the
 *  given \a deviceId to the given DeviceDescriptorId.
 *  Only devices with \l{DeviceClass}{CreateMethodDiscovery} can be changed using this method.
 *  The \a deviceDescriptorId must refer to an existing DeviceDescriptorId from the discovery.
 *  This method allows to rediscover a device and update it's \l{Param}{Params}.
 *
 *  Returns \l{DeviceError} to inform about the result. */
DeviceSetupInfo *DeviceManagerImplementation::reconfigureDevice(const DeviceDescriptorId &deviceDescriptorId, const ParamList &params, const QString &name)
{
    DeviceDescriptor descriptor = m_discoveredDevices.value(deviceDescriptorId);
    if (!descriptor.isValid()) {
        qCWarning(dcDeviceManager()) << "Cannot reconfigure device. No deviceDescriptor with ID" << deviceDescriptorId << "found.";
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        info->finish(Device::DeviceErrorDeviceDescriptorNotFound);
        return info;
    }

    Device *device = findConfiguredDevice(descriptor.deviceId());
    if (!device) {
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        qCWarning(dcDeviceManager()) << "Cannot reconfigure device. No device with ID" << descriptor.deviceId() << "found.";
        info->finish(Device::DeviceErrorDeviceNotFound);
        return info;
    }

    DeviceClass deviceClass = findDeviceClass(device->deviceClassId());
    if (!deviceClass.isValid()) {
        qCWarning(dcDeviceManager()) << "Cannot reconfigure device. No deviceClass with ID" << device->deviceClassId() << "found.";
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        info->finish(Device::DeviceErrorDeviceClassNotFound);
        return info;
    }

    ParamList finalParams = buildParams(device->deviceClass().paramTypes(), params, descriptor.params());
    return reconfigureDeviceInternal(device, finalParams, name);
}

DeviceSetupInfo *DeviceManagerImplementation::reconfigureDeviceInternal(Device *device, const ParamList &params, const QString &name)
{
    DevicePlugin *plugin = m_devicePlugins.value(device->deviceClass().pluginId());
    if (!plugin) {
        qCWarning(dcDeviceManager()) << "Cannot reconfigure device. Plugin for DeviceClass" << device->deviceClassId().toString() << "not found.";
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        info->finish(Device::DeviceErrorPluginNotFound);
        return info;
    }

    ParamList finalParams = buildParams(device->deviceClass().paramTypes(), params);
    Device::DeviceError result = DeviceUtils::verifyParams(device->deviceClass().paramTypes(), finalParams);
    if (result != Device::DeviceErrorNoError) {
        qCWarning(dcDeviceManager()) << "Cannot reconfigure device. Params failed validation.";
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        info->finish(result);
        return info;
    }

    // first remove the device in the plugin
    plugin->deviceRemoved(device);

    // mark setup as incomplete
    device->setSetupStatus(Device::DeviceSetupStatusInProgress, Device::DeviceErrorNoError);

    // set new params
    foreach (const Param &param, params) {
        device->setParamValue(param.paramTypeId(), param.value());
    }

    if (!name.isEmpty()) {
        device->setName(name);
    }

    // try to setup the device with the new params
    DeviceSetupInfo *info = new DeviceSetupInfo(device, this, 30000);
    plugin->setupDevice(info);
    connect(info, &DeviceSetupInfo::finished, this, [this, info](){

        if (info->status() != Device::DeviceErrorNoError) {
            qCWarning(dcDeviceManager()) << "Device reconfiguration failed for" << info->device()->name() << info->device()->id().toString() << info->status() << info->displayMessage();
            info->device()->setSetupStatus(Device::DeviceSetupStatusFailed, info->status(), info->displayMessage());
            // TODO: recover old params.??
            return;
        }

        storeConfiguredDevices();

        postSetupDevice(info->device());
        info->device()->setSetupStatus(Device::DeviceSetupStatusComplete, Device::DeviceErrorNoError);

        emit deviceChanged(info->device());

    });

    return info;

}

/*! Edit the \a name of the \l{Device} with the given \a deviceId.
    Returns \l{Device::DeviceError}{DeviceError} to inform about the result.
*/
Device::DeviceError DeviceManagerImplementation::editDevice(const DeviceId &deviceId, const QString &name)
{
    Device *device = findConfiguredDevice(deviceId);
    if (!device)
        return Device::DeviceErrorDeviceNotFound;

    device->setName(name);

    return Device::DeviceErrorNoError;
}

Device::DeviceError DeviceManagerImplementation::setDeviceSettings(const DeviceId &deviceId, const ParamList &settings)
{
    Device *device = findConfiguredDevice(deviceId);
    if (!device) {
        qCWarning(dcDeviceManager()) << "Cannot set device settings. Device" << deviceId.toString() << "not found";
        return Device::DeviceErrorDeviceNotFound;
    }
    // build a list of settings using: a) provided new settings b) previous settings and c) default values
    ParamList effectiveSettings = buildParams(device->deviceClass().settingsTypes(), settings, device->settings());
    Device::DeviceError status = DeviceUtils::verifyParams(device->deviceClass().settingsTypes(), effectiveSettings);
    if (status != Device::DeviceErrorNoError) {
        qCWarning(dcDeviceManager()) << "Error setting device settings for device" << device->name() << device->id().toString();
        return status;
    }
    device->setSettings(effectiveSettings);
    return Device::DeviceErrorNoError;
}

DevicePairingInfo* DeviceManagerImplementation::pairDevice(const DeviceClassId &deviceClassId, const ParamList &params, const QString &name)
{
    PairingTransactionId transactionId = PairingTransactionId::createPairingTransactionId();

    DeviceClass deviceClass = m_supportedDevices.value(deviceClassId);
    if (!deviceClass.isValid()) {
        qCWarning(dcDeviceManager) << "Cannot find a DeviceClass with ID" << deviceClassId.toString();
        DevicePairingInfo *info = new DevicePairingInfo(transactionId, deviceClassId, DeviceId(), name, ParamList(), DeviceId(), this);
        info->finish(Device::DeviceErrorDeviceClassNotFound);
        return info;
    }

    // Create new device id
    DeviceId newDeviceId = DeviceId::createDeviceId();

    // Use given params, if there are missing some, use the defaults ones.
    ParamList finalParams = buildParams(deviceClass.paramTypes(), params);

    DevicePairingInfo *info = new DevicePairingInfo(transactionId, deviceClassId, newDeviceId, name, finalParams, DeviceId(), this, 30000);
    pairDeviceInternal(info);
    return info;
}

DevicePairingInfo* DeviceManagerImplementation::pairDevice(const DeviceDescriptorId &deviceDescriptorId, const ParamList &params, const QString &name)
{
    PairingTransactionId pairingTransactionId = PairingTransactionId::createPairingTransactionId();
    DeviceDescriptor descriptor = m_discoveredDevices.value(deviceDescriptorId);
    if (!descriptor.isValid()) {
        qCWarning(dcDeviceManager) << "Cannot find a DeviceDescriptor with ID" << deviceDescriptorId.toString();
        DevicePairingInfo *info = new DevicePairingInfo(pairingTransactionId, DeviceClassId(), DeviceId(), name, ParamList(), DeviceId(), this);
        info->finish(Device::DeviceErrorDeviceDescriptorNotFound);
        return info;
    }

    DeviceClass deviceClass = m_supportedDevices.value(descriptor.deviceClassId());
    if (!deviceClass.isValid()) {
        qCWarning(dcDeviceManager) << "Cannot find a DeviceClass with ID" << descriptor.deviceClassId().toString();
        DevicePairingInfo *info = new DevicePairingInfo(pairingTransactionId, descriptor.deviceClassId(), DeviceId(), name, ParamList(), DeviceId(), this);
        info->finish(Device::DeviceErrorDeviceClassNotFound);
        return info;
    }

    DeviceId deviceId = descriptor.deviceId();
    // If it's a new device (not a reconfiguration), create a new DeviceId now.
    if (deviceId.isNull()) {
        deviceId = DeviceId::createDeviceId();
    }

    // Use given params, if there are missing some, use the discovered ones.
    ParamList finalParams = buildParams(deviceClass.paramTypes(), params, descriptor.params());

    DevicePairingInfo *info = new DevicePairingInfo(pairingTransactionId, descriptor.deviceClassId(), deviceId, name, finalParams, descriptor.parentDeviceId(), this, 30000);
    pairDeviceInternal(info);
    return info;
}

DevicePairingInfo *DeviceManagerImplementation::pairDevice(const DeviceId &deviceId, const ParamList &params, const QString &name)
{
    PairingTransactionId pairingTransactionId = PairingTransactionId::createPairingTransactionId();

    Device *device = findConfiguredDevice(deviceId);
    if (!device) {
        qCWarning(dcDeviceManager) << "Cannot find a Device with ID" << deviceId.toString();
        DevicePairingInfo *info = new DevicePairingInfo(pairingTransactionId, DeviceClassId(), deviceId, name, ParamList(), DeviceId(), this);
        info->finish(Device::DeviceErrorDeviceDescriptorNotFound);
        return info;
    }

    // Use new params, if there are missing some, use the existing ones.
    ParamList finalParams = buildParams(device->deviceClass().paramTypes(), params, device->params());

    DevicePairingInfo *info = new DevicePairingInfo(pairingTransactionId, device->deviceClassId(), deviceId, name, finalParams, DeviceId(), this, 30000);
    pairDeviceInternal(info);
    return info;
}

/*! Confirms the pairing of a \l{Device} with the given \a pairingTransactionId and \a secret.
 *  Returns \l{Device::DeviceError}{DeviceError} to inform about the result. */
DevicePairingInfo *DeviceManagerImplementation::confirmPairing(const PairingTransactionId &pairingTransactionId, const QString &username, const QString &secret)
{
    if (!m_pendingPairings.contains(pairingTransactionId)) {
        qCWarning(dcDeviceManager()) << "No pairing transaction with id" << pairingTransactionId.toString();
        DevicePairingInfo *info = new DevicePairingInfo(pairingTransactionId, DeviceClassId(), DeviceId(), QString(), ParamList(), DeviceId(), this);
        info->finish(Device::DeviceErrorPairingTransactionIdNotFound);
        return info;
    }

    PairingContext context = m_pendingPairings.take(pairingTransactionId);
    DeviceClassId deviceClassId = context.deviceClassId;

    DeviceClass deviceClass = m_supportedDevices.value(deviceClassId);
    DevicePlugin *plugin = m_devicePlugins.value(deviceClass.pluginId());
    if (!plugin) {
        qCWarning(dcDeviceManager) << "Can't find a plugin for this device class:" << deviceClass;
        DevicePairingInfo *info = new DevicePairingInfo(pairingTransactionId, deviceClassId, context.deviceId, context.deviceName, context.params, context.parentDeviceId, this);
        info->finish(Device::DeviceErrorPluginNotFound);
        return info;
    }

    DeviceId deviceId = context.deviceId;
    // If we already have a device for this ID, we're reconfiguring an existing device, else we're adding a new one.
    bool addNewDevice = !m_configuredDevices.contains(context.deviceId);

    // We're using two different info objects here, one to hand over to the plugin for the pairing, the other we give out
    // to the user. After the internal one has finished, we'll start a setupDevice job and finish the external pairingInfo only after
    // both, the internal pairing and the setup have completed.
    DevicePairingInfo *internalInfo = new DevicePairingInfo(pairingTransactionId, deviceClassId, deviceId, context.deviceName, context.params, context.parentDeviceId, this);
    DevicePairingInfo *externalInfo = new DevicePairingInfo(pairingTransactionId, deviceClassId, deviceId, context.deviceName, context.params, context.parentDeviceId, this);
    plugin->confirmPairing(internalInfo, username, secret);

    connect(internalInfo, &DevicePairingInfo::finished, this, [this, internalInfo, externalInfo, plugin, addNewDevice](){

        // Internal pairing failed, so fail the exernal one too.
        if (internalInfo->status() != Device::DeviceErrorNoError) {
            qCWarning(dcDeviceManager()) << "ConfirmPairing failed for" << internalInfo->deviceName() << internalInfo->deviceClassId();
            externalInfo->finish(internalInfo->status(), internalInfo->displayMessage());
            return;
        }

        // Internal pairing succeeded, set up the device.
        if (!addNewDevice && !m_configuredDevices.contains(internalInfo->deviceId())) {
            qCWarning(dcDeviceManager) << "The device to be reconfigured has disappeared!";
            externalInfo->finish(Device::DeviceErrorDeviceNotFound);
            return;
        }

        DeviceClass deviceClass = m_supportedDevices.value(internalInfo->deviceClassId());
        Device *device = nullptr;

        if (addNewDevice) {
            device = new Device(plugin->pluginId(), deviceClass, internalInfo->deviceId(), this);
            if (internalInfo->deviceName().isEmpty()) {
                device->setName(deviceClass.displayName());
            } else {
                device->setName(internalInfo->deviceName());
            }
        } else {
            device = m_configuredDevices.value(internalInfo->deviceId());
            device->setSetupStatus(Device::DeviceSetupStatusInProgress, Device::DeviceErrorNoError);
            qCDebug(dcDeviceManager()) << "Reconfiguring device" << device;
        }

        device->setParams(internalInfo->params());
        ParamList settings = buildParams(deviceClass.settingsTypes(), ParamList());
        device->setSettings(settings);

        DeviceSetupInfo *info = setupDevice(device);
        connect(info, &DeviceSetupInfo::finished, device, [this, info, externalInfo, addNewDevice](){

            externalInfo->finish(info->status(), info->displayMessage());

            if (info->status() != Device::DeviceErrorNoError) {
                if (addNewDevice) {
                    qCWarning(dcDeviceManager()) << "Failed to set up device" << info->device()->name()
                                                 << "Not adding device to the system. Error:"
                                                  << info->status() << info->displayMessage();
                    info->device()->deleteLater();
                }

                if (!addNewDevice) {
                    qCWarning(dcDeviceManager()) << "Failed to reconfigure device" << info->device()->name() <<
                                                    "Error:" << info->status() << info->displayMessage();
                    info->device()->setSetupStatus(Device::DeviceSetupStatusFailed, info->status(), info->displayMessage());
                    // TODO: restore parameters?
                }

                return;
            }

            qCDebug(dcDeviceManager()) << "Setup complete for device" << info->device();
            info->device()->setSetupStatus(Device::DeviceSetupStatusComplete, Device::DeviceErrorNoError);

            if (addNewDevice) {
                qCDebug(dcDeviceManager()) << "Device added:" << info->device();
                m_configuredDevices.insert(info->device()->id(), info->device());
                emit deviceAdded(info->device());
            } else {
                emit deviceChanged(info->device());
            }

            storeConfiguredDevices();
            postSetupDevice(info->device());
        });

    });

    return externalInfo;
}

/*! This method will only be used from the DeviceManagerImplementation in order to add a \l{Device} with the given \a deviceClassId, \a name, \a params and \ id.
 *  Returns \l{DeviceError} to inform about the result. */
DeviceSetupInfo* DeviceManagerImplementation::addConfiguredDeviceInternal(const DeviceClassId &deviceClassId, const QString &name, const ParamList &params, const DeviceId &parentDeviceId)
{
    DeviceClass deviceClass = findDeviceClass(deviceClassId);
    if (deviceClass.id().isNull()) {
        qCWarning(dcDeviceManager()) << "Cannot add device. DeviceClass" << deviceClassId << "not found.";
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        info->finish(Device::DeviceErrorDeviceClassNotFound);
        return info;
    }

    if (deviceClass.setupMethod() != DeviceClass::SetupMethodJustAdd) {
        qCWarning(dcDeviceManager()) << "Cannot add device. This device cannot be added this way. (SetupMethodJustAdd)";
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        info->finish(Device::DeviceErrorCreationMethodNotSupported);
        return info;
    }

    DeviceId deviceId = DeviceId::createDeviceId();
    // Chances are like 0, but...
    while (m_configuredDevices.contains(deviceId)) {
        deviceId = DeviceId::createDeviceId();
    }

    DevicePlugin *plugin = m_devicePlugins.value(deviceClass.pluginId());
    if (!plugin) {
        qCWarning(dcDeviceManager()) << "Cannot add device. Plugin for device class" << deviceClass.name() << "not found.";
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        info->finish(Device::DeviceErrorPluginNotFound);
        return info;
    }

    // set params
    ParamList effectiveParams = buildParams(deviceClass.paramTypes(), params);
    Device::DeviceError paramsResult = DeviceUtils::verifyParams(deviceClass.paramTypes(), effectiveParams);
    if (paramsResult != Device::DeviceErrorNoError) {
        qCWarning(dcDeviceManager()) << "Cannot add device. Parameter verification failed.";
        DeviceSetupInfo *info = new DeviceSetupInfo(nullptr, this);
        info->finish(paramsResult);
        return info;
    }

    Device *device = new Device(plugin->pluginId(), deviceClass, deviceId, this);
    device->setParentId(parentDeviceId);
    if (name.isEmpty()) {
        device->setName(deviceClass.name());
    } else {
        device->setName(name);
    }
    device->setParams(effectiveParams);

    // set settings (init with defaults)
    ParamList settings = buildParams(deviceClass.settingsTypes(), ParamList());
    qCDebug(dcDeviceManager()) << "Adding device settings" << settings << deviceId;
    device->setSettings(settings);

    DeviceSetupInfo *info = setupDevice(device);
    connect(info, &DeviceSetupInfo::finished, this, [this, info](){
        if (info->status() != Device::DeviceErrorNoError) {
            qCWarning(dcDeviceManager) << "Device setup failed. Not adding device to system.";
            info->device()->deleteLater();
            return;
        }

        info->device()->setSetupStatus(Device::DeviceSetupStatusComplete, Device::DeviceErrorNoError);

        qCDebug(dcDeviceManager) << "Device setup complete.";
        m_configuredDevices.insert(info->device()->id(), info->device());
        storeConfiguredDevices();
        postSetupDevice(info->device());

        emit deviceAdded(info->device());
    });

    return info;
}

/*! Removes a \l{Device} with the given \a deviceId from the list of configured \l{Device}{Devices}.
 *  This method also deletes all saved settings of the \l{Device}.
 *  Returns \l{DeviceError} to inform about the result. */
Device::DeviceError DeviceManagerImplementation::removeConfiguredDevice(const DeviceId &deviceId)
{
    Device *device = m_configuredDevices.take(deviceId);
    if (!device) {
        return Device::DeviceErrorDeviceNotFound;
    }
    m_devicePlugins.value(device->pluginId())->deviceRemoved(device);

    device->deleteLater();

    NymeaSettings settings(NymeaSettings::SettingsRoleDevices);
    settings.beginGroup("DeviceConfig");
    settings.beginGroup(deviceId.toString());
    settings.remove("");
    settings.endGroup();

    NymeaSettings stateCache(NymeaSettings::SettingsRoleDeviceStates);
    stateCache.remove(deviceId.toString());

    emit deviceRemoved(deviceId);

    return Device::DeviceErrorNoError;
}

BrowseResult *DeviceManagerImplementation::browseDevice(const DeviceId &deviceId, const QString &itemId, const QLocale &locale)
{
    Device *device = m_configuredDevices.value(deviceId);

    BrowseResult *result = new BrowseResult(device, itemId, locale, this, 30000);

    if (!device) {
        qCWarning(dcDeviceManager()) << "Cannot browse device. No such device:" << deviceId.toString();
        result->finish(Device::DeviceErrorDeviceNotFound);
        return result;
    }

    DevicePlugin *plugin = m_devicePlugins.value(device->pluginId());
    if (!plugin) {
        qCWarning(dcDeviceManager()) << "Cannot browse device. Plugin not found for device" << device;
        return result;
    }

    if (!device->setupComplete()) {
        qCWarning(dcDeviceManager()) << "Cannot browse device. Device did not finish setup" << device;
        return result;
    }

    if (!device->deviceClass().browsable()) {
        qCWarning(dcDeviceManager()) << "Cannot browse device. DeviceClass" << device->deviceClass().name() << "is not browsable.";
        result->finish(Device::DeviceErrorUnsupportedFeature);
        return result;
    }

    plugin->browseDevice(result);
    connect(result, &BrowseResult::finished, this, [result](){
        if (result->status() != Device::DeviceErrorNoError) {
            qCWarning(dcDeviceManager()) << "Browse device failed:" << result->status();
        }
    });
    return result;
}

BrowserItemResult *DeviceManagerImplementation::browserItemDetails(const DeviceId &deviceId, const QString &itemId, const QLocale &locale)
{
    Device *device = m_configuredDevices.value(deviceId);

    BrowserItemResult *result = new BrowserItemResult(device, itemId, locale, this, 30000);

    if (!device) {
        qCWarning(dcDeviceManager()) << "Cannot browse device. No such device:" << deviceId.toString();
        result->finish(Device::DeviceErrorDeviceNotFound);
        return result;
    }

    DevicePlugin *plugin = m_devicePlugins.value(device->pluginId());
    if (!plugin) {
        qCWarning(dcDeviceManager()) << "Cannot browse device. Plugin not found for device" << device;
        return result;
    }

    if (device->setupStatus() != Device::DeviceSetupStatusComplete) {
        qCWarning(dcDeviceManager()) << "Cannot browse device. Device did not finish setup" << device;
        return result;
    }

    if (!device->deviceClass().browsable()) {
        qCWarning(dcDeviceManager()) << "Cannot browse device. DeviceClass" << device->deviceClass().name() << "is not browsable.";
        result->finish(Device::DeviceErrorUnsupportedFeature);
        return result;
    }

    plugin->browserItem(result);
    connect(result, &BrowserItemResult::finished, this, [result](){
        if (result->status() != Device::DeviceErrorNoError) {
            qCWarning(dcDeviceManager()) << "Browse device failed:" << result->status();
        }
    });
    return result;
}

BrowserActionInfo* DeviceManagerImplementation::executeBrowserItem(const BrowserAction &browserAction)
{
    Device *device = m_configuredDevices.value(browserAction.deviceId());

    BrowserActionInfo *info = new BrowserActionInfo(device, browserAction, this, 30000);

    if (!device) {
        info->finish(Device::DeviceErrorDeviceNotFound);
        return info;
    }

    DevicePlugin *plugin = m_devicePlugins.value(device->pluginId());
    if (!plugin) {
        qCWarning(dcDeviceManager()) << "Cannot browse device. Plugin not found for device" << device;
        info->finish(Device::DeviceErrorPluginNotFound);
        return info;
    }

    if (device->setupStatus() != Device::DeviceSetupStatusComplete) {
        qCWarning(dcDeviceManager()) << "Cannot browse device. Device did not finish setup" << device;
        info->finish(Device::DeviceErrorSetupFailed);
        return info;
    }

    if (!device->deviceClass().browsable()) {
        info->finish(Device::DeviceErrorUnsupportedFeature);
        return info;
    }
    plugin->executeBrowserItem(info);
    return info;
}

BrowserItemActionInfo* DeviceManagerImplementation::executeBrowserItemAction(const BrowserItemAction &browserItemAction)
{
    Device *device = m_configuredDevices.value(browserItemAction.deviceId());

    BrowserItemActionInfo *info = new BrowserItemActionInfo(device, browserItemAction, this, 30000);

    if (!device) {
        info->finish(Device::DeviceErrorDeviceNotFound);
        return info;
    }

    DevicePlugin *plugin = m_devicePlugins.value(device->pluginId());
    if (!plugin) {
        qCWarning(dcDeviceManager()) << "Cannot execute browser item action. Plugin not found for device" << device;
        info->finish(Device::DeviceErrorPluginNotFound);
        return info;
    }

    if (device->setupStatus() != Device::DeviceSetupStatusComplete) {
        qCWarning(dcDeviceManager()) << "Cannot execute browser item action. Device did not finish setup" << device;
        info->finish(Device::DeviceErrorSetupFailed);
        return info;
    }

    if (!device->deviceClass().browsable()) {
        info->finish(Device::DeviceErrorUnsupportedFeature);
        return info;
    }
    // TODO: check browserItemAction.params with deviceClass

    plugin->executeBrowserItemAction(info);
    return info;
}

QString DeviceManagerImplementation::translate(const PluginId &pluginId, const QString &string, const QLocale &locale)
{
    return m_translator->translate(pluginId, string, locale);
}

ParamType DeviceManagerImplementation::translateParamType(const PluginId &pluginId, const ParamType &paramType, const QLocale &locale)
{
    ParamType translatedParamType = paramType;
    translatedParamType.setDisplayName(translate(pluginId, paramType.displayName(), locale));
    return translatedParamType;
}

DeviceClass DeviceManagerImplementation::translateDeviceClass(const DeviceClass &deviceClass, const QLocale &locale)
{
    DeviceClass translatedDeviceClass = deviceClass;
    translatedDeviceClass.setDisplayName(translate(deviceClass.pluginId(), deviceClass.displayName(), locale));

    ParamTypes translatedSettingsTypes;
    foreach (const ParamType &paramType, deviceClass.settingsTypes()) {
        translatedSettingsTypes.append(translateParamType(deviceClass.pluginId(), paramType, locale));
    }
    translatedDeviceClass.setSettingsTypes(translatedSettingsTypes);
    return translatedDeviceClass;
}

Vendor DeviceManagerImplementation::translateVendor(const Vendor &vendor, const QLocale &locale)
{
    DevicePlugin *plugin = nullptr;
    foreach (DevicePlugin *p, m_devicePlugins) {
        if (p->supportedVendors().contains(vendor)) {
            plugin = p;
        }
    }
    if (!plugin) {
        return vendor;
    }

    Vendor translatedVendor = vendor;
    translatedVendor.setDisplayName(translate(plugin->pluginId(), vendor.displayName(), locale));
    return translatedVendor;
}

/*! Returns the \l{Device} with the given \a id. Null if the id couldn't be found. */
Device *DeviceManagerImplementation::findConfiguredDevice(const DeviceId &id) const
{
    foreach (Device *device, m_configuredDevices) {
        if (device->id() == id) {
            return device;
        }
    }
    return nullptr;
}

/*! Returns all configured \{Device}{Devices} in the system. */
Devices DeviceManagerImplementation::configuredDevices() const
{
    return m_configuredDevices.values();
}

/*! Returns all \l{Device}{Devices} matching the \l{DeviceClass} referred by \a deviceClassId. */
Devices DeviceManagerImplementation::findConfiguredDevices(const DeviceClassId &deviceClassId) const
{
    QList<Device*> ret;
    foreach (Device *device, m_configuredDevices) {
        if (device->deviceClassId() == deviceClassId) {
            ret << device;
        }
    }
    return ret;
}

/*! Returns all \l{Device}{Devices} with the given \a interface. See also \l{Interfaces for DeviceClasses}{interfaces}. */
Devices DeviceManagerImplementation::findConfiguredDevices(const QString &interface) const
{
    QList<Device*> ret;
    foreach (Device *device, m_configuredDevices) {
        DeviceClass dc = m_supportedDevices.value(device->deviceClassId());
        if (dc.interfaces().contains(interface)) {
            ret.append(device);
        }
    }
    return ret;
}

/*! Returns all child \l{Device}{Devices} of the \l{Device} with the given \a id. */
Devices DeviceManagerImplementation::findChildDevices(const DeviceId &id) const
{
    QList<Device *> ret;
    foreach (Device *d, m_configuredDevices) {
        if (d->parentId() == id) {
            ret.append(d);
        }
    }
    return ret;
}

/*! For conveninece, this returns the \l{DeviceClass} with the id given by \a deviceClassId.
 *  Note: The returned \l{DeviceClass} may be invalid. */
DeviceClass DeviceManagerImplementation::findDeviceClass(const DeviceClassId &deviceClassId) const
{
    foreach (const DeviceClass &deviceClass, m_supportedDevices) {
        if (deviceClass.id() == deviceClassId) {
            return deviceClass;
        }
    }
    return DeviceClass();
}

/*! Execute the given \l{Action}.
 *  This will find the \l{Device} \a action refers to the \l{Action}{deviceId()} and
 *  its \l{DevicePlugin}. Then will dispatch the execution to the \l{DevicePlugin}.*/
DeviceActionInfo *DeviceManagerImplementation::executeAction(const Action &action)
{
    Action finalAction = action;
    Device *device = m_configuredDevices.value(action.deviceId());
    if (!device) {
        qCWarning(dcDeviceManager()) << "Cannot execute action. No such device:" << action.deviceId();
        DeviceActionInfo *info = new DeviceActionInfo(nullptr, action, this);
        info->finish(Device::DeviceErrorDeviceNotFound);
        return info;
    }

    if (!device->setupComplete()) {
        qCWarning(dcDeviceManager()) << "Cannot execute action. Device" << device->name() << "hasn't completed setup.";
        DeviceActionInfo *info = new DeviceActionInfo(nullptr, action, this);
        info->finish(Device::DeviceErrorSetupFailed);
        return info;
    }

    // Make sure this device has an action type with this id
    DeviceClass deviceClass = findDeviceClass(device->deviceClassId());
    ActionType actionType = deviceClass.actionTypes().findById(action.actionTypeId());
    if (actionType.id().isNull()) {
        qCWarning(dcDeviceManager()) << "Cannot execute action. No such action type" << action.actionTypeId();
        DeviceActionInfo *info = new DeviceActionInfo(device, action, this);
        info->finish(Device::DeviceErrorActionTypeNotFound);
        return info;
    }

    ParamList finalParams = buildParams(actionType.paramTypes(), action.params());
    Device::DeviceError paramCheck = DeviceUtils::verifyParams(actionType.paramTypes(), finalParams);
    if (paramCheck != Device::DeviceErrorNoError) {
        qCWarning(dcDeviceManager()) << "Cannot execute action. Parameter verification failed.";
        DeviceActionInfo *info = new DeviceActionInfo(device, action, this);
        info->finish(paramCheck);
        return info;
    }
    finalAction.setParams(finalParams);

    DeviceActionInfo *info = new DeviceActionInfo(device, finalAction, this, 30000);

    DevicePlugin *plugin = m_devicePlugins.value(device->pluginId());
    if (!plugin) {
        qCWarning(dcDeviceManager()) << "Cannot execute action. Plugin not found for device" << device->name();
        info->finish(Device::DeviceErrorPluginNotFound);
        return info;
    }

    plugin->executeAction(info);

    return info;
}

void DeviceManagerImplementation::loadPlugins()
{
    foreach (const QString &path, pluginSearchDirs()) {
        QDir dir(path);
        qCDebug(dcDeviceManager) << "Loading plugins from:" << dir.absolutePath();
        foreach (const QString &entry, dir.entryList()) {
            QFileInfo fi;
            if (entry.startsWith("libnymea_deviceplugin") && entry.endsWith(".so")) {
                fi.setFile(path + "/" + entry);
            } else {
                fi.setFile(path + "/" + entry + "/libnymea_deviceplugin" + entry + ".so");
            }

            if (!fi.exists())
                continue;

            // Check plugin API version compatibility
            QLibrary lib(fi.absoluteFilePath());
            QFunctionPointer versionFunc = lib.resolve("libnymea_api_version");
            if (!versionFunc) {
                qCWarning(dcDeviceManager()).nospace() << "Unable to resolve version in plugin " << entry << ". Not loading plugin.";
                lib.unload();
                continue;
            }

            QString version = reinterpret_cast<QString(*)()>(versionFunc)();
            lib.unload();
            QStringList parts = version.split('.');
            QStringList coreParts = QString(LIBNYMEA_API_VERSION).split('.');
            if (parts.length() != 3 || parts.at(0).toInt() != coreParts.at(0).toInt() || parts.at(1).toInt() > coreParts.at(1).toInt()) {
                qCWarning(dcDeviceManager()).nospace() << "Libnymea API mismatch for " << entry << ". Core API: " << LIBNYMEA_API_VERSION << ", Plugin API: " << version;
                continue;
            }

            // Version is ok. Now load the plugin
            QPluginLoader loader;
            loader.setFileName(fi.absoluteFilePath());
            loader.setLoadHints(QLibrary::ResolveAllSymbolsHint);

            qCDebug(dcDeviceManager()) << "Loading plugin from:" << fi.absoluteFilePath();
            if (!loader.load()) {
                qCWarning(dcDeviceManager) << "Could not load plugin data of" << entry << "\n" << loader.errorString();
                continue;
            }

            QJsonObject pluginInfo = loader.metaData().value("MetaData").toObject();
            PluginMetadata metaData(pluginInfo);
            if (!metaData.isValid()) {
                foreach (const QString &error, metaData.validationErrors()) {
                    qCWarning(dcDeviceManager()) << error;
                }
                loader.unload();
                continue;
            }

            DevicePlugin *pluginIface = qobject_cast<DevicePlugin *>(loader.instance());
            if (!pluginIface) {
                qCWarning(dcDeviceManager) << "Could not get plugin instance of" << entry;
                loader.unload();
                continue;
            }
            if (m_devicePlugins.contains(pluginIface->pluginId())) {
                qCWarning(dcDeviceManager()) << "A plugin with this ID is already loaded. Not loading" << entry;
                continue;
            }
            loadPlugin(pluginIface, metaData);
            PluginInfoCache::cachePluginInfo(pluginInfo);
        }
    }

#if QT_VERSION >= QT_VERSION_CHECK(5,12,0)
    foreach (const QString &path, pluginSearchDirs()) {
        QDir dir(path);
        qCDebug(dcDeviceManager) << "Loading JS plugins from:" << dir.absolutePath();
        foreach (const QString &entry, dir.entryList()) {
            QFileInfo jsFi;
            QFileInfo jsonFi;

            if (entry.endsWith(".js")) {
                jsFi.setFile(path + "/" + entry);
            } else {
                jsFi.setFile(path + "/" + entry + "/" + entry + ".js");
            }

            if (!jsFi.exists()) {
                continue;
            }

            ScriptDevicePlugin *plugin = new ScriptDevicePlugin(this);
            bool ret = plugin->loadScript(jsFi.absoluteFilePath());
            if (!ret) {
                delete plugin;
                qCWarning(dcDeviceManager()) << "JS plugin failed to load";
                continue;
            }
            PluginMetadata metaData(plugin->metaData());
            if (!metaData.isValid()) {
                qCWarning(dcDeviceManager()) << "Not loading JS plugin. Invalid metadata.";
                foreach (const QString &error, metaData.validationErrors()) {
                    qCWarning(dcDeviceManager()) << error;
                }
            }
            loadPlugin(plugin, metaData);
        }
    }
#endif
}

void DeviceManagerImplementation::loadPlugin(DevicePlugin *pluginIface, const PluginMetadata &metaData)
{
    pluginIface->setParent(this);
    pluginIface->initPlugin(metaData, this, m_hardwareManager);

    qCDebug(dcDeviceManager) << "**** Loaded plugin" << pluginIface->pluginName();
    foreach (const Vendor &vendor, pluginIface->supportedVendors()) {
        qCDebug(dcDeviceManager) << "* Loaded vendor:" << vendor.name() << vendor.id();
        if (m_supportedVendors.contains(vendor.id()))
            continue;

        m_supportedVendors.insert(vendor.id(), vendor);
    }

    foreach (const DeviceClass &deviceClass, pluginIface->supportedDevices()) {
        if (!m_supportedVendors.contains(deviceClass.vendorId())) {
            qCWarning(dcDeviceManager) << "Vendor not found. Ignoring device. VendorId:" << deviceClass.vendorId() << "DeviceClass:" << deviceClass.name() << deviceClass.id();
            continue;
        }
        m_vendorDeviceMap[deviceClass.vendorId()].append(deviceClass.id());
        m_supportedDevices.insert(deviceClass.id(), deviceClass);
        qCDebug(dcDeviceManager) << "* Loaded device class:" << deviceClass.name();
    }

    NymeaSettings settings(NymeaSettings::SettingsRolePlugins);
    settings.beginGroup("PluginConfig");
    ParamList params;
    if (settings.childGroups().contains(pluginIface->pluginId().toString())) {
        settings.beginGroup(pluginIface->pluginId().toString());

        if (!settings.childGroups().isEmpty()) {
            // Note: since nymea 0.12.2 the param type gets saved too for better data converting
            foreach (const QString &paramTypeIdString, settings.childGroups()) {
                ParamTypeId paramTypeId(paramTypeIdString);
                ParamType paramType = pluginIface->configurationDescription().findById(paramTypeId);
                if (!paramType.isValid()) {
                    qCWarning(dcDeviceManager()) << "Not loading Param for plugin" << pluginIface->pluginName() << "because the ParamType for the saved Param" << ParamTypeId(paramTypeIdString).toString() << "could not be found.";
                    continue;
                }

                QVariant paramValue;
                settings.beginGroup(paramTypeIdString);
                paramValue = settings.value("value", paramType.defaultValue());
                paramValue.convert(settings.value("type").toInt());
                params.append(Param(paramTypeId, paramValue));
                settings.endGroup();
            }
        } else {
            // Note: < nymea 0.12.2
            foreach (const QString &paramTypeIdString, settings.allKeys()) {
                params.append(Param(ParamTypeId(paramTypeIdString), settings.value(paramTypeIdString)));
            }
        }

        settings.endGroup();
    } else if (!pluginIface->configurationDescription().isEmpty()){
        // plugin requires config but none stored. Init with defaults
        foreach (const ParamType &paramType, pluginIface->configurationDescription()) {
            Param param(paramType.id(), paramType.defaultValue());
            params.append(param);
        }
    }
    settings.endGroup();

    if (params.count() > 0) {
        Device::DeviceError status = pluginIface->setConfiguration(params);
        if (status != Device::DeviceErrorNoError) {
            qCWarning(dcDeviceManager) << "Error setting params to plugin. Broken configuration?";
        }
    }

    // Call the init method of the plugin
    pluginIface->init();

    m_devicePlugins.insert(pluginIface->pluginId(), pluginIface);

    connect(pluginIface, &DevicePlugin::emitEvent, this, &DeviceManagerImplementation::onEventTriggered, Qt::QueuedConnection);
    connect(pluginIface, &DevicePlugin::autoDevicesAppeared, this, &DeviceManagerImplementation::onAutoDevicesAppeared, Qt::QueuedConnection);
    connect(pluginIface, &DevicePlugin::autoDeviceDisappeared, this, &DeviceManagerImplementation::onAutoDeviceDisappeared, Qt::QueuedConnection);
}

void DeviceManagerImplementation::loadConfiguredDevices()
{
    NymeaSettings settings(NymeaSettings::SettingsRoleDevices);
    settings.beginGroup("DeviceConfig");
    qCDebug(dcDeviceManager) << "Loading devices from" << settings.fileName();
    foreach (const QString &idString, settings.childGroups()) {
        settings.beginGroup(idString);
        QString deviceName = settings.value("devicename").toString();
        PluginId pluginId = PluginId(settings.value("pluginid").toString());
        DevicePlugin *plugin = m_devicePlugins.value(pluginId);
        if (!plugin) {
            qCWarning(dcDeviceManager()) << "Plugin for device" << deviceName << idString << "not found. This device will not be functional until the plugin can be loaded.";
        }
        DeviceClassId deviceClassId = DeviceClassId(settings.value("deviceClassId").toString());
        DeviceClass deviceClass = findDeviceClass(deviceClassId);
        if (!deviceClass.isValid()) {
            // Try to load the device class from the cache
            QJsonObject pluginInfo = PluginInfoCache::loadPluginInfo(pluginId);
            if (!pluginInfo.empty()) {
                PluginMetadata pluginMetadata(pluginInfo);
                deviceClass = pluginMetadata.deviceClasses().findById(deviceClassId);
                if (deviceClass.isValid()) {
                    m_supportedDevices.insert(deviceClassId, deviceClass);
                    if (!m_supportedVendors.contains(deviceClass.vendorId())) {
                        Vendor vendor = pluginMetadata.vendors().findById(deviceClass.vendorId());
                        m_supportedVendors.insert(vendor.id(), vendor);
                    }
                }
            }
        }
        if (!deviceClass.isValid()) {
            qCWarning(dcDeviceManager()) << "Not loading device" << deviceName << idString << "because the device class for this device could not be found.";
            settings.endGroup(); // DeviceId
            continue;
        }

        // Cross-check if this plugin still implements this device class
        if (plugin && !plugin->supportedDevices().contains(deviceClass)) {
            qCWarning(dcDeviceManager()) << "Not loading device" << deviceName << idString << "because plugin" << plugin->pluginName() << "has removed support for it.";
            settings.endGroup(); // DeviceId
            continue;
        }
        Device *device = new Device(pluginId, deviceClass, DeviceId(idString), this);
        device->m_autoCreated = settings.value("autoCreated").toBool();
        device->setName(deviceName);
        device->setParentId(DeviceId(settings.value("parentid", QUuid()).toString()));


        ParamList params;
        settings.beginGroup("Params");
        if (!settings.childGroups().isEmpty()) {
            foreach (const QString &paramTypeIdString, settings.childGroups()) {
                ParamTypeId paramTypeId(paramTypeIdString);
                ParamType paramType = deviceClass.paramTypes().findById(paramTypeId);
                QVariant defaultValue;
                if (!paramType.isValid()) {
                    // NOTE: We're not skipping unknown parameters to give plugins a chance to still access old values if they change their config and migrate things over.
                    qCWarning(dcDeviceManager()) << "Unknown param" << paramTypeIdString << "for" << device << ". ParamType could not be found in device class.";
                }

                // Note: since nymea 0.12.2
                QVariant paramValue;
                settings.beginGroup(paramTypeIdString);
                paramValue = settings.value("value", paramType.defaultValue());
                paramValue.convert(settings.value("type").toInt());
                params.append(Param(paramTypeId, paramValue));
                settings.endGroup(); // ParamId
            }
        } else {
            foreach (const QString &paramTypeIdString, settings.allKeys()) {
                params.append(Param(ParamTypeId(paramTypeIdString), settings.value(paramTypeIdString)));
            }
        }
        // Make sure all params are around. if they aren't initialize with default values
        foreach (const ParamType &paramType, deviceClass.paramTypes()) {
            if (!params.hasParam(paramType.id())) {
                params.append(Param(paramType.id(), paramType.defaultValue()));
            }
        }
        device->setParams(params);
        settings.endGroup(); // Params

        ParamList deviceSettings;
        settings.beginGroup("Settings");
        if (!settings.childGroups().isEmpty()) {
            foreach (const QString &paramTypeIdString, settings.childGroups()) {
                ParamTypeId paramTypeId(paramTypeIdString);
                ParamType paramType = deviceClass.settingsTypes().findById(paramTypeId);
                if (!paramType.isValid()) {
                    qCWarning(dcDeviceManager()) << "Not loading Setting for device" << device << "because the ParamType for the saved Setting" << ParamTypeId(paramTypeIdString).toString() << "could not be found.";
                    continue;
                }

                // Note: since nymea 0.12.2
                QVariant paramValue;
                settings.beginGroup(paramTypeIdString);
                paramValue = settings.value("value", paramType.defaultValue());
                paramValue.convert(settings.value("type").toInt());
                deviceSettings.append(Param(paramTypeId, paramValue));
                settings.endGroup(); // ParamId
            }
        } else {
            foreach (const QString &paramTypeIdString, settings.allKeys()) {
                params.append(Param(ParamTypeId(paramTypeIdString), settings.value(paramTypeIdString)));
            }
        }

        // Fill in any missing params with defaults
        deviceSettings = buildParams(deviceClass.settingsTypes(), deviceSettings);

        device->setSettings(deviceSettings);

        settings.endGroup(); // Settings
        settings.endGroup(); // DeviceId

        // We always add the device to the list in this case. If its in the storedDevices
        // it means that it was working at some point so lets still add it as there might
        // be rules associated with this device.
        m_configuredDevices.insert(device->id(), device);
    }
    settings.endGroup();

    QHash<DeviceId, Device*> setupList = m_configuredDevices;
    while (!setupList.isEmpty()) {
        Device *device = nullptr;
        foreach (Device *d, setupList) {
            if (d->parentId().isNull() || !setupList.contains(d->parentId())) {
                device = d;
                setupList.take(d->id());
                break;
            }
        }
        Q_ASSERT(device != nullptr);

        device->setSetupStatus(Device::DeviceSetupStatusInProgress, Device::DeviceErrorNoError);
        DeviceSetupInfo *info = setupDevice(device);
        // Set receiving object to "device" because at startup we load it in any case, knowing that it worked at
        // some point. However, it'll be marked as non-working until the setup succeeds so the user might delete
        // it in the meantime... In that case we don't want to call postsetup on it.
        connect(info, &DeviceSetupInfo::finished, device, [this, info](){

            if (info->status() != Device::DeviceErrorNoError) {
                qCWarning(dcDeviceManager()) << "Error setting up device" << info->device()->name() << info->device()->id().toString() << info->status() << info->displayMessage();
                info->device()->setSetupStatus(Device::DeviceSetupStatusFailed, info->status(), info->displayMessage());
                emit deviceChanged(info->device());
                return;
            }

            qCDebug(dcDeviceManager()) << "Setup complete for device" << info->device();
            info->device()->setSetupStatus(Device::DeviceSetupStatusComplete, Device::DeviceErrorNoError);
            emit deviceChanged(info->device());
            postSetupDevice(info->device());
        });
    }
}

void DeviceManagerImplementation::storeConfiguredDevices()
{
    NymeaSettings settings(NymeaSettings::SettingsRoleDevices);
    settings.beginGroup("DeviceConfig");
    foreach (Device *device, m_configuredDevices) {
        settings.beginGroup(device->id().toString());
        // Note: clean device settings before storing it for clean up
        settings.remove("");
        settings.setValue("autoCreated", device->autoCreated());
        settings.setValue("devicename", device->name());
        settings.setValue("deviceClassId", device->deviceClassId().toString());
        settings.setValue("pluginid", device->pluginId().toString());
        if (!device->parentId().isNull())
            settings.setValue("parentid", device->parentId().toString());

        settings.beginGroup("Params");
        foreach (const Param &param, device->params()) {
            settings.beginGroup(param.paramTypeId().toString());
            settings.setValue("type", static_cast<int>(param.value().type()));
            settings.setValue("value", param.value());
            settings.endGroup(); // ParamTypeId
        }
        settings.endGroup(); // Params

        settings.beginGroup("Settings");
        foreach (const Param &param, device->settings()) {
            settings.beginGroup(param.paramTypeId().toString());
            settings.setValue("type", static_cast<int>(param.value().type()));
            settings.setValue("value", param.value());
            settings.endGroup(); // ParamTypeId
        }
        settings.endGroup(); // Settings


        settings.endGroup(); // DeviceId
    }
    settings.endGroup(); // DeviceConfig
}

void DeviceManagerImplementation::startMonitoringAutoDevices()
{
    foreach (DevicePlugin *plugin, m_devicePlugins) {
        plugin->startMonitoringAutoDevices();
    }
}

void DeviceManagerImplementation::onAutoDevicesAppeared(const DeviceDescriptors &deviceDescriptors)
{
    foreach (const DeviceDescriptor &deviceDescriptor, deviceDescriptors) {

        DeviceClass deviceClass = findDeviceClass(deviceDescriptor.deviceClassId());
        if (!deviceClass.isValid()) {
            qCWarning(dcDeviceManager()) << "Ignoring auto device appeared for an unknown DeviceClass" << deviceDescriptor.deviceClassId();
            return;
        }

        DevicePlugin *plugin = m_devicePlugins.value(deviceClass.pluginId());
        if (!plugin) {
            return;
        }

        if (!deviceDescriptor.parentDeviceId().isNull() && !m_configuredDevices.contains(deviceDescriptor.parentDeviceId())) {
            qCWarning(dcDeviceManager()) << "Invalid parent device id. Not adding device to the system.";
            continue;
        }

        Device *device = nullptr;

        // If the appreaed auto device holds a valid device id, do a reconfiguration for this device
        if (!deviceDescriptor.deviceId().isNull()) {
            device = findConfiguredDevice(deviceDescriptor.deviceId());
            if (!device) {
                qCWarning(dcDeviceManager()) << "Could not find device for auto device descriptor" << deviceDescriptor.deviceId();
                continue;
            }
            qCDebug(dcDeviceManager()) << "Start reconfiguring auto device" << device;
            ParamList finalParams = buildParams(deviceClass.paramTypes(), deviceDescriptor.params());
            reconfigureDeviceInternal(device, finalParams);
            continue;
        }

        device = new Device(plugin->pluginId(), deviceClass, this);
        device->m_autoCreated = true;
        device->setName(deviceDescriptor.title());
        device->setParams(deviceDescriptor.params());
        ParamList settings = buildParams(deviceClass.settingsTypes(), ParamList());
        device->setSettings(settings);
        device->setParentId(deviceDescriptor.parentDeviceId());

        qCDebug(dcDeviceManager()) << "Setting up auto device:" << device->name() << device->id().toString();

        DeviceSetupInfo *info = setupDevice(device);
        connect(info, &DeviceSetupInfo::finished, device, [this, info](){

            if (info->status() != Device::DeviceErrorNoError) {
                qCWarning(dcDeviceManager) << "Device setup failed. Not adding auto device to system.";
                info->device()->deleteLater();
                return;
            }

            info->device()->setSetupStatus(Device::DeviceSetupStatusComplete, Device::DeviceErrorNoError);
            m_configuredDevices.insert(info->device()->id(), info->device());
            storeConfiguredDevices();

            emit deviceAdded(info->device());

            postSetupDevice(info->device());
        });
    }
}

void DeviceManagerImplementation::onAutoDeviceDisappeared(const DeviceId &deviceId)
{
    DevicePlugin *plugin = static_cast<DevicePlugin*>(sender());
    Device *device = m_configuredDevices.value(deviceId);

    if (!device) {
        qWarning(dcDeviceManager) << "Received an autoDeviceDisappeared signal but don't know this device:" << deviceId;
        return;
    }

    DeviceClass deviceClass = m_supportedDevices.value(device->deviceClassId());

    if (deviceClass.pluginId() != plugin->pluginId()) {
        qWarning(dcDeviceManager) << "Received a autoDeviceDisappeared signal but emitting plugin does not own the device";
        return;
    }

    if (!device->autoCreated()) {
        qWarning(dcDeviceManager) << "Received an autoDeviceDisappeared signal but device creationMethod is not CreateMothodAuto";
        return;
    }

    emit deviceDisappeared(deviceId);
}

void DeviceManagerImplementation::onLoaded()
{
    qCDebug(dcDeviceManager()) << "Done loading plugins and devices.";
    emit loaded();

    // schedule some housekeeping...
    QTimer::singleShot(0, this, SLOT(cleanupDeviceStateCache()));
}

void DeviceManagerImplementation::cleanupDeviceStateCache()
{
    NymeaSettings settings(NymeaSettings::SettingsRoleDeviceStates);
    foreach (const QString &entry, settings.childGroups()) {
        DeviceId deviceId(entry);
        if (!m_configuredDevices.contains(deviceId)) {
            qCDebug(dcDeviceManager()) << "Device ID" << deviceId << "not found in configured devices. Cleaning up stale device state cache.";
            settings.remove(entry);
        }
    }
}

void DeviceManagerImplementation::onEventTriggered(const Event &event)
{
    // Doing some sanity checks here...
    Device *device = m_configuredDevices.value(event.deviceId());
    if (!device) {
        qCWarning(dcDeviceManager()) << "Invalid device id in emitted event. Not forwarding event. Device setup not complete yet?";
        return;
    }
    EventType eventType = device->deviceClass().eventTypes().findById(event.eventTypeId());
    if (!eventType.isValid()) {
        qCWarning(dcDeviceManager()) << "The given device does not have an event type of id " + event.eventTypeId().toString() + ". Not forwarding event.";
        return;
    }
    // All good, forward the event
    emit eventTriggered(event);
}

void DeviceManagerImplementation::slotDeviceStateValueChanged(const StateTypeId &stateTypeId, const QVariant &value)
{
    Device *device = qobject_cast<Device*>(sender());
    if (!device || !m_configuredDevices.contains(device->id())) {
        qCWarning(dcDeviceManager()) << "Invalid device id in state change. Not forwarding event. Device setup not complete yet?";
        return;
    }
    emit deviceStateChanged(device, stateTypeId, value);

    Param valueParam(ParamTypeId(stateTypeId.toString()), value);
    Event event(EventTypeId(stateTypeId.toString()), device->id(), ParamList() << valueParam, true);
    emit eventTriggered(event);
}

void DeviceManagerImplementation::slotDeviceSettingChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    Device *device = qobject_cast<Device*>(sender());
    if (!device) {
        return;
    }
    storeConfiguredDevices();
    emit deviceSettingChanged(device->id(), paramTypeId, value);
}

void DeviceManagerImplementation::slotDeviceNameChanged()
{
    Device *device = qobject_cast<Device*>(sender());
    if (!device) {
        return;
    }
    storeConfiguredDevices();
    emit deviceChanged(device);
}

ParamList DeviceManagerImplementation::buildParams(const ParamTypes &types, const ParamList &first, const ParamList &second)
{
    // Merge params from discovered descriptor and additional overrides provided on API call. User provided params have higher priority than discovery params.
    ParamList finalParams;
    foreach (const ParamType &paramType, types) {
        if (first.hasParam(paramType.id())) {
            finalParams.append(Param(paramType.id(), first.paramValue(paramType.id())));
        } else if (second.hasParam(paramType.id())) {
            finalParams.append(Param(paramType.id(), second.paramValue(paramType.id())));
        } else if (paramType.defaultValue().isValid()){
            finalParams.append(Param(paramType.id(), paramType.defaultValue()));
        }
    }
    return finalParams;
}

void DeviceManagerImplementation::pairDeviceInternal(DevicePairingInfo *info)
{
    DeviceClass deviceClass = m_supportedDevices.value(info->deviceClassId());
    if (deviceClass.id().isNull()) {
        qCWarning(dcDeviceManager) << "Cannot find a device class with id" << info->deviceClassId();
        info->finish(Device::DeviceErrorDeviceClassNotFound);
        return;
    }

    if (deviceClass.setupMethod() == DeviceClass::SetupMethodJustAdd) {
        qCWarning(dcDeviceManager) << "Cannot setup this device this way. No need to pair this device.";
        info->finish(Device::DeviceErrorSetupMethodNotSupported);
        return;
    }

    DevicePlugin *plugin = m_devicePlugins.value(deviceClass.pluginId());
    if (!plugin) {
        qCWarning(dcDeviceManager) << "Cannot pair device class" << deviceClass.name() << "because no plugin for it is loaded.";
        info->finish(Device::DeviceErrorPluginNotFound);
        return;
    }

    plugin->startPairing(info);

    connect(info, &DevicePairingInfo::finished, this, [this, info, deviceClass](){
        if (info->status() != Device::DeviceErrorNoError) {
            qCWarning(dcDeviceManager()) << "PairDevice failed for" << deviceClass << info->status() << info->displayMessage();
            return;
        }

        qCDebug(dcDeviceManager()) << "Pairing started for" << deviceClass << "using transaction id" << info->transactionId();
        PairingContext context;
        context.deviceId = info->deviceId();
        context.deviceClassId = info->deviceClassId();
        context.deviceName = info->deviceName();
        context.params = info->params();
        context.parentDeviceId = info->parentDeviceId();
        m_pendingPairings.insert(info->transactionId(), context);

        // TODO: Add a timer to clean up stuff if confirmPairing is never called.
    });
}

DeviceSetupInfo* DeviceManagerImplementation::setupDevice(Device *device)
{
    DeviceClass deviceClass = findDeviceClass(device->deviceClassId());
    DevicePlugin *plugin = m_devicePlugins.value(deviceClass.pluginId());

    if (!plugin) {
        qCWarning(dcDeviceManager) << "Can't find a plugin for this device" << device;
        DeviceSetupInfo *info = new DeviceSetupInfo(device, this);
        info->finish(Device::DeviceErrorPluginNotFound, tr("The plugin for this device or service is not loaded."));
        return info;
    }

    QList<State> states;
    foreach (const StateType &stateType, deviceClass.stateTypes()) {
        State state(stateType.id(), device->id());
        states.append(state);
    }
    device->setStates(states);
    loadDeviceStates(device);

    connect(device, &Device::stateValueChanged, this, &DeviceManagerImplementation::slotDeviceStateValueChanged);
    connect(device, &Device::settingChanged, this, &DeviceManagerImplementation::slotDeviceSettingChanged);
    connect(device, &Device::nameChanged, this, &DeviceManagerImplementation::slotDeviceNameChanged);


    DeviceSetupInfo *info = new DeviceSetupInfo(device, this, 30000);
    plugin->setupDevice(info);

    return info;
}

void DeviceManagerImplementation::postSetupDevice(Device *device)
{
    DeviceClass deviceClass = findDeviceClass(device->deviceClassId());
    DevicePlugin *plugin = m_devicePlugins.value(deviceClass.pluginId());

    plugin->postSetupDevice(device);
}

void DeviceManagerImplementation::loadDeviceStates(Device *device)
{
    NymeaSettings settings(NymeaSettings::SettingsRoleDeviceStates);
    settings.beginGroup(device->id().toString());
    DeviceClass deviceClass = m_supportedDevices.value(device->deviceClassId());
    foreach (const StateType &stateType, deviceClass.stateTypes()) {
        if (stateType.cached()) {
            QVariant value;
            // First try to load new style
            if (settings.childGroups().contains(stateType.id().toString())) {
                settings.beginGroup(stateType.id().toString());
                value = settings.value("value", stateType.defaultValue());
                value.convert(settings.value("type").toInt());
                settings.endGroup();
            } else { // Try to fall back to the pre 0.9.0 way of storing states
                value = settings.value(stateType.id().toString(), stateType.defaultValue());
            }
            device->setStateValue(stateType.id(), value);
        } else {
            device->setStateValue(stateType.id(), stateType.defaultValue());
        }
    }
    settings.endGroup();
}

void DeviceManagerImplementation::storeDeviceStates(Device *device)
{
    NymeaSettings settings(NymeaSettings::SettingsRoleDeviceStates);
    settings.beginGroup(device->id().toString());
    DeviceClass deviceClass = m_supportedDevices.value(device->deviceClassId());
    foreach (const StateType &stateType, deviceClass.stateTypes()) {
        if (stateType.cached()) {
            settings.beginGroup(stateType.id().toString());
            settings.setValue("type", static_cast<int>(device->stateValue(stateType.id()).type()));
            settings.setValue("value", device->stateValue(stateType.id()));
            settings.endGroup();
        }
    }
    settings.endGroup();
}

