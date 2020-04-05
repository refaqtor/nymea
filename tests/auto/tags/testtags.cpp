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

#include "nymeatestbase.h"
#include "servers/mocktcpserver.h"
#include "tagging/tagsstorage.h"

using namespace nymeaserver;

class TestTags: public NymeaTestBase
{
    Q_OBJECT

private:
    inline void verifyTagError(const QVariant &response, TagsStorage::TagError error = TagsStorage::TagErrorNoError) {
        verifyError(response, "tagError", enumValueName(error));
    }

private slots:
    void addTag_data();
    void addTag();

    void updateTagValue();

    void removeTag();

private:
    QVariantMap createThingTag(const QString &thingId, const QString &appId, const QString &tagId, const QString &value);
    bool compareThingTag(const QVariantMap &tag, const QUuid &thingId, const QString &appId, const QString &tagId, const QString &value);
    QVariantMap createRuleTag(const QString &ruleId, const QString &appId, const QString &tagId, const QString &value);
    bool comapreRuleTag(const QVariantMap &tag, const QString &ruleId, const QString &appId, const QString &tagId, const QString &value);
};

QVariantMap TestTags::createThingTag(const QString &thingId, const QString &appId, const QString &tagId, const QString &value)
{
    QVariantMap tag;
    tag.insert("thingId", thingId);
    tag.insert("appId", appId);
    tag.insert("tagId", tagId);
    tag.insert("value", value);
    return tag;
}

QVariantMap TestTags::createRuleTag(const QString &ruleId, const QString &appId, const QString &tagId, const QString &value)
{
    QVariantMap tag;
    tag.insert("ruleId", ruleId);
    tag.insert("appId", appId);
    tag.insert("tagId", tagId);
    tag.insert("value", value);
    return tag;
}

bool TestTags::compareThingTag(const QVariantMap &tag, const QUuid &thingId, const QString &appId, const QString &tagId, const QString &value)
{
    return tag.value("thingId").toUuid() == thingId &&
            tag.value("deviceId").toUuid() == thingId && // backwards compatibility to < 0.19 adds deviceId along with thingId
            tag.value("appId").toString() == appId &&
            tag.value("tagId").toString() == tagId &&
            tag.value("value").toString() == value;
}
void TestTags::addTag_data()
{
    QTest::addColumn<ThingId>("thingId");
    QTest::addColumn<QString>("appId");
    QTest::addColumn<QString>("tagId");
    QTest::addColumn<QString>("value");
    QTest::addColumn<TagsStorage::TagError>("expectedError");

    QTest::newRow("tagThing") << m_mockThingId << "testtags" << "favorites" << "1" << TagsStorage::TagErrorNoError;
    QTest::newRow("invalidThing") << ThingId::createThingId() << "testtags" << "favorites" << "1" << TagsStorage::TagErrorThingNotFound;
}

void TestTags::addTag()
{
    QFETCH(ThingId, thingId);
    QFETCH(QString, appId);
    QFETCH(QString, tagId);
    QFETCH(QString, value);
    QFETCH(TagsStorage::TagError, expectedError);

    enableNotifications({"Tags"});

    // Setup connection to mock client
    QSignalSpy clientSpy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    // Create a tag;
    QVariantMap params;
    params.insert("tag", createThingTag(thingId.toString(), appId, tagId, value));
    QVariant response = injectAndWait("Tags.AddTag", params);
    verifyTagError(response, expectedError);

    if (expectedError != TagsStorage::TagErrorNoError) {
    // If we expected an error, we can drop out here
        return;
    }

    // Make sure the TagAdded notification is emitted.
    QVariantMap notificationTagMap = checkNotification(clientSpy, "Tags.TagAdded").toMap().value("params").toMap().value("tag").toMap();
    QJsonDocument jsonDoc = QJsonDocument::fromVariant(notificationTagMap);
    QVERIFY2(compareThingTag(notificationTagMap, thingId, appId, tagId, value), QString("Tag in notification not matching: %1").arg(qUtf8Printable(jsonDoc.toJson())).toLatin1());

    // Try getting the tag via GetTag
    params.clear();
    params.insert("thingId", thingId.toString());
    params.insert("appId", appId);
    params.insert("tagId", tagId);
    response = injectAndWait("Tags.GetTags", params);
    QVariantList tagsList = response.toMap().value("params").toMap().value("tags").toList();
    QCOMPARE(tagsList.count(), 1);
    QVERIFY2(compareThingTag(tagsList.first().toMap(), thingId, appId, tagId, value), "Fetched tag isn't matching the one we added");
}

