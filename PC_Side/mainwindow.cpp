#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "rxparcer.h"
#include <QLabel>
#include <QMessageBox>
#include "settingsdialog.h"
#include <QVector>
#include "pinbox.h"
#include "pinbutton.h"
#include <QMenu>
#include <QAction>
#include <QSerialPort>
#include <QSerialPortInfo>

static const char blankString[] = QT_TRANSLATE_NOOP("MainWindow", "N/A");

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow),
    m_status(new QLabel),
    m_settings(new SettingsDialog(this)),
    m_serial(new QSerialPort(this)),
    txparcer(new Parcer(this))
{
    m_ui->setupUi(this);
    m_ui->statusBar->addWidget(m_status);
    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(&mcu_wdt, &QTimer::timeout, this, &MainWindow::sendCommand);
    connect(&response_wdt, &QTimer::timeout, this, &MainWindow::resetConnection);
    connect(this, &MainWindow::toTxParcer, txparcer, &Parcer::getRawData);
    connect(txparcer, &Parcer::deviceData, this, &MainWindow::toPinButtonSender);
    connect(m_settings, &SettingsDialog::updateGlobalSettings, this, &MainWindow::applyCustomSettings);

    for (int x = 0, y = 0, count = 0; count < (6*16); count++, x++) //забиваем раскладку динамически
    {        
        if (x > 15)
        {
            y++;
            x = 0;
        }
        PinButton *byteBtn = new PinButton(y, x);
        connect(byteBtn, &PinButton::clicked, this, &MainWindow::slotCustomMenuRequested);
        connect(this, &MainWindow::pinStatus, byteBtn, &PinButton::setColor);
        connect(this, &MainWindow::enableButton, byteBtn, &PinButton::enableButton);
        connect(this, &MainWindow::setButtonText, byteBtn, &PinButton::setTxt);
        m_ui->dynamicButtonsLayout->addWidget(byteBtn,x,y);
        emit enableButton(y, x, false);
    }
    fillPortsInfo();
    updateSettings();
    showStatusMessage("Milandr Pin Test, Илья Кияшко, 2022");
}

MainWindow::~MainWindow()
{
    delete m_ui;
    delete m_settings;
}

void MainWindow::slotCustomMenuRequested()
{
    QObject* obj=QObject::sender();
    if (PinButton *pb=qobject_cast<PinButton *>(obj)){
    currentPort = pb->port;
    currentPin = pb->pin;
    }

    QMenu * menu = new QMenu(this);

    QAction * setInput = new QAction(("Вход с подтяжкой к питанию (по умолчанию)"), this);
    QAction * setOutput1 = new QAction(("Выход в состояние 1"), this);
    QAction * setOutput0 = new QAction(("Выход в состояние 0"), this);
    QAction * setOutputBlink = new QAction(("Выход с автопереключением состояния 1Hz"), this);

    connect(setInput, SIGNAL(triggered()), this, SLOT(setInput()));
    connect(setOutput1, SIGNAL(triggered()), this, SLOT(setOutput1()));
    connect(setOutput0, SIGNAL(triggered()), this, SLOT(setOutput0()));
    connect(setOutputBlink, SIGNAL(triggered()), this, SLOT(setOutputBlink()));

    menu->addAction(setInput);
    menu->addAction(setOutput1);
    menu->addAction(setOutput0);
    menu->addAction(setOutputBlink);

    menu->popup(QCursor::pos());
}

void MainWindow::setInput()
{
    QVector<uint8_t> command = {0xFF, 0x01, currentPort, currentPin, 0};
    toTransmit = command;
    newcommand = true;
    emit setButtonText(currentPort, currentPin, "Вход PullUp");
}

void MainWindow::setOutput1()
{
    QVector<uint8_t> command = {0xFF, 0x03, currentPort, currentPin, 0};
    toTransmit = command;
    newcommand = true;
    emit setButtonText(currentPort, currentPin, "Выход 1");
}

void MainWindow::setOutput0()
{
    QVector<uint8_t> command = {0xFF, 0x04, currentPort, currentPin, 0};
    toTransmit = command;
    newcommand = true;
    emit setButtonText(currentPort, currentPin, "Выход 0");
}

