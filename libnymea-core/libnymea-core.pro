TEMPLATE = lib
TARGET = nymea-core

include(../nymea.pri)

QT += sql qml
INCLUDEPATH += $$top_srcdir/libnymea $$top_builddir
LIBS += -L$$top_builddir/libnymea/ -lnymea -lssl -lcrypto -lnymea-mqtt

target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

# icons for the webserver
RESOURCES += $$top_srcdir/icons.qrc \
             $$top_srcdir/data/debug-interface/debug-interface.qrc


HEADERS += nymeacore.h \
    integrations/plugininfocache.h \
    integrations/thingmanagerimplementation.h \
    integrations/translator.h \
    experiences/experiencemanager.h \
    ruleengine/ruleengine.h \
    ruleengine/rule.h \
    ruleengine/stateevaluator.h \
    ruleengine/ruleaction.h \
    ruleengine/ruleactionparam.h \
    scriptengine/script.h \
    scriptengine/scriptaction.h \
    scriptengine/scriptalarm.h \
    scriptengine/scriptengine.h \
    scriptengine/scriptevent.h \
    scriptengine/scriptstate.h \
    transportinterface.h \
    nymeaconfiguration.h \
    servermanager.h \
    servers/tcpserver.h \
    servers/mocktcpserver.h \
    servers/webserver.h \
    servers/httprequest.h \
    servers/httpreply.h \
    servers/bluetoothserver.h \
    servers/websocketserver.h \
    servers/mqttbroker.h \
    jsonrpc/jsonrpcserverimplementation.h \
    jsonrpc/jsonvalidator.h \
    jsonrpc/integrationshandler.h \
    jsonrpc/devicehandler.h \
    jsonrpc/ruleshandler.h \
    jsonrpc/actionhandler.h \
    jsonrpc/eventhandler.h \
    jsonrpc/statehandler.h \
    jsonrpc/logginghandler.h \
    jsonrpc/configurationhandler.h \
    jsonrpc/networkmanagerhandler.h \
    jsonrpc/tagshandler.h \
    jsonrpc/systemhandler.h \
    jsonrpc/scriptshandler.h \
    jsonrpc/usershandler.h \
    logging/logging.h \
    logging/logengine.h \
    logging/logfilter.h \
    logging/logentry.h \
    logging/logvaluetool.h \
    time/timemanager.h \
    networkmanager/dbus-interfaces.h \
    networkmanager/networkmanager.h \
    networkmanager/networkdevice.h \
    networkmanager/wirelessaccesspoint.h \
    networkmanager/wirelessnetworkdevice.h \
    networkmanager/networksettings.h \
    networkmanager/networkconnection.h \
    networkmanager/wirednetworkdevice.h \
    usermanager/userinfo.h \
    usermanager/usermanager.h \
    usermanager/tokeninfo.h \
    usermanager/pushbuttondbusservice.h \
    certificategenerator.h \
    cloud/awsconnector.h \
    cloud/cloudmanager.h \
    cloud/cloudnotifications.h \
    hardwaremanagerimplementation.h \
    hardware/plugintimermanagerimplementation.h \
    hardware/radio433/radio433brennenstuhl.h \
    hardware/radio433/radio433transmitter.h \
    hardware/radio433/radio433brennenstuhlgateway.h \
    hardware/bluetoothlowenergy/bluetoothlowenergymanagerimplementation.h \
    hardware/bluetoothlowenergy/bluetoothlowenergydeviceimplementation.h \
    hardware/bluetoothlowenergy/bluetoothdiscoveryreplyimplementation.h \
    hardware/network/networkaccessmanagerimpl.h \
    hardware/network/upnp/upnpdiscoveryimplementation.h \
    hardware/network/upnp/upnpdiscoveryrequest.h \
    hardware/network/upnp/upnpdiscoveryreplyimplementation.h \
    hardware/network/mqtt/mqttproviderimplementation.h \
    hardware/network/mqtt/mqttchannelimplementation.h \
    debugserverhandler.h \
    tagging/tagsstorage.h \
    tagging/tag.h \
    cloud/cloudtransport.h \
    debugreportgenerator.h \
    platform/platform.h \


