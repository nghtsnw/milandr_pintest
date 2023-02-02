#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include "connection.h"
#include <QThread>

QT_BEGIN_NAMESPACE

class QLabel;

namespace Ui {
class MainWindow;
}

QT_END_NAMESPACE

class SettingsDialog;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);    
    quint8 txBuf[4];
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
    Settings m_currentSettings;
    ~MainWindow();

signals:
    void toTxParcer(QByteArray data);
    void pinStatus(quint8 portname, quint8 pin, bool status);
    void enableButton(quint8 port_in, quint8 pin_in, bool b);
    void setButtonText(quint8 port, quint8 pin, QString text);
    void emitCommand(QVector<quint8> command, bool newcommandflag);
    void responseWaitingTimer(int time, bool start);
    void mcuMessageTimer(int time, bool start);
    void closePort();

private slots:
    void handleError(QSerialPort::SerialPortError error);    
    void slotCustomMenuRequested();
    void toPinButtonSender(QVector<quint8> snapshot);    
    void setInput();
    void setOutput1();
    void setOutput0();
    void setOutputBlink();
    void on_connectButton_clicked();
    void fillPortsInfo();
    void updateSettings();
    void openSerialPort();
    void closeSerialPort();
    void applyCustomSettings();
    void resetConnection();
    void on_portBox_activated(int index);

private:
    const int commandSize = 4;
    void setPinStatus(quint8 portname, const uint16_t *portarray);
    const QVector<quint8> pname = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
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
    Connection *backend = nullptr;
    quint8 milandrIndex = 0;
    void enableButtonsForDevice();
    bool waitOfConnection;
    QString connectionMessage;
    quint8 currentPort;
    quint8 currentPin;
    void setPix();

    int blinkPort = 999; // Стартовые значения не равные тому какими могут быть порты и пины
    int blinkPin = 999;

    QThread *thread4connection;
};

#endif // MAINWINDOW_H
