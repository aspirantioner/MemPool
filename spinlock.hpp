#include <atomic>

using namespace std;

class SpinLock
{
public:
    SpinLock() : flag_(false) {}
    void Lock()
    {
        bool expect = false;
        while (!flag_.compare_exchange_weak(expect, true))
        {
            expect = false;
        }
    }
    void Unlock()
    {
        flag_.store(false);
    }

private:
    std::atomic<bool> flag_;
};