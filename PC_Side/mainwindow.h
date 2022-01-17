#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTimer>
#include "rxparcer.h"

QT_BEGIN_NAMESPACE

class QLabel;

namespace Ui {
class MainWindow;
}

QT_END_NAMESPACE

class Console;
class SettingsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    QTimer mcu_wdt;
    QTimer response_wdt;
    uint8_t txBuf[4];

    struct Settings {
        QString name;
        qint32 baudRate;
        QString stringBaudRate;
        QSerialPort::DataBits dataBits;
        QString stringDataBits;
        QSerialPort::Parity parity;
        QString stringParity;
        QSerialPort::StopBits stopBits;
        QString stringStopBits;
        QSerialPort::FlowControl flowControl;
        QString stringFlowControl;
    };

    ~MainWindow();

signals:
    void toTxParcer(QByteArray data);
    void pinStatus(uint8_t portname, uint8_t pin, bool status);
    void enableButton(uint8_t port_in, uint8_t pin_in, bool b);
    void setButtonText(uint8_t port, uint8_t pin, QString text);

private slots:
    void openSerialPort();
    void closeSerialPort();
    void writeData(const QByteArray &data);
    void readData();
    void handleError(QSerialPort::SerialPortError error);
    void sendCommand();
    void slotCustomMenuRequested();
    void toPinButtonSender(QVector<uint8_t> snapshot);
    void fillPortsInfo();
    void updateSettings();
    void applyCustomSettings();
    void resetConnection();

    void setInput();
    void setOutput1();
    void setOutput0();
    void setOutputBlink();

    void on_connectButton_clicked();

    void on_portBox_currentTextChanged(const QString &arg1);

private:
    const int commandSize = 4;
    uint8_t calcCrc(const QVector<uint8_t> &arr);
    void setPinStatus(uint8_t portname, const uint16_t *portarray);
    const QVector<uint8_t> pname = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    const QVector<QString> pnamestr = {"PA", "PB", "PC", "PD", "PE", "PF"};
    const QVector<QVector<bool>> ve91pinMap = // port, pin, enable or disable
    {{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}};
    const QVector<QVector<bool>> ve92pinMap = // port, pin, enable or disable
    {{1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,1,1,1,1,0,0,0,0},{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},{1,1,1,1,0,0,1,1,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0}};
    const QVector<QVector<bool>> ve93pinMap = // port, pin, enable or disable
    {{1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0},{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},{1,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0}};
    uint16_t *portarray = nullptr;
    void showStatusMessage(const QString &message);
    Ui::MainWindow *m_ui = nullptr;
    QLabel *m_status = nullptr;
    SettingsDialog *m_settings = nullptr;
    QSerialPort *m_serial = nullptr;
    Parcer *txparcer = nullptr;
    uint8_t milandrIndex = 0;
    void enableButtonsForDevice();
    bool waitOfConnection;
    QString connectionMessage;
    QVector<uint8_t> toTransmit;
    bool newcommand;

    uint8_t currentPort;
    uint8_t currentPin;

    int blinkPort = 999; // Стартовые значения не равные тому какими могут быть порты и пины
    int blinkPin = 999;

    Settings m_currentSettings;
};

#endif // MAINWINDOW_H
