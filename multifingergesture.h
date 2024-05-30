#ifndef MULTIFINGERGESTURE_H
#define MULTIFINGERGESTURE_H

#include <QObject>
#include <QVector>
#include <QDebug>
#include <QtConcurrent>
#include <QTimer>

class MultiFingerGesture : public QObject
{
    Q_OBJECT
public:

     MultiFingerGesture(QObject *parent = 0);

     MultiFingerGesture(QByteArray data,int length,QObject *parent = 0);
     ~MultiFingerGesture();

     
private:

     void parseInfraredFrame();

     void singleTouchProcess();

     void twoFingerProcess();

     void threeFingerProcess();

     void fourFingerProcess();

     QPoint pointCalibProcess(int x,int y);

     int calculateTwoPointDistance(int x1,int y1,int x2,int y2);

     // mouse event
     void mouseDown(int button);
     void mouseUp(int button);
     void mouseClick(int button);
     void mouseMove(int x,int y);
     void mouseDrag(int x,int y);

     // keybord event
     void keyDown(int key);
     void keyUp(int key);
     void keyClick(int key);

     void key_Shift_Ctrl_R(); // Shift Ctrl R
     void key_Ctrl_R(); // Ctrl R

     void key_Ctrl_Minus(); // Ctrl -
     void key_Ctrl_Plus(); // Ctrl +

     void key_Alt_Left(); // Alt Left
     void key_Alt_Right(); // Alt Right

     void key_Ctrl_Alt_D(); // Ctrl Alt D
     void key_Super_D(); // Super D

     void key_Super(); // Super

     void key_Ctrl_Alt_Up(); // Ctrl Alt Up
     void key_Ctrl_Alt_Down(); // Ctrl Alt Down
     void key_Ctrl_Alt_Left(); // Ctrl Alt Left
     void key_Ctrl_Alt_Right(); // Ctrl Alt Right


public slots:
     void slotSendMsg(QByteArray data);
     void slotTimerAction();

public:

    QByteArray m_data;
    int m_length;

    bool isBoardAcitve = false;

    int touchFingerMax = 9;
    int arrayBufferMax = 19;
    int m_bufferIndex = 0;
    QVector<QVector<QVector<int>>> m_finger;

    int fingerNum = 0;
    int packetCountTenFinger = 0;
    int lastFingerNum = 0;


    QTimer *timer;
    int actionCountForTimer = 0;
    int lastACTF = 0;

    // single finger  Process
    int last1Tap = 0;
    int finger1Tap = 0;
    int last2Tap = 0;
    int finger2Tap = 0;
    bool leftButtonDownFlag = false;
    bool rightClickFlag = false;
    bool cancelLeftButton = false;
    bool cancelTwoFinger = false;
    double lastClickTime = 0;

    // two finger process
    bool twoBeginFlag = false;
    QVector<QVector<int>> tfBeginXY; // two finger, 2 value X,Y
    QVector<int> scrollPreviousX;
    QVector<int> scrollPreviousY;
    int beginTwoFingerDistance = 0;
    int deltaXBegin = 0;
    int deltaYBegin = 0;
    int absDeltaXBegin = 0;
    int absDeltaYBegin = 0;
    int scrollEventCount = 0;
    double finger2TapDownTime = 0;

    bool gestureDone = false;
    bool ctrlKeyDownFlag = false;
    bool shiftKeyDownFlag = false;
    bool altKeyDownFlag = false;

    // three finger process
    bool threeBeginFlag = false;

    QVector<QVector<int>> thfBeginXY;

    QVector<int> threeFingerDeltax;

    // four finger process
    QVector<QVector<int>> ffBeginXY;
    QVector<int> fourFingerDeltaX;
    QVector<int> fourFingerDeltaY;
    bool fourBeginFlag = false;
    
    // five finger process
    QVector<QVector<int>> fiveBeginXY;
    QVector<int> beginFiveFingerDistance;
    bool fiveBeginFlag = false;
    
    int beginAvgDis = 0;
    
    int beginFingerNum = 4;


    float m_sw;// virtual screen width
    float m_sh;// virtual screen height

};

#endif // MULTIFINGERGESTURE_H
