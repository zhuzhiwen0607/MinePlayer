#ifndef BASETIME_H
#define BASETIME_H

#include <QTime>

class BaseTime : public QTime
{

public:
    static BaseTime* GetInstance();

    BaseTime(const BaseTime&) = delete;
    BaseTime operator =(const BaseTime&) = delete;

private:
    BaseTime() = default;
    ~BaseTime() = default;

private:
    static BaseTime *mInstance;

};



#endif // BASETIME_H
