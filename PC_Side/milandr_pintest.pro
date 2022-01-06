QT += widgets serialport
requires(qtConfig(combobox))

TARGET = milandr_pintest
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    pinbutton.cpp \
    rxparcer.cpp \
    settingsdialog.cpp

HEADERS += \
    mainwindow.h \
    pinbutton.h \
    rxparcer.h \
    settingsdialog.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

target.path = $$[QT_INSTALL_EXAMPLES]/serialport/terminal
INSTALLS += target