SOURCES += nymeacore.cpp \
    integrations/plugininfocache.cpp \
    integrations/thingmanagerimplementation.cpp \
    integrations/translator.cpp \
    experiences/experiencemanager.cpp \
    ruleengine/ruleengine.cpp \
    ruleengine/rule.cpp \
    ruleengine/stateevaluator.cpp \
    ruleengine/ruleaction.cpp \
    ruleengine/ruleactionparam.cpp \
    scriptengine/script.cpp \
    scriptengine/scriptaction.cpp \
    scriptengine/scriptalarm.cpp \
    scriptengine/scriptengine.cpp \
    scriptengine/scriptevent.cpp \
    scriptengine/scriptstate.cpp \
    transportinterface.cpp \
    nymeaconfiguration.cpp \
    servermanager.cpp \
    servers/tcpserver.cpp \
    servers/mocktcpserver.cpp \
    servers/webserver.cpp \
    servers/httprequest.cpp \
    servers/httpreply.cpp \
    servers/websocketserver.cpp \
    servers/bluetoothserver.cpp \
    servers/mqttbroker.cpp \
    jsonrpc/jsonrpcserverimplementation.cpp \
    jsonrpc/jsonvalidator.cpp \
    jsonrpc/integrationshandler.cpp \
    jsonrpc/devicehandler.cpp \
    jsonrpc/ruleshandler.cpp \
    jsonrpc/actionhandler.cpp \
    jsonrpc/eventhandler.cpp \
    jsonrpc/statehandler.cpp \
    jsonrpc/logginghandler.cpp \
    jsonrpc/configurationhandler.cpp \
    jsonrpc/networkmanagerhandler.cpp \
    jsonrpc/tagshandler.cpp \
    jsonrpc/systemhandler.cpp \
    jsonrpc/scriptshandler.cpp \
    jsonrpc/usershandler.cpp \
    logging/logengine.cpp \
    logging/logfilter.cpp \
    logging/logentry.cpp \
    logging/logvaluetool.cpp \
    time/timemanager.cpp \
    networkmanager/networkmanager.cpp \
    networkmanager/networkdevice.cpp \
    networkmanager/wirelessaccesspoint.cpp \
    networkmanager/wirelessnetworkdevice.cpp \
    networkmanager/networksettings.cpp \
    networkmanager/networkconnection.cpp \
    networkmanager/wirednetworkdevice.cpp \
    usermanager/userinfo.cpp \
    usermanager/usermanager.cpp \
    usermanager/tokeninfo.cpp \
    usermanager/pushbuttondbusservice.cpp \
    certificategenerator.cpp \
    cloud/awsconnector.cpp \
    cloud/cloudmanager.cpp \
    cloud/cloudnotifications.cpp \
    hardwaremanagerimplementation.cpp \
    hardware/plugintimermanagerimplementation.cpp \
    hardware/radio433/radio433brennenstuhl.cpp \
    hardware/radio433/radio433transmitter.cpp \
    hardware/radio433/radio433brennenstuhlgateway.cpp \
    hardware/bluetoothlowenergy/bluetoothlowenergymanagerimplementation.cpp \
    hardware/bluetoothlowenergy/bluetoothlowenergydeviceimplementation.cpp \
    hardware/bluetoothlowenergy/bluetoothdiscoveryreplyimplementation.cpp \
    hardware/network/networkaccessmanagerimpl.cpp \
    hardware/network/upnp/upnpdiscoveryimplementation.cpp \
    hardware/network/upnp/upnpdiscoveryrequest.cpp \
    hardware/network/upnp/upnpdiscoveryreplyimplementation.cpp \
    hardware/network/mqtt/mqttproviderimplementation.cpp \
    hardware/network/mqtt/mqttchannelimplementation.cpp \
    debugserverhandler.cpp \
    tagging/tagsstorage.cpp \
    tagging/tag.cpp \
    cloud/cloudtransport.cpp \
    debugreportgenerator.cpp \
    platform/platform.cpp \


versionAtLeast(QT_VERSION, 5.12.0) {
HEADERS += \
    integrations/scriptintegrationplugin.h

SOURCES += \
    integrations/scriptintegrationplugin.cpp
}