void MainWindow::setOutputBlink()
{
    if ((blinkPort != currentPort) && (blinkPin != currentPin) && (blinkPort != 999) && (blinkPin != 999))
    {
       emit setButtonText(blinkPort, blinkPin, "Выход 1Hz Стоп");
    }
    blinkPort = currentPort;
    blinkPin = currentPin;
    QVector<uint8_t> command = {0xFF, 0x05, currentPort, currentPin, 0};
    toTransmit = command;
    newcommand = true;
    emit setButtonText(currentPort, currentPin, "Выход 1Hz");
}

void MainWindow::toPinButtonSender(QVector<uint8_t> snapshot)
{
    response_wdt.start(3000);
    static uint16_t porta;
    static uint16_t portb;
    static uint16_t portc;
    static uint16_t portd;
    static uint16_t porte;
    static uint16_t portf;

    if (snapshot[0] == 0xFF && snapshot[1] == 0xFF)
    {
        if (milandrIndex != snapshot[2]){
            milandrIndex = snapshot[2];
            enableButtonsForDevice();
            showStatusMessage("Соединение установлено. " + connectionMessage);
        }
        porta = snapshot[3] + (snapshot[4] << 8);
        portb = snapshot[5] + (snapshot[6] << 8);
        portc = snapshot[7] + (snapshot[8] << 8);
        portd = snapshot[9] + (snapshot[10] << 8);
        porte = snapshot[11] + (snapshot[12] << 8);
        portf = snapshot[13] + (snapshot[14] << 8);
    }
    for (int i = 0; i < 6; i++)
    {
        switch (i)
        {
            case 0: portarray = &porta; break;
            case 1: portarray = &portb; break;
            case 2: portarray = &portc; break;
            case 3: portarray = &portd; break;
            case 4: portarray = &porte; break;
            case 5: portarray = &portf; break;
            default: portarray = 0; break;
        }
    setPinStatus(i, portarray);
    }
}

void MainWindow::setPinStatus(uint8_t portname, const uint16_t *portarray)
{
    static const uint16_t mask = 0x01;
    for (uint8_t pin = 0; pin < 16; pin++)
    {
        if (*portarray & (mask << pin))
            emit pinStatus(portname, pin, true);
        else emit pinStatus(portname, pin, false);
    }
}

void MainWindow::enableButtonsForDevice()
{
    QVector<bool> tempVector;
    for (int port_it = 0; port_it < 6; port_it++){
    switch(milandrIndex){
    case 0x01: tempVector = ve91pinMap.at(port_it); break;// ВЕ91
    case 0x02: tempVector = ve92pinMap.at(port_it); break;// ВЕ92
    case 0x03: tempVector = ve93pinMap.at(port_it); break;// ВЕ93
    default: tempVector = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    }
    for (int pin_it = 0; pin_it < 16; pin_it++)
    {
        emit enableButton(port_it, pin_it, tempVector.at(pin_it));
        if (tempVector.at(pin_it)) emit setButtonText(port_it, pin_it, "Вход PullUp");
        else emit setButtonText(port_it, pin_it, "Н/Д");
    }
    }
}

void MainWindow::openSerialPort()
{
    const MainWindow::Settings p = m_currentSettings;
    m_serial->setPortName(p.name);
    m_serial->setBaudRate(p.baudRate);
    m_serial->setDataBits(p.dataBits);
    m_serial->setParity(p.parity);
    m_serial->setStopBits(p.stopBits);
    m_serial->setFlowControl(p.flowControl);
    if (m_serial->open(QIODevice::ReadWrite))
    {
        connectionMessage = (tr("Соединено с %1 : %2, %3, %4, %5, %6")
                             .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                             .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
        showStatusMessage(connectionMessage);
        mcu_wdt.start(250); // Пинаем контроллер с этим таймером, в ответ получаем данные
        response_wdt.start(3000); // Если нет ответа от контроллера и этот таймер вышел, то считаем что нет соединения
    }
    else
    {
        QMessageBox::critical(this, tr("Ошибка"), m_serial->errorString());
        showStatusMessage(tr("Ошибка открытия"));
    }
}

void MainWindow::closeSerialPort()
{
    if (m_serial->isOpen())
        m_serial->close();
    mcu_wdt.stop();
    response_wdt.stop();
    resetConnection();
    showStatusMessage(tr("Соединение отключено"));
}

void MainWindow::resetConnection()
{
    showStatusMessage(tr("Нет ответа от микроконтроллера"));
    for (int x = 0, y = 0, count = 0; count < (6*16); count++, x++) // Блокируем кнопки в интерфейсе
    {
        if (x > 15)
        {
            y++;
            x = 0;
        }
        emit enableButton(y, x, false);
    }
    milandrIndex = 0;
}

void MainWindow::fillPortsInfo()
{
    m_ui->portBox->clear();
    QString description;
    QString manufacturer;
    QString serialNumber;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QStringList list;
        description = info.description();
        manufacturer = info.manufacturer();
        serialNumber = info.serialNumber();
        list << info.portName()
             << (!description.isEmpty() ? description : blankString)
             << (!manufacturer.isEmpty() ? manufacturer : blankString)
             << (!serialNumber.isEmpty() ? serialNumber : blankString)
             << info.systemLocation()
             << (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : blankString)
             << (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : blankString);

        m_ui->portBox->addItem(list.first(), list);
    }
    m_ui->portBox->addItem(tr("Расширенные настройки..."));
}

