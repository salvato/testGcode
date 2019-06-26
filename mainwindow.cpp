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
            serialPort.clear();
            connect(&serialPort, SIGNAL(readyRead()),
                    this, SLOT(onSerialDataAvailable()));
            grblConnectionTimer.start(waitTimeout);
            serialPort.write("\n");
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
            serialPort.clear();
            connect(&serialPort, SIGNAL(readyRead()),
                    this, SLOT(onSerialDataAvailable()));
            grblConnectionTimer.start(waitTimeout);
            serialPort.write("\n");
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
    qDebug() << responseData;
    grblConnectionTimer.stop();
}


void
MainWindow::onGrblFound() {
    // The event is handled in the derived classes
}

