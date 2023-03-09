#include "renderview.h"
#include "ui_renderview.h"
#include <QDebug>

RenderView::RenderView(RenderView::CONFIG &config, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RenderView)
{
    ui->setupUi(this);

    mConfig = config;
}


RenderView::~RenderView()
{
//    qDebug() << QString("mConfig: name=%1").arg(mConfig.name);

    delete ui;
}