void MainWindow::updateSettings()
{ // по умолчанию всё задумано работать так, конечному пользователю будет проще жить
    m_currentSettings.name = m_ui->portBox->currentText();
    m_currentSettings.baudRate = 115200;
    m_currentSettings.stringBaudRate = QString::number(115200);
    m_currentSettings.dataBits = QSerialPort::Data8;
    m_currentSettings.stringDataBits = QString::number(8);
    m_currentSettings.parity = QSerialPort::NoParity;
    m_currentSettings.stringParity = "No Parity";
    m_currentSettings.stopBits = QSerialPort::OneStop;
    m_currentSettings.stringStopBits = "1";
    m_currentSettings.flowControl = QSerialPort::NoFlowControl;
    m_currentSettings.stringFlowControl = "No Flow Control";
}

void MainWindow::applyCustomSettings()
{ // для исключительного случая с одним rs485 адаптером (EL201-1) оставил кастомный режим, ему нужен parity: even
    m_currentSettings.name = m_settings->m_currentSettings.name;
    m_currentSettings.baudRate = m_settings->m_currentSettings.baudRate;
    m_currentSettings.stringBaudRate = m_settings->m_currentSettings.stringBaudRate;
    m_currentSettings.dataBits = m_settings->m_currentSettings.dataBits;
    m_currentSettings.stringDataBits = m_settings->m_currentSettings.stringDataBits;
    m_currentSettings.parity = m_settings->m_currentSettings.parity;
    m_currentSettings.stringParity = m_settings->m_currentSettings.stringParity;
    m_currentSettings.stopBits = m_settings->m_currentSettings.stopBits;
    m_currentSettings.stringStopBits = m_settings->m_currentSettings.stringStopBits;
    m_currentSettings.flowControl = m_settings->m_currentSettings.flowControl;
    m_currentSettings.stringFlowControl = m_settings->m_currentSettings.stringFlowControl;
}

void MainWindow::writeData(const QByteArray &data)
{
    m_serial->write(data);
}

void MainWindow::readData()
{
    const QByteArray data = m_serial->readAll();
    emit toTxParcer(data);
}

void MainWindow::sendCommand()
{
    if (m_serial->isOpen()){
        if (!newcommand) toTransmit = {0xFF, 0xA1, 0, 0, 0};
        toTransmit.last() = calcCrc(toTransmit);
        QByteArray ba;
        for (auto i : qAsConst(toTransmit))
            ba.append(i);
        writeData(ba);
        if (newcommand) newcommand = false;
    }
}

uint8_t MainWindow::calcCrc(const QVector<uint8_t> &arr)
{
    uint8_t crc = 0;
    for (int i = 0; i < arr.size(); i++) crc += arr[i];
    return crc;
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Критическая ошибка"), m_serial->errorString());
        closeSerialPort();
    }
}

void MainWindow::showStatusMessage(const QString &message)
{
    m_status->setText(message);
}


void MainWindow::on_connectButton_clicked()
{
    if (m_serial->isOpen()){
        closeSerialPort();
        m_ui->connectButton->setText("Подключить");
    }
    else{
        openSerialPort();
        m_ui->connectButton->setText("Отключить");
    }
}

void MainWindow::on_portBox_currentTextChanged(const QString &arg1)
{
    if (arg1 == "Расширенные настройки...")
        m_settings->show();
    else updateSettings();
}

