#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <libudev.h>

#include <tcpserver.h>
#include <QTcpSocket>

#include <websocketserver.h>
#include <QWebSocket>

#include <QApplication>
#include <QVector>
#include <QDebug>

#include<QSystemTrayIcon>
#include<QMenu>

#include <QByteArray>
#include <xcb/xcb.h>

#include <QtNetwork>
#include <QSslConfiguration>
#include <qtsingleapplication.h>
using namespace std;

// 定义要打开的VID/PID, 若无需匹配PID则定义PID为0
uint16_t VID =  0x2757;
uint16_t PID = 0;
uint16_t PID_F = 0x0112;
uint16_t PID_E = 0x0119;

struct hidraw_devinfo {
    unsigned int bustype;
    unsigned short vendor;
    unsigned short product;
};

#define HIDIOCGRAWINFO _IOR('H', 0x03, struct hidraw_devinfo)
int fds[10] = {0};
int fds_max = 0;
int router = 0;
bool isFirst = true;
pthread_t tid0;

TcpServer* server = nullptr;
WebSocketServer* webSocketServer = nullptr;
// 按整齐的格式输出到stdout
void printArr(uint8_t  *arr, int len) {
    static char output[1024];
    int i, off = 0;
    for (i = 0; i < len; i++) {

        if (i%16==0)
            off += sprintf(output+off, "\n\t");
        if (i%4==0)
            off += sprintf(output+off, " ");
        off += sprintf(output+off, "%02x ", arr[i]);
        {   // ==> characters
            static char str[20] = {0};
            sprintf(str+i%16, "%c", isprint(arr[i])?arr[i]:'.');
            if (i%16==15 || i==len-1) {
                off += sprintf(output+off, " > %s", str);
                memset(str, 0, 20);
            }
        }
    }
//    printf("%s\n\t-----\n",output);
    //Debug()<<"output="<<output;
}

// 按VID/PID打开设备, PID=0表示不匹配PID
void hid_open_dev(uint16_t  vendor_id, uint16_t  product_id)
{
    int i, fd = 0;
    char node[100];
    fds_max = 0;
    for (i = 0; i < 20; i++) {
        sprintf(node, "/dev/hidraw%d", i);
        fd = open(node, O_RDWR | O_NONBLOCK);
        qDebug()<<"fd"<<fd;
        if (fd > 0) {
            struct hidraw_devinfo rawinfo;
            ioctl(fd, HIDIOCGRAWINFO, &rawinfo);
            // &&
           // ((PID_F == rawinfo.product) || (PID_E == rawinfo.product)

            qDebug()<<"last VID "<<vendor_id<<" new VID"<<rawinfo.vendor;
            if (vendor_id  == rawinfo.vendor)//
            {
                fds[fds_max++] = fd;
                //               printf("...... found hidraw%d, opened as fd=%d\n", i, fd);
                qDebug()<<"...... found hidraw i = "<<i<<" opened as fd="<<fd<<" fds_max = "<<fds_max<<" pid ="<<hex<<rawinfo.product<<"fds[0]"<<fds[0];
            } else {
                close(fd);
                fd = 0;
            }

        }
    }

}

// HID输出一包数据
void hid_write_hidraw(uint8_t *cmd, int len)
{

    int fd = fds[0];
    int ret = write(fd, cmd, 64);
    if (ret>0){

    } else
    {
        printf("↓↓ raw[0] write fail? ret=%d.", ret);
    }
}

// HID输出一包数据(来自stdin)
void hid_write_hidraw_stdin(void)
{
    static char readbuf[1024];
    char *p = readbuf;
    uint8_t cmd[64] = {0};
    int i = 0, ret = scanf("%[^\n]", readbuf);
    getchar();
    if (ret) {
        if (strcmp(readbuf, "zhuanfa") == 0) {
            router = 1;
            printf("\t=== router mode on ===\n");
            return ;
        }
        if (strcmp(readbuf, "buzhuanfa") == 0) {
            router = 0;
            printf("\t=== router mode off ===\n");
            return ;
        }
        for (i = 0; i < 64 && p < readbuf+strlen(readbuf); i++)
            cmd[i] = strtoul(p, &p, 16);
        if ((cmd[0]&0xf0) == 0xa0 || (cmd[0]&0xf0) == 0xc0 || cmd[0] == 0x06)
        {
            hid_write_hidraw(cmd, 64);

        }


    }
}


