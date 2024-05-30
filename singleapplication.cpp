#include "singleapplication.h"
#include <QLocalSocket>
#include <QLocalServer>
#include <QFileInfo>
#include <QLibrary>

SingleApplication::SingleApplication(int &argc,char **argv) : QApplication(argc,argv),bRunning(false),localServer(nullptr)
{
    serverName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    initLocalConnection();
}

bool SingleApplication::isRuning()
{
    return bRunning;
}

void SingleApplication::newLocalConnection()
{
    QLocalSocket *socket = localServer->nextPendingConnection();
    if(!socket)
        return;


    socket->waitForReadyRead(1000);
    QTextStream stream(socket);
    delete socket;

}

void SingleApplication::initLocalConnection()
{
    bRunning = false;
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if(socket.waitForConnected(500))
    {
        bRunning = true;
        QTextStream stream(&socket);
        QStringList args = QCoreApplication::arguments();

        if(args.count() > 1)

            stream << args.last();
         else
            stream << QString();
        stream.flush();
        socket.waitForBytesWritten();
    }

    newLocalServer();
}

void SingleApplication::newLocalServer()
{
    localServer = new  QLocalServer(this);
    connect(localServer,SIGNAL(newConnection()),this,SLOT(newLocalConnection()));
    if(!localServer->listen(serverName))
    {
        if(localServer->serverError() == QAbstractSocket::AddressInUseError)
        {
            QLocalServer::removeServer(serverName);
            localServer->listen(serverName);
        }
    }
}
