#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);

protected:
    void ConnectToGrbl();

public slots:
    void onSerialDataAvailable();
    void onGrblConnectionTimerTimeout();
    void onGrblFound();

signals:
    void grblFound();
    void ready2Send();

private:
    Ui::MainWindow *ui;

private:
    QSerialPort            serialPort;
    QSerialPortInfo        serialPortinfo;
    QList<QSerialPortInfo> serialPorts;
    QSerialPort::BaudRate  baudRate;
    int                    currentPort;
    int                    waitTimeout;
    QByteArray             responseData;
    QTimer                 grblConnectionTimer;
    bool                   bGrblConnected;
    volatile bool          bReadyToSend;
};

#endif // MAINWINDOW_H