// HID一直读取数据,并输出到stdout, (按需要转发)
void* hid_read_hidraw(void *arg)
{
    int i = (long)arg,fd = fds[i];
    qDebug() << "dev fd"<<fd;
    while(1) {
        uint8_t buf[64] = {0};
        int ret;
        usleep(1000);
        ret = read(fd, buf, 64);
        uint8_t redir[64] = {0xa1, 0x18};
//         qDebug()<<" fd = "<<fd<<" ret = "<<ret;
        if (ret > 0) {

            // printf("↑↑ raw[%d] read %d bytes:", i, ret);
             printArr(buf,ret);


            if(ret == 64)
            {

                QByteArray byte;

                for(int j= 0; j< 64; j++)
                {

                    byte[j] = buf[j];
//                    qDebug()<<"index = "<<j<<hex<<" data"<<((int)buf[4]);
                }

                if(server != nullptr)
                {
                    server->SendData(byte);

                }

                if(webSocketServer != nullptr && !server->isBoardAcitve)
                {
                    webSocketServer->SendData(byte);

                }


            }

            if (router) {
                if (buf[0] == 2) {
                    memcpy(redir+2, buf, 62);
                    hid_write_hidraw(redir, 64);

                    if (ret != 62)
                        printf("WARNING: NOT 62 bytes.\n");
                }
            }
        }
        if (errno == 5) {
            //printf("dev disconnected.\n");
        }
    }
}


void * hid_read_While(void *)
{
    while(1) {
        hid_write_hidraw_stdin();
    }
}

void device_added(struct udev_device *dev)
{

    if(isFirst){
        system("/opt/Newline/restart.sh");
        isFirst = true;
    }
    return;

    pthread_cancel(tid0);
    //    printf("Device added:%s\n",udev_device_get_devnode(dev));
    qDebug()<<"device add";
    for(int i = 0; i < sizeof(fds)/sizeof(int); i++)
    {
        int fd = fds[i];
        close(fd);
    }

    hid_open_dev(VID,PID);
    qDebug()<<"fds_max"<<fds_max<<"udev_device"<<&dev;
    if (fds_max == 0) {
        qDebug()<<"open failed\n";
    }

    if (fds_max > 0) {
        qDebug()<<"open ok: %d opened\n\n"<< fds_max;
        // === 2.write
        //touch switch to a1
        // e4/e0
        uint8_t cmd[64]={0xa1, 0x55, 0xe4, 0xff};

        hid_write_hidraw(cmd, 64);
        // ee/ef
        uint8_t cmd2[64]={0xa1, 0x55, 0xee, 0xff};

        hid_write_hidraw(cmd2, 64);

//        if(!isFirst)
//        {
            // === 3.read...
            qDebug()<<"device add red"<<"fds[0]"<<fds[0];
//            QtConcurrent::run([=]{
//               hid_read_hidraw((void*)0);

//            });
//            pthread_t tid0;
            if (fds[0])
                pthread_create(&tid0, NULL, hid_read_hidraw, (void*)0);
//            if (fds[1])
//                pthread_create(&tid1, NULL, hid_read_hidraw, (void*)1);
//        }
//        isFirst = false;
    }

}


void device_removed(struct udev_device *dev)
{
    pthread_cancel(tid0);
//    isFirst = true;
//    printf("Device removed:%s\n",udev_device_get_devnode(dev));
    qDebug()<<"device removed";
}

