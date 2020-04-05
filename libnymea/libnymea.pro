include(../nymea.pri)

TARGET = nymea
TEMPLATE = lib

QT += network bluetooth dbus
QT -= gui
DEFINES += LIBNYMEA_LIBRARY

QMAKE_LFLAGS += -fPIC

HEADERS += \
    integrations/browseractioninfo.h \
    integrations/browseritemactioninfo.h \
    integrations/browseritemresult.h \
    integrations/integrationplugin.h \
    integrations/pluginmetadata.h \
    integrations/browseresult.h \
    integrations/thing.h \
    integrations/thingactioninfo.h \
    integrations/thingdescriptor.h \
    integrations/thingdiscoveryinfo.h \
    integrations/thingmanager.h \
    integrations/thingpairinginfo.h \
    integrations/thingsetupinfo.h \
    integrations/thingutils.h \
    jsonrpc/jsoncontext.h \
    jsonrpc/jsonhandler.h \
    jsonrpc/jsonreply.h \
    jsonrpc/jsonrpcserver.h \
    libnymea.h \
    platform/package.h \
    platform/repository.h \
    types/browseritem.h \
    types/browseritemaction.h \
    types/browseraction.h \
    types/mediabrowseritem.h \
    types/thingclass.h \
    typeutils.h \
    loggingcategories.h \
    nymeasettings.h \
    hardware/gpio.h \
    hardware/gpiomonitor.h \
    hardware/pwm.h \
    hardware/radio433/radio433.h \
    network/upnp/upnpdiscovery.h \
    network/upnp/upnpdevice.h \
    network/upnp/upnpdevicedescriptor.h \
    network/upnp/upnpdiscoveryreply.h \
    network/networkaccessmanager.h \
    network/oauth2.h \
    network/zeroconf/zeroconfservicebrowser.h \
    network/zeroconf/zeroconfserviceentry.h \
    network/zeroconf/zeroconfservicepublisher.h \
    hardware/bluetoothlowenergy/bluetoothlowenergydevice.h \
    hardware/bluetoothlowenergy/bluetoothdiscoveryreply.h \
    hardware/bluetoothlowenergy/bluetoothlowenergymanager.h \
    coap/coap.h \
    coap/coappdu.h \
    coap/coapoption.h \
    coap/coaprequest.h \
    coap/coapreply.h \
    coap/coappdublock.h \
    coap/corelinkparser.h \
    coap/corelink.h \
    coap/coapobserveresource.h \
    types/action.h \
    types/actiontype.h \
    types/state.h \
    types/statetype.h \
    types/eventtype.h \
    types/event.h \
    types/eventdescriptor.h \
    types/vendor.h \
    types/paramtype.h \
    types/param.h \
    types/paramdescriptor.h \
    types/statedescriptor.h \
    types/interface.h \
    time/timedescriptor.h \
    time/calendaritem.h \
    time/repeatingoption.h \
    time/timeeventitem.h \
    hardwareresource.h \
    plugintimer.h \
    hardwaremanager.h \
    nymeadbusservice.h \
    network/mqtt/mqttprovider.h \
    network/mqtt/mqttchannel.h \
    platform/platformsystemcontroller.h \
    platform/platformupdatecontroller.h \
    platform/platformzeroconfcontroller.h \
    experiences/experienceplugin.h \

