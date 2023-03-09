#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMenuBar>
#include <QFileDialog>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    CreateMenus();
}

MainWindow::~MainWindow()
{
    for (auto playEngine : mPlayEngines)
    {
        delete playEngine;
    }

    for (auto renderView : mRenderViews)
    {
        delete renderView;
    }

    delete ui;
}

void MainWindow::OnOpenFile()
{
    static int renderViewId = 1;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"));
    if (fileName.isEmpty())
        return;

    RenderView::CONFIG renderViewConfig = { };
    renderViewConfig.name = QString("RenderView%1").arg(renderViewId++);

    RenderView *renderView = new RenderView(renderViewConfig);
    renderView->setWindowTitle(renderViewConfig.name);
    renderView->show();
    mRenderViews.push_back(renderView);


    PlayEngine::CONFIG playEngineConfig = { };
    playEngineConfig.fileName = fileName;
    playEngineConfig.renderView = renderView;
    PlayEngine *playEngine = new PlayEngine();
    playEngine->Init(playEngineConfig);
    mPlayEngines.push_back(playEngine);




    playEngine->Play();


}

void MainWindow::CreateMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *openAction = new QAction(tr("&Open"), this);
    fileMenu->addAction(openAction);
    connect(openAction, &QAction::triggered, this, &MainWindow::OnOpenFile);

}