void *detect_usb_plug_unplug(void *)
{

    struct udev *udev;
    struct udev_monitor *mon;

    udev = udev_new();
    if(!udev)
    {
        printf("Cannot create udev context\n");
    }

    mon = udev_monitor_new_from_netlink(udev,"udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon,"hidraw",NULL);
    udev_monitor_enable_receiving(mon);

    int fd;
    fd_set fdSet;
    fd = udev_monitor_get_fd(mon);
    while (1) {
        usleep(1000000);
        FD_ZERO(&fdSet);
        FD_SET(fd,&fdSet);
        int res = select(fd+1,&fdSet,NULL,NULL,NULL);
        if( res <= 0)
            continue;
        if(FD_ISSET(fd,&fdSet))
        {
            struct udev_device *dev;
            dev = udev_monitor_receive_device(mon);

            // struct udev_list_entry * entry ;

            // udev_list_entry_foreach(entry, udev_device_get_properties_list_entry(dev)) {
            // const char* name = udev_list_entry_get_name(entry);
            // const char* value = udev_list_entry_get_value(entry);

            //qDebug()<<"name = "<< name <<" value = "<< value <<" devtype = "<< udev_device_get_devtype(dev);

            QString dev_path = QString::fromLocal8Bit(udev_device_get_devpath(dev));
            //qDebug()<< "dev path = "<< dev_path;
            if(dev && dev_path.contains("2757"))
            {
                if(udev_device_get_action(dev) && strcmp(udev_device_get_action(dev),"add") == 0)
                {
                    device_added(dev);
                }else if(udev_device_get_action(dev) && strcmp(udev_device_get_action(dev),"remove") == 0)
                {
                    device_removed(dev);
                }
                udev_device_unref(dev);
            }
        }
    }

    udev_monitor_unref(mon);
    udev_unref(udev);

}

void* frontMostAppliction(void *)
{
    while (1) {
        usleep(1000000);
        QString appName;

        xcb_connection_t* ct = xcb_connect(nullptr,nullptr);
        if(ct == nullptr)
        {
            return 0;
        }

        xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(ct)).data;
        if(screen == nullptr)
        {
            xcb_disconnect(ct);
            continue;
        }

        xcb_window_t rootWindow = screen->root;
        xcb_intern_atom_cookie_t cookie = xcb_intern_atom(ct,false,strlen("_NET_ACTIVE_WINDOW"),"_NET_ACTIVE_WINDOW");

        xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(ct,cookie,nullptr);
        if(reply == nullptr)
        {
            xcb_disconnect(ct);
            continue;
        }

        xcb_atom_t netActiveWindowAtom = reply->atom;
        free(reply);

        xcb_get_property_cookie_t cookie2 = xcb_get_property(ct,
                                                             false,
                                                             rootWindow,
                                                             netActiveWindowAtom,
                                                             XCB_ATOM_WINDOW,
                                                             0,
                                                             sizeof(xcb_window_t));
        xcb_generic_error_t* error = nullptr;
        xcb_get_property_reply_t* reply2 = xcb_get_property_reply(ct,cookie2,&error);
        if(error != nullptr || reply2 == nullptr || xcb_get_property_value_length(reply2) == 0)
        {
            xcb_disconnect(ct);
            continue;
        }

        if(xcb_get_property_value(reply2) == nullptr)
        {
            xcb_disconnect(ct);
            continue;
        }

        xcb_window_t activeWindow = *(reinterpret_cast<xcb_window_t*>(xcb_get_property_value(reply2)));
        free(reply2);

        xcb_intern_atom_cookie_t cookie3 = xcb_intern_atom(ct,false,strlen("WM_CLASS"),"WM_CLASS");
        xcb_intern_atom_reply_t* reply3 = xcb_intern_atom_reply(ct,cookie3,nullptr);

        if(reply3 == nullptr)
        {
            xcb_disconnect(ct);
            continue;
        }
        xcb_atom_t netWmNameAtom = reply3->atom;
        free(reply3);

        xcb_get_property_cookie_t cookie4 = xcb_get_property(ct,false,activeWindow,netWmNameAtom,XCB_ATOM_STRING,0,1024);
        xcb_get_property_reply_t* reply4 = xcb_get_property_reply(ct,cookie4,&error);

        if(error == nullptr && reply4 != nullptr)
        {
            QByteArray buffer = QByteArray::fromRawData(reinterpret_cast<const char*>(xcb_get_property_value(reply4)),xcb_get_property_value_length(reply4));
            appName = QString(buffer);
          // qDebug()<<"active application name "<< appName;

            if(server == nullptr)
            {
                return 0;
            }

            if(appName == "newline engage" || appName == "newline-engage")
            {

                server->setBoardActive(true);

            }else
            {
                server->setBoardActive(false);

                //qDebug()<<"active application name "<< appName;


            }
        }

        free(reply4);
        xcb_disconnect(ct);

    }


}


