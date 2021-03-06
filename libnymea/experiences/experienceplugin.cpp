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

#include "experienceplugin.h"

ExperiencePlugin::ExperiencePlugin(QObject *parent) : QObject(parent)
{

}

/*! This method will be called when the plugin has been completely loaded and experience
    logic may start operating. A plugin can reimplment this to do initialisation code. */
void ExperiencePlugin::init()
{

}

/*! Returns a pointer to the DeviceManager. The pointer won't be valid unless init() has been called. */
DeviceManager *ExperiencePlugin::deviceManager()
{
    return m_deviceManager;
}

/*! Returns a pointer to the JsonRPCServer. The pointer won't be valid unless init() has been called. */
JsonRPCServer *ExperiencePlugin::jsonRpcServer()
{
    return m_jsonRpcServer;
}


void ExperiencePlugin::initPlugin(DeviceManager *deviceManager, JsonRPCServer *jsonRPCServer)
{
    m_deviceManager = deviceManager;
    m_jsonRpcServer = jsonRPCServer;

    init();
}
