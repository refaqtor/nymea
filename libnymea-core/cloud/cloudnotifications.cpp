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

#include "cloudnotifications.h"
#include "loggingcategories.h"
#include "devices/devicesetupinfo.h"
#include "devices/deviceactioninfo.h"

#include <QDebug>
#include <QJsonObject>

DeviceClassId cloudNotificationsDeviceClassId = DeviceClassId("81c1bbcc-543a-48fd-bd18-ab6a76f9c38d");
ParamTypeId cloudNotificationsDeviceClassUserParamId = ParamTypeId("5bdeaf08-91a9-42bc-a9f9-ef6b02ecaa3c");
ParamTypeId cloudNotificationsDeviceClassEndpointParamId = ParamTypeId("e7c41785-dd3b-4f46-b5b4-1f8a7d060ddd");

ActionTypeId notifyActionTypeId = ActionTypeId("211d1f25-28e7-4eba-8938-b29de0e41571");
ParamTypeId notifyActionParamTitleId = ParamTypeId("096503fc-b343-4d7f-8387-96162faf0f8e");
ParamTypeId notifyActionParamBodyId = ParamTypeId("4bd0fa87-c663-4621-8040-99b6d535387c");

StateTypeId connectedStateTypeId = StateTypeId("518e27b6-c3bf-49d7-be24-05ae978c00f7");

CloudNotifications::CloudNotifications(AWSConnector* awsConnector, QObject *parent):
    DevicePlugin(parent),
    m_awsConnector(awsConnector)
{
    connect(m_awsConnector, &AWSConnector::pushNotificationEndpointsUpdated, this, &CloudNotifications::pushNotificationEndpointsUpdated);
    connect(m_awsConnector, &AWSConnector::pushNotificationEndpointAdded, this, &CloudNotifications::pushNotificationEndpointAdded);
    connect(m_awsConnector, &AWSConnector::pushNotificationSent, this, &CloudNotifications::pushNotificationSent);
}

PluginMetadata CloudNotifications::metaData() const
{
    QVariantMap pluginMetaData;
    pluginMetaData.insert("id", "ccc6dbc8-e352-48a1-8e87-3c89a4669fc2");
    pluginMetaData.insert("name", "CloudNotifications");
    pluginMetaData.insert("displayName", tr("Cloud Notifications"));

    QVariantList interfaces;
    interfaces.append("notifications");
    interfaces.append("connectable");

    QVariantList createMethods;
    createMethods.append("auto");

    QVariantMap userIdParam;
    userIdParam.insert("id", cloudNotificationsDeviceClassUserParamId);
    userIdParam.insert("name", "userId");
    userIdParam.insert("displayName", tr("User ID"));
    userIdParam.insert("type", "QString");

    QVariantMap endpointIdParam;
    endpointIdParam.insert("id", cloudNotificationsDeviceClassEndpointParamId);
    endpointIdParam.insert("name", "endpoint");
    endpointIdParam.insert("displayName", tr("Device"));
    endpointIdParam.insert("type", "QString");

    QVariantList cloudNotificationDeviceClassParamTypes;
    cloudNotificationDeviceClassParamTypes.append(userIdParam);
    cloudNotificationDeviceClassParamTypes.append(endpointIdParam);

    QVariantMap notifyActionParamTitle;
    notifyActionParamTitle.insert("id", notifyActionParamTitleId);
    notifyActionParamTitle.insert("name", "title");
    notifyActionParamTitle.insert("displayName", tr("Title"));
    notifyActionParamTitle.insert("type", "QString");

    QVariantMap notifyActionParamBody;
    notifyActionParamBody.insert("id", notifyActionParamBodyId);
    notifyActionParamBody.insert("name", "body");
    notifyActionParamBody.insert("displayName", tr("Message text"));
    notifyActionParamBody.insert("type", "QString");

    QVariantList notifyActionParamTypes;
    notifyActionParamTypes.append(notifyActionParamTitle);
    notifyActionParamTypes.append(notifyActionParamBody);

    QVariantMap notifyAction;
    notifyAction.insert("id", notifyActionTypeId);
    notifyAction.insert("name", "notify");
    notifyAction.insert("displayName", tr("Send notification"));
    notifyAction.insert("paramTypes", notifyActionParamTypes);

    QVariantList actionTypes;
    actionTypes.append(notifyAction);

    QVariantMap connectedState;
    connectedState.insert("id", connectedStateTypeId);
    connectedState.insert("name", "connected");
    connectedState.insert("displayName", tr("connected"));
    connectedState.insert("type", "bool");
    connectedState.insert("displayNameEvent", tr("Connected changed"));
    connectedState.insert("defaultValue", false);

    QVariantList stateTypes;
    stateTypes.append(connectedState);


    QVariantMap cloudNotificationsDeviceClass;
    cloudNotificationsDeviceClass.insert("id", cloudNotificationsDeviceClassId);
    cloudNotificationsDeviceClass.insert("name", "cloudNotifications");
    cloudNotificationsDeviceClass.insert("displayName", tr("Cloud Notifications"));
    cloudNotificationsDeviceClass.insert("createMethods", createMethods);
    cloudNotificationsDeviceClass.insert("paramTypes", cloudNotificationDeviceClassParamTypes);
    cloudNotificationsDeviceClass.insert("interfaces", interfaces);
    cloudNotificationsDeviceClass.insert("actionTypes", actionTypes);
    cloudNotificationsDeviceClass.insert("stateTypes", stateTypes);

    QVariantList deviceClasses;
    deviceClasses.append(cloudNotificationsDeviceClass);

    QVariantMap guhVendor;
    guhVendor.insert("id", "2062d64d-3232-433c-88bc-0d33c0ba2ba6"); // nymea's id
    guhVendor.insert("name", "nymea");
    guhVendor.insert("displayName", "nymea");
    guhVendor.insert("deviceClasses", deviceClasses);

    QVariantList vendors;
    vendors.append(guhVendor);
    pluginMetaData.insert("vendors", vendors);

    return PluginMetadata(QJsonObject::fromVariantMap(pluginMetaData), true);
}

