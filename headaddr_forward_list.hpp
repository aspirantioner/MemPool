#ifndef HEADADDR_FORWARD_LIST_H_
#define HEADADDR_FORWARD_LIST_H_

extern "C"
{
#include <stdint.h>

/*当前地址段next指向赋值*/
#define NEXTADDR_POINT(next_ptr, dst_ptr) (*((intptr_t *)next_ptr) = (intptr_t)dst_ptr)

/*得到当前地址段next指向的下一个地址段*/
#define GET_NEXTADDR(cur_ptr, next_ptr) (next_ptr = (void *)*((intptr_t *)cur_ptr))
}

using namespace std;

template <typename ElemType>
class HeadAddrForwardList
{
public:
    HeadAddrForwardList() : head_ptr_(nullptr) {}

    ~HeadAddrForwardList()
    {
        ElemType next = nullptr;
        while (head_ptr_)
        {
            GET_NEXTADDR(head_ptr_, next);
            ::operator delete(head_ptr_);
            head_ptr_ = next;
        }
    }

    /*头插插入链表中*/
    void Insert(ElemType elem)
    {
        if (!head_ptr_)
        {
            NEXTADDR_POINT(elem, head_ptr_); // 非空队列elem next_ptr 指向 head_ptr_
        }
        head_ptr_ = elem;
    }

private:
    ElemType head_ptr_; // 地址段链表头节点
};

#endif