void TestTags::updateTagValue()
{
    enableNotifications({"Tags"});

    // Setup connection to mock client
    QSignalSpy clientSpy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    QString thingId = m_mockThingId.toString();
    QString appId = "testtags";
    QString tagId = "changedNotificationTag";

    // Create a Tag
    QVariantMap params;
    params.insert("tag", createThingTag(thingId, appId, tagId, "1"));
    QVariant response = injectAndWait("Tags.AddTag", params);
    verifyTagError(response, TagsStorage::TagErrorNoError);

    // Check for TagAdded notification
    QVariantMap notificationTagMap = checkNotification(clientSpy, "Tags.TagAdded").toMap().value("params").toMap().value("tag").toMap();
    QJsonDocument jsonDoc = QJsonDocument::fromVariant(notificationTagMap);
    QVERIFY2(compareThingTag(notificationTagMap, thingId, appId, tagId, "1"), QString("Tag in notification not matching: %1").arg(qUtf8Printable(jsonDoc.toJson())).toLatin1());
    clientSpy.clear();

    // Try getting the changed tag via GetTag
    params.clear();
    params.insert("thingId", thingId);
    params.insert("appId", appId);
    params.insert("tagId", tagId);
    response = injectAndWait("Tags.GetTags", params);
    QVariantList tagsList = response.toMap().value("params").toMap().value("tags").toList();
    QCOMPARE(tagsList.count(), 1);
    QVERIFY2(compareThingTag(tagsList.first().toMap(), thingId, appId, tagId, "1"), "Fetched tag isn't matching the one we added");

    // Now update the tag
    params.clear();
    params.insert("tag", createThingTag(thingId, appId, tagId, "2"));
    response = injectAndWait("Tags.AddTag", params);
    verifyTagError(response, TagsStorage::TagErrorNoError);

    // Check for TagAdded notification
    notificationTagMap = checkNotification(clientSpy, "Tags.TagValueChanged").toMap().value("params").toMap().value("tag").toMap();
    jsonDoc = QJsonDocument::fromVariant(notificationTagMap);
    QVERIFY2(compareThingTag(notificationTagMap, thingId, appId, tagId, "2"), QString("Tag in notification not matching: %1").arg(qUtf8Printable(jsonDoc.toJson())).toLatin1());

    // Try getting the changed tag via GetTag
    params.clear();
    params.insert("thingId", thingId);
    params.insert("appId", appId);
    params.insert("tagId", tagId);
    response = injectAndWait("Tags.GetTags", params);
    tagsList = response.toMap().value("params").toMap().value("tags").toList();
    QCOMPARE(tagsList.count(), 1);
    QVERIFY2(compareThingTag(tagsList.first().toMap(), thingId, appId, tagId, "2"), "Fetched tag isn't matching the one we added");
}

void TestTags::removeTag()
{
    enableNotifications({"Tags"});

    // Setup connection to mock client
    QSignalSpy clientSpy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    QString thingId = m_mockThingId.toString();
    QString appId = "testtags";
    QString tagId = "removeTagTest";
    QString value = "1";

    // Create a Tag
    QVariantMap params;
    params.insert("tag", createThingTag(thingId, appId, tagId, value));
    QVariant response = injectAndWait("Tags.AddTag", params);
    verifyTagError(response, TagsStorage::TagErrorNoError);

    // Check for TagAdded notification
    QVariantMap notificationTagMap = checkNotification(clientSpy, "Tags.TagAdded").toMap().value("params").toMap().value("tag").toMap();
    QJsonDocument jsonDoc = QJsonDocument::fromVariant(notificationTagMap);
    QVERIFY2(compareThingTag(notificationTagMap, thingId, appId, tagId, value), QString("Tag in notification not matching: %1").arg(qUtf8Printable(jsonDoc.toJson())).toLatin1());
    clientSpy.clear();

    // Try getting the tag via GetTag
    params.clear();
    params.insert("thingId", thingId);
    params.insert("appId", appId);
    params.insert("tagId", tagId);
    response = injectAndWait("Tags.GetTags", params);
    QVariantList tagsList = response.toMap().value("params").toMap().value("tags").toList();
    QCOMPARE(tagsList.count(), 1);
    QVERIFY2(compareThingTag(tagsList.first().toMap(), thingId, appId, tagId, value), "Fetched tag isn't matching the one we added");

    // Now remove the tag
    params.clear();
    params.insert("tag", createThingTag(thingId, appId, tagId, QString()));
    response = injectAndWait("Tags.RemoveTag", params);
    verifyTagError(response, TagsStorage::TagErrorNoError);

    // Check for TagRemoved notification
    notificationTagMap = checkNotification(clientSpy, "Tags.TagRemoved").toMap().value("params").toMap().value("tag").toMap();
    jsonDoc = QJsonDocument::fromVariant(notificationTagMap);
    QVERIFY2(compareThingTag(notificationTagMap, thingId, appId, tagId, QString()), QString("Tag in notification not matching: %1").arg(qUtf8Printable(jsonDoc.toJson())).toLatin1());

    // Try getting the tag via GetTag
    params.clear();
    params.insert("thingId", thingId);
    params.insert("appId", appId);
    params.insert("tagId", tagId);
    response = injectAndWait("Tags.GetTags", params);
    tagsList = response.toMap().value("params").toMap().value("tags").toList();
    QCOMPARE(tagsList.count(), 0);
}

#include "testtags.moc"
QTEST_MAIN(TestTags)