void CloudNotifications::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    device->setStateValue(connectedStateTypeId, m_awsConnector->isConnected());
    qCDebug(dcCloud) << "Cloud Notifications Device setup:" << device->name() << "Connected:" << m_awsConnector->isConnected();
    connect(m_awsConnector, &AWSConnector::connected, info->device(), [device]() {
        device->setStateValue(connectedStateTypeId, true);
    });
    connect(m_awsConnector, &AWSConnector::disconnected, device, [device]() {
        device->setStateValue(connectedStateTypeId, false);
    });
    info->finish(Device::DeviceErrorNoError);
}

void CloudNotifications::startMonitoringAutoDevices()
{
}

void CloudNotifications::executeAction(DeviceActionInfo *info)
{
    qCDebug(dcCloud()) << "executeAction" << info->device() << info->action().id() << info->action().params();
    QString userId = info->device()->paramValue(cloudNotificationsDeviceClassUserParamId).toString();
    QString endpointId = info->device()->paramValue(cloudNotificationsDeviceClassEndpointParamId).toString();
    int id = m_awsConnector->sendPushNotification(userId, endpointId, info->action().param(notifyActionParamTitleId).value().toString(), info->action().param(notifyActionParamBodyId).value().toString());
    m_pendingPushNotifications.insert(id, info);
}

void CloudNotifications::pushNotificationEndpointsUpdated(const QList<AWSConnector::PushNotificationsEndpoint> &endpoints)
{
    qCDebug(dcCloud()) << "Push Notification endpoint update";
    QList<Device*> devicesToRemove;
    foreach (Device *configuredDevice, myDevices()) {
        bool found = false;
        foreach (const AWSConnector::PushNotificationsEndpoint &ep, endpoints) {
            if (configuredDevice->paramValue(cloudNotificationsDeviceClassUserParamId).toString() == ep.userId
                    && configuredDevice->paramValue(cloudNotificationsDeviceClassEndpointParamId).toString() == ep.endpointId) {
                found = true;
                break;
            }
        }
        if (!found) {
            devicesToRemove.append(configuredDevice);
        }
    }
    foreach (Device *d, devicesToRemove) {
        emit autoDeviceDisappeared(d->id());
    }

    DeviceDescriptors devicesToAdd;
    foreach (const AWSConnector::PushNotificationsEndpoint &ep, endpoints) {
        bool found = false;
        qCDebug(dcCloud) << "Checking endoint:" << ep.endpointId;
        foreach (Device *d, myDevices()) {
            qCDebug(dcCloud) << "Have existing device:" << d->name() << d->paramValue(cloudNotificationsDeviceClassEndpointParamId);
            if (d->paramValue(cloudNotificationsDeviceClassUserParamId).toString() == ep.userId
                    && d->paramValue(cloudNotificationsDeviceClassEndpointParamId).toString() == ep.endpointId) {
                found = true;
                break;
            }
        }
        if (!found) {
            qCDebug(dcCloud) << "Adding new notification device" << ep.displayName;
            DeviceDescriptor descriptor(cloudNotificationsDeviceClassId, ep.displayName, QString("Send notifications to %1").arg(ep.displayName));
            ParamList params;
            Param userIdParam(cloudNotificationsDeviceClassUserParamId, ep.userId);
            params.append(userIdParam);
            Param endpointIdParam(cloudNotificationsDeviceClassEndpointParamId, ep.endpointId);
            params.append(endpointIdParam);
            descriptor.setParams(params);
            devicesToAdd.append(descriptor);
        }
    }
    emit autoDevicesAppeared(devicesToAdd);

}

void CloudNotifications::pushNotificationEndpointAdded(const AWSConnector::PushNotificationsEndpoint &endpoint)
{
    // Could be just an update, don't add it in that case...
    foreach (Device *d, myDevices()) {
        if (d->paramValue(cloudNotificationsDeviceClassUserParamId).toString() == endpoint.userId
                && d->paramValue(cloudNotificationsDeviceClassEndpointParamId).toString() == endpoint.endpointId) {
            return;
        }
    }
    qCDebug(dcCloud) << "Push notification endpoint added:" << endpoint.displayName;
    DeviceDescriptor descriptor(cloudNotificationsDeviceClassId, endpoint.displayName, QString("Send notifications to %1").arg(endpoint.displayName));
    ParamList params;
    Param userIdParam(cloudNotificationsDeviceClassUserParamId, endpoint.userId);
    params.append(userIdParam);
    Param endpointIdParam(cloudNotificationsDeviceClassEndpointParamId, endpoint.endpointId);
    params.append(endpointIdParam);
    descriptor.setParams(params);
    emit autoDevicesAppeared({descriptor});
}

void CloudNotifications::pushNotificationSent(int id, int status)
{
    qCDebug(dcCloud()) << "Push notification sent" << id << status;
    DeviceActionInfo *info = m_pendingPushNotifications.take(id);
    if (!info) {
        qCWarning(dcCloud()) << "Received a push notification send reponse for a request we're not waiting for.";
        return;
    }
    info->finish(status == 200 ? Device::DeviceErrorNoError : Device::DeviceErrorHardwareNotAvailable);
}
