QT += widgets serialport
requires(qtConfig(combobox))

TARGET = milandr_pintest
TEMPLATE = app

SOURCES += \
    connection.cpp \
    main.cpp \
    mainwindow.cpp \
    pinbutton.cpp \
    settingsdialog.cpp

HEADERS += \
    connection.h \
    mainwindow.h \
    pinbutton.h \
    settingsdialog.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

DISTFILES +=

RESOURCES += \
    res.qrc