int main(int argc, char *argv[])
{

    if (argc == 2 && memcmp(argv[1], "vid=", 4) == 0) {
        VID = strtoul(argv[1]+4, NULL, 16);
        printf("\nhidraw read-write demo --- "
               " set to : %04x\n", VID);
    } else {
        printf("\nhidraw read-write demo --- VID default : %04x\n", VID);
    }

    QtSingleApplication a("NewlineLinuxDriver",argc, argv);
//    SingleApplication a(argc,argv);

    if(a.isRunning()){
        return EXIT_SUCCESS;
    }

    server = new TcpServer(&a);

// Test QWebSocketServer
    webSocketServer = new WebSocketServer(&a);

//    QWebSocket* webSocketClient = new QWebSocket(QStringLiteral("Engage Web Server"));

//    QSslConfiguration sslConfig;

//    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);

//    webSocketClient->setSslConfiguration(sslConfig);

//    webSocketClient->open(QUrl("ws://192.168.1.121:9001"));
//    QObject::connect(webSocketClient,&QWebSocket::binaryMessageReceived,[](const QByteArray &msg){
//        qDebug()<<"webScoket Client received  message "<<hex<<msg;
//    });


    QSystemTrayIcon systemTray ;
    systemTray.setIcon(QIcon(":/NewlineLinuxDriver.png"));

    QMenu menu;
    QAction quitAction(QString::fromLocal8Bit("Quit"),nullptr);
    menu.addAction(&quitAction);

    systemTray.setContextMenu(&menu);
    systemTray.show();

    QObject::connect(&quitAction,&QAction::triggered,&QApplication::quit);
    // app quit
    QObject::connect(&a,&QApplication::aboutToQuit,[&](){

        qDebug()<<"Newline Linux Driver Quit , Touch switch to 02" ;
        // touch switch to 02
        // e4/e0
        uint8_t cmd[64]={0xa1, 0x55, 0xe0, 0xff};

        hid_write_hidraw(cmd, 64);
        // ef/ee
        uint8_t cmd2[64]={0xa1, 0x55, 0xef, 0xff};

        hid_write_hidraw(cmd2, 64);
    });

    // === 1.open
    hid_open_dev(VID,PID);

    if (fds_max == 0) {
        qDebug()<<"open failed\n";
    }

    if (fds_max > 0) {
//        isFirst = true;
        qDebug()<<"open ok: %d opened\n\n"<< fds_max;
        // === 2.write
        //touch switch to a1
        // e4 /e0
        uint8_t cmd[64]={0xa1, 0x55, 0xe4, 0xff};

        hid_write_hidraw(cmd, 64);
        // ef/ee
        uint8_t cmd2[64]={0xa1, 0x55, 0xee, 0xff};

        hid_write_hidraw(cmd2, 64);

        // === 3.read...

//        pthread_t tid1;
        if (fds[0])
            pthread_create(&tid0, NULL, hid_read_hidraw, (void*)0);
//        if (fds[1])
//            pthread_create(&tid1, NULL, hid_read_hidraw, (void*)1);
//         pthread_create(&tid2, NULL, hid_read_While, (void*)2);

    }



    pthread_t tid3;
    pthread_create(&tid3, NULL, detect_usb_plug_unplug, (void*)3);

    pthread_t tid4;
    pthread_create(&tid4, NULL, frontMostAppliction, (void*)4);

    return a.exec();
}


