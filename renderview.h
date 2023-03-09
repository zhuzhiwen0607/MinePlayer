#ifndef RENDERVIEW_H
#define RENDERVIEW_H

#include <QWidget>


namespace Ui {
class RenderView;
}

class RenderView : public QWidget
{
    Q_OBJECT

public:
    typedef struct
    {
        QString name;
        qint32 width;
        qint32 height;
    } CONFIG;

public:
    explicit RenderView(CONFIG &config, QWidget *parent = nullptr);

    ~RenderView();



private:
    Ui::RenderView *ui;

    CONFIG mConfig;
};

#endif // RENDERVIEW_H