SOURCES += \
    integrations/browseractioninfo.cpp \
    integrations/browseritemactioninfo.cpp \
    integrations/browseritemresult.cpp \
    integrations/integrationplugin.cpp \
    integrations/pluginmetadata.cpp \
    integrations/browseresult.cpp \
    integrations/thing.cpp \
    integrations/thingactioninfo.cpp \
    integrations/thingdescriptor.cpp \
    integrations/thingdiscoveryinfo.cpp \
    integrations/thingmanager.cpp \
    integrations/thingpairinginfo.cpp \
    integrations/thingsetupinfo.cpp \
    integrations/thingutils.cpp \
    jsonrpc/jsoncontext.cpp \
    jsonrpc/jsonhandler.cpp \
    jsonrpc/jsonreply.cpp \
    jsonrpc/jsonrpcserver.cpp \
    loggingcategories.cpp \
    nymeasettings.cpp \
    platform/package.cpp \
    platform/repository.cpp \
    hardware/gpio.cpp \
    hardware/gpiomonitor.cpp \
    hardware/pwm.cpp \
    hardware/radio433/radio433.cpp \
    network/upnp/upnpdiscovery.cpp \
    network/upnp/upnpdevice.cpp \
    network/upnp/upnpdevicedescriptor.cpp \
    network/upnp/upnpdiscoveryreply.cpp \
    network/networkaccessmanager.cpp \
    network/oauth2.cpp \
    network/zeroconf/zeroconfserviceentry.cpp \
    network/zeroconf/zeroconfservicebrowser.cpp \
    network/zeroconf/zeroconfservicepublisher.cpp \
    hardware/bluetoothlowenergy/bluetoothlowenergymanager.cpp \
    hardware/bluetoothlowenergy/bluetoothlowenergydevice.cpp \
    hardware/bluetoothlowenergy/bluetoothdiscoveryreply.cpp \
    coap/coap.cpp \
    coap/coappdu.cpp \
    coap/coapoption.cpp \
    coap/coaprequest.cpp \
    coap/coapreply.cpp \
    coap/coappdublock.cpp \
    coap/corelinkparser.cpp \
    coap/corelink.cpp \
    coap/coapobserveresource.cpp \
    types/browseritem.cpp \
    types/browseritemaction.cpp \
    types/browseraction.cpp \
    types/mediabrowseritem.cpp \
    types/action.cpp \
    types/actiontype.cpp \
    types/state.cpp \
    types/statetype.cpp \
    types/eventtype.cpp \
    types/event.cpp \
    types/eventdescriptor.cpp \
    types/thingclass.cpp \
    types/vendor.cpp \
    types/paramtype.cpp \
    types/param.cpp \
    types/paramdescriptor.cpp \
    types/statedescriptor.cpp \
    types/interface.cpp \
    time/timedescriptor.cpp \
    time/calendaritem.cpp \
    time/repeatingoption.cpp \
    time/timeeventitem.cpp \
    hardwareresource.cpp \
    plugintimer.cpp \
    hardwaremanager.cpp \
    nymeadbusservice.cpp \
    network/mqtt/mqttprovider.cpp \
    network/mqtt/mqttchannel.cpp \
    platform/platformsystemcontroller.cpp \
    platform/platformupdatecontroller.cpp \
    platform/platformzeroconfcontroller.cpp \
    experiences/experienceplugin.cpp \


RESOURCES += \
    interfaces/interfaces.qrc

## Install instructions

# install plugin.pri for external plugins
pluginpri.files = integrations/plugin.pri
pluginpri.path = $$[QT_INSTALL_PREFIX]/include/nymea/
INSTALLS += pluginpri

# install header file with relative subdirectory
for(header, HEADERS) {
    path = $$[QT_INSTALL_PREFIX]/include/nymea/$${dirname(header)}
    eval(headers_$${path}.files += $${header})
    eval(headers_$${path}.path = $${path})
    eval(INSTALLS *= headers_$${path})
}

# define install target
target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

# Create pkgconfig file
CONFIG += create_pc create_prl no_install_prl
QMAKE_PKGCONFIG_NAME = libnymea
QMAKE_PKGCONFIG_DESCRIPTION = nymea development library
QMAKE_PKGCONFIG_PREFIX = $$[QT_INSTALL_PREFIX]
QMAKE_PKGCONFIG_INCDIR = $$[QT_INSTALL_PREFIX]/include/nymea/
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_VERSION = $$NYMEA_VERSION_STRING
QMAKE_PKGCONFIG_FILE = nymea
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

