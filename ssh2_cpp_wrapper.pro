TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
        ssh2_cpp_wrapper.cpp

INCLUDEPATH += D:/Workspace/libssh2
LIBS += -LD:/Workspace/libssh2 -llibssh2
LIBS += -lWs2_32
LIBS += -LD:/Workspace/OpenSSL/lib -lopenssl -llibcrypto

HEADERS += \
    ssh2_cpp_wrapper.hpp
