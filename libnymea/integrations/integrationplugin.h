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
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INTEGRATIONPLUGIN_H
#define INTEGRATIONPLUGIN_H

#include "libnymea.h"
#include "typeutils.h"

#include "thing.h"
#include "thingdescriptor.h"
#include "pluginmetadata.h"

#include "types/thingclass.h"
#include "types/event.h"
#include "types/action.h"
#include "types/vendor.h"
#include "types/param.h"
#include "types/interface.h"
#include "types/browseraction.h"
#include "types/browseritemaction.h"

#include "hardwaremanager.h"

#include "thingdiscoveryinfo.h"
#include "thingpairinginfo.h"
#include "thingsetupinfo.h"
#include "thingactioninfo.h"
#include "browseresult.h"
#include "browseritemresult.h"
#include "browseractioninfo.h"
#include "browseritemactioninfo.h"

#include <QObject>
#include <QTranslator>
#include <QPair>
#include <QSettings>
#include <QMetaType>

class ThingManager;

class LIBNYMEA_EXPORT IntegrationPlugin: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUuid id READ pluginId)
    Q_PROPERTY(QString name READ pluginName)
    Q_PROPERTY(QString displayName READ pluginDisplayName)
    Q_PROPERTY(ParamTypes paramTypes READ configurationDescription)

public:
    IntegrationPlugin(QObject *parent = nullptr);
    virtual ~IntegrationPlugin();

    virtual void init() {}

    PluginId pluginId() const;
    QString pluginName() const;
    QString pluginDisplayName() const;
    Vendors supportedVendors() const;
    ThingClasses supportedThings() const;

    virtual void startMonitoringAutoThings();
    virtual void discoverThings(ThingDiscoveryInfo *info);

    virtual void setupThing(ThingSetupInfo *info);
    virtual void postSetupThing(Thing *thing);
    virtual void thingRemoved(Thing *thing);

    virtual void startPairing(ThingPairingInfo *info);
    virtual void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret);

    virtual void executeAction(ThingActionInfo *info);

    virtual void browseThing(BrowseResult *result);
    virtual void browserItem(BrowserItemResult *result);
    virtual void executeBrowserItem(BrowserActionInfo *info);
    virtual void executeBrowserItemAction(BrowserItemActionInfo *info);

    // Configuration
    ParamTypes configurationDescription() const;
    Thing::ThingError setConfiguration(const ParamList &configuration);
    ParamList configuration() const;
    QVariant configValue(const ParamTypeId &paramTypeId) const;
    Thing::ThingError setConfigValue(const ParamTypeId &paramTypeId, const QVariant &value);

    bool isBuiltIn() const;

signals:
    void emitEvent(const Event &event);
    void configValueChanged(const ParamTypeId &paramTypeId, const QVariant &value);
    void autoThingsAppeared(const ThingDescriptors &thingDescriptors);
    void autoThingDisappeared(const ThingId &thingId);

protected:
    Things myThings() const;
    HardwareManager *hardwareManager() const;
    QSettings *pluginStorage() const;

private:
    friend class ThingManager;
    friend class ThingManagerImplementation;


    void setMetaData(const PluginMetadata &metaData);
    void initPlugin(const PluginMetadata &metadata, ThingManager *thingManager, HardwareManager *hardwareManager);

    QPair<bool, QList<ParamType> > parseParamTypes(const QJsonArray &array) const;

    // Returns <missingFields, unknownFields>
    QPair<QStringList, QStringList> verifyFields(const QStringList &possibleFields, const QStringList &mandatoryFields, const QJsonObject &value) const;

    // load and verify enum values
    QPair<bool, Types::Unit> loadAndVerifyUnit(const QString &unitString) const;
    QPair<bool, Types::InputType> loadAndVerifyInputType(const QString &inputType) const;

    ThingManager *m_thingManager = nullptr;
    HardwareManager *m_hardwareManager = nullptr;
    QSettings *m_storage = nullptr;

    PluginMetadata m_metaData;
    ParamList m_config;
};
Q_DECLARE_INTERFACE(IntegrationPlugin, "io.nymea.IntegrationPlugin")
Q_DECLARE_METATYPE(IntegrationPlugin*)


class LIBNYMEA_EXPORT IntegrationPlugins: public QList<IntegrationPlugin*>
{
    Q_GADGET
    Q_PROPERTY(int count READ count)
public:
    IntegrationPlugins();
    IntegrationPlugins(const QList<IntegrationPlugin*> &other);
    IntegrationPlugin* findById(const PluginId &id) const;
    Q_INVOKABLE QVariant get(int index) const;
    Q_INVOKABLE void put(const QVariant &variant);
};
Q_DECLARE_METATYPE(IntegrationPlugins)

#endif // INTEGRATIONPLUGIN_H
