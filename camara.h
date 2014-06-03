#ifndef CAMARA_H
#define CAMARA_H

#include <QMainWindow>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QTcpServer>
#include <QFile>
#include <QDataStream>
#include <iostream>
#include <QString>

#include <cmath>
#define PI 3.14159265358979323846
#define WIDTH 100
#define HEIGHT 100


class SocketThread;
class ReadingThread;

namespace Ui {
class Camara;
}

class environment;

class Camara : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit Camara(QWidget *parent = 0);
    ~Camara();
    //void closeEvent(QCloseEvent *);

    int objNum;

signals:

private:
    class MySensor
    {
    public:
        double x, y, z, alpha, beta, zeta;
        double xc, yc, zc;
        double x_dir, y_dir, z_dir;
        double zoom;
    };

    class objInfo
    {
    public:
        int objSeq;
        int objx;
        int objy;
        int objz;
    };

    objInfo object[8];
    int stepx;
    int stepy;
    int choose;
    int newObjSeq;
    int times;

    
private slots:

    void on_connectButton_clicked();

    void on_lineEdit_textChanged(const QString &arg1);

    void on_stopButton_clicked();

    void transform(MySensor sensor, int a, int b, int c, double &screen_x, double &screen_y);

    void direction(MySensor &sensor);

    void scktReadData();

    void updateCPos(int camNum, double alpha, double beta, double zoom);

    void on_action_Quit_triggered();

    void on_action_Open_triggered();

    void changeCameraAngle(QStringList list, QString cameraFileName);

    int genObjCoor(int times);

private:
    Ui::Camara *ui;
    QTcpSocket *tcpSocket;
    QList<QList<int>> points;
    QList<MySensor> sensorList;
    QString dataFileName;
    QString camaraFileName;
    QString data;

    bool stopped;
};


#endif // CAMARA_H
