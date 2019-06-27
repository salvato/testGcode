#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>


#define LOG_VERBOSE


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    baudRate = QSerialPort::Baud115200;
    waitTimeout = 1000;
    bReadyToSend = false;
    bGrblConnected = false;
    responseData.clear();
    ConnectToGrbl();
}


MainWindow::~MainWindow() {
    delete ui;
}


void
MainWindow::closeEvent(QCloseEvent *event) {
    Q_UNUSED(event)
    if(serialPort.isOpen()) {
#ifdef LOG_VERBOSE
        qDebug() << QString("Closing serial port %1")
                    .arg(serialPort.portName());
#endif
        serialPort.waitForBytesWritten(1000);
        serialPort.clear();
        serialPort.close();
    }
}


void
MainWindow::ConnectToGrbl() {
    // Get a list of available serial ports
    serialPorts = QSerialPortInfo::availablePorts();
    // Remove from the list NON tty and already opened devices
    for(int i=0; i<serialPorts.count(); i++) {
        serialPortinfo = serialPorts.at(i);
        if(!serialPortinfo.portName().contains("tty"))
            serialPorts.removeAt(i);
        serialPort.setPortName(serialPortinfo.portName());
        if(serialPort.isOpen())
            serialPorts.removeAt(i);
    }
    // Do we have still serial ports ?
    if(serialPorts.isEmpty()) {
        qDebug() << QString("No serial port available");
        return;
    }
    // Yes we have serial ports available:
    // Search for the one connected to Grbl
    connect(this, SIGNAL(grblFound()),
            this, SLOT(onGrblFound()));
    connect(&grblConnectionTimer, SIGNAL(timeout()),
            this, SLOT(onGrblConnectionTimerTimeout()));

    for(currentPort=0; currentPort<serialPorts.count(); currentPort++) {
        serialPortinfo = serialPorts.at(currentPort);
        serialPort.setPortName(serialPortinfo.portName());
        serialPort.setBaudRate(baudRate);
        serialPort.setDataBits(QSerialPort::Data8);
        if(serialPort.open(QIODevice::ReadWrite)) {
#ifdef LOG_VERBOSE
            qDebug() << QString("Trying connection to %1")
                        .arg(serialPortinfo.portName());
#endif
            connect(&serialPort, SIGNAL(readyRead()),
                    this, SLOT(onSerialDataAvailable()));
            grblConnectionTimer.start(waitTimeout);
            char rst[2] = {0x18, 0};
            serialPort.write(rst);
            return;
        }
#ifdef LOG_VERBOSE
        else {
            qDebug() << QString("Unable to open %1 because %2")
                        .arg(serialPortinfo.portName())
                        .arg(serialPort.errorString());
        }
#endif
    }
    qDebug() << QString("Error: No Grbl ready to use !");
}


void
MainWindow::onGrblConnectionTimerTimeout() {
    grblConnectionTimer.stop();
    serialPort.disconnect();
    serialPort.close();

    for(++currentPort; currentPort<serialPorts.count(); currentPort++) {
        serialPortinfo = serialPorts.at(currentPort);
        serialPort.setPortName(serialPortinfo.portName());
        serialPort.setBaudRate(baudRate);
        serialPort.setDataBits(QSerialPort::Data8);
        if(serialPort.open(QIODevice::ReadWrite)) {
#ifdef LOG_VERBOSE
            qDebug() << QString("Trying connection to %1")
                       .arg(serialPortinfo.portName());
#endif
            connect(&serialPort, SIGNAL(readyRead()),
                    this, SLOT(onSerialDataAvailable()));
            grblConnectionTimer.start(waitTimeout);
            char rst[2] = {0x18, 0};
            serialPort.write(rst);
            return;
        }
#ifdef LOG_VERBOSE
        else {
            qDebug() << QString("Unable to open %1 because %2")
                       .arg(serialPortinfo.portName())
                       .arg(serialPort.errorString());
        }
#endif
    }
#ifdef LOG_VERBOSE
    qDebug() << QString("No Grbl ready to use !");
#endif
}


void
MainWindow::onSerialDataAvailable() {
    QSerialPort *testPort = static_cast<QSerialPort *>(sender());
    responseData.append(testPort->readAll());
    while(!testPort->atEnd()) {
        responseData.append(testPort->readAll());
    }
    int eol = responseData.indexOf("\r\n");
    if(eol != -1) {
        QString sString = responseData.left(eol);
        qDebug() << sString;
        responseData.remove(0, eol+2);
        if(sString.contains("ok")) {
            emit ready2Send();
            bReadyToSend = true;
        }
        bReadyToSend = false;
        if(sString.contains("error")) {
            qDebug() << "error !";
        }
        if(!bGrblConnected)
            emit(grblFound());
    }
}


void
MainWindow::onGrblFound() {
    bGrblConnected = true;
    grblConnectionTimer.stop();
}

