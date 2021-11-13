#include "MyThread.hpp"
#include <chrono>

int32_t main()
{
    ThreadPool tp(3, 10);

    for (int32_t i = 0; i < 20; i++)
    {
        std::function<void()> worker = [=]
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        };

        (void)tp.add(worker);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
