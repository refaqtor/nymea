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

#ifndef STATEEVALUATOR_H
#define STATEEVALUATOR_H

#include "types/state.h"
#include "types/statedescriptor.h"

#include <QDebug>

class NymeaSettings;

namespace nymeaserver {
class StateEvaluator;

class StateEvaluators: public QList<StateEvaluator>
{
    Q_GADGET
    Q_PROPERTY(int count READ count)
public:
    StateEvaluators();
    StateEvaluators(const QList<StateEvaluator> &other);
    Q_INVOKABLE QVariant get(int index) const;
    Q_INVOKABLE void put(const QVariant &variant);
};

class StateEvaluator
{
    Q_GADGET
    Q_PROPERTY(StateDescriptor stateDescriptor READ stateDescriptor WRITE setStateDescriptor USER true)
    Q_PROPERTY(nymeaserver::StateEvaluators childEvaluators READ childEvaluators WRITE setChildEvaluators USER true)
    Q_PROPERTY(Types::StateOperator operator READ operatorType WRITE setOperatorType USER true)
public:
    StateEvaluator(const StateDescriptor &stateDescriptor);
    StateEvaluator(QList<StateEvaluator> childEvaluators = QList<StateEvaluator>(), Types::StateOperator stateOperator = Types::StateOperatorAnd);

    StateDescriptor stateDescriptor() const;
    void setStateDescriptor(const StateDescriptor &stateDescriptor);

    StateEvaluators childEvaluators() const;
    void setChildEvaluators(const StateEvaluators &childEvaluators);
    void appendEvaluator(const StateEvaluator &stateEvaluator);

    Types::StateOperator operatorType() const;
    void setOperatorType(Types::StateOperator operatorType);

    bool evaluate() const;
    bool containsThing(const ThingId &thingId) const;

    void removeThing(const ThingId &thingId);
    QList<ThingId> containedThings() const;

    void dumpToSettings(NymeaSettings &settings, const QString &groupName) const;
    static StateEvaluator loadFromSettings(NymeaSettings &settings, const QString &groupPrefix);

    bool isValid() const;
    bool isEmpty() const;

private:
    StateDescriptor m_stateDescriptor;

    QList<StateEvaluator> m_childEvaluators;
    Types::StateOperator m_operatorType;
};


QDebug operator<<(QDebug dbg, const StateEvaluator &stateEvaluator);

}
Q_DECLARE_METATYPE(nymeaserver::StateEvaluator)
Q_DECLARE_METATYPE(nymeaserver::StateEvaluators)

#endif // STATEEVALUATOR_H
