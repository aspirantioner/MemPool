#include "mem_pool.hpp"

#include <iostream>
#include <deque>
#include <array>

using namespace std;

int main(int argc, char *argv[])
{
    const int alloc_num = 4;
    const int alloc_times = 5;
    MemPool<int, alloc_num> pool;
    array<int *, static_cast<size_t>(alloc_times)> arr;
    int i = 0;
    for (i = 0; i < alloc_times; i++)
    {
        if (i == alloc_num)
        {
            for (int j = 0; j < alloc_num; j++)
            {
                pool.Free(arr[j]);
            }
        }
        arr[i] = pool.Malloc();
    }
    return 0;
}
