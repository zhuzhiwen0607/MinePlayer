#include "basetime.h"

BaseTime* BaseTime::mInstance = nullptr;

BaseTime* BaseTime::GetInstance()
{
    if (nullptr == mInstance)
    {
        mInstance = new BaseTime;
        return mInstance;
    }
    return mInstance;
}
