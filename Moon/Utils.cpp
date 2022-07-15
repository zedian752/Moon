#include "Utils.h"

int Utils::getIncrement()
{
    static std::atomic<int> n(0);
    return n.fetch_add(1);
}
