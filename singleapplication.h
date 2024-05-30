#ifndef SINGLEAPPLICATION_H
#define SINGLEAPPLICATION_H

#include <QApplication>

class QLocalServer;

class SingleApplication : public QApplication
{
    Q_OBJECT
public:

    SingleApplication(int &argc,char **argv);

    bool isRuning();

private slots:

    void newLocalConnection();

private:

    void initLocalConnection();
    void newLocalServer();

    bool bRunning;
    QLocalServer *localServer;
    QString serverName;

};

#endif // SINGLEAPPLICATION_H
