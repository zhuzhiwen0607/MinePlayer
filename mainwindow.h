#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include "renderview.h"
#include "playengine.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void OnOpenFile();

private:
    void CreateMenus();

private:
    Ui::MainWindow *ui;

//    int mRenderViewId = 1;
    QVector<RenderView*> mRenderViews;
    QVector<PlayEngine*> mPlayEngines;

};
#endif // MAINWINDOW_H
