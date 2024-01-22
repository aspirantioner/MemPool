#ifndef MEM_POOL_H_
#define MEM_POOL_H_

#include "unit_queue.hpp"
#include "spinlock.hpp"
#include "headaddr_forward_list.hpp"
#include <forward_list>

#define DEFAULT_ALLOC_SIZE 128                                    // 内存池默认一次new分配单元数
#define MEM_POOL_ADDR_SIZE 8                                      // mem_pool 首地址
#define POOL_UNIT_NEXT_SIZE 8                                     // 指向下一个内存单元的指针
#define META_INFO_SIZE (MEM_POOL_ADDR_SIZE + POOL_UNIT_NEXT_SIZE) // 内存单元元数据大小(加括号防减抵消值不变)

using headaddr_type = void *;

extern "C"
{
/*根据内存单元首地址定位内存单元存储数据首地址*/
#define UNIT_FIND_DATA(unit_ptr, data_ptr, data_type) (data_ptr = (data_type *)(((char *)unit_ptr) + META_INFO_SIZE))

/*根据存储数据地址定位该内存单元地址*/
#define DATA_FIND_UNIT(data_ptr, unit_ptr) (unit_ptr = (Unit *)((char *)data_ptr - META_INFO_SIZE))

/*根据内存单元管理者定位内存池地址*/
#define REGU_FIND_POOL(regu_ptr, pool_ptr) (pool_ptr = (MemPool *)(regu_ptr))

/*根据分配连续内存单元段的首地址找到首个内存单元地址*/
#define GET_HEADUNIT_ADDR(head_addr, headunit_addr) (headunit_addr = (char *)head_addr + sizeof(headaddr_type))
}

using namespace std;

template <typename DataType, int AllocSize = DEFAULT_ALLOC_SIZE> // AllocSize:定义内存池一次分配的Unit数量
class MemPool
{
private:
    /*每次分配的连续内存段按单元进行划分*/
    struct Unit
    {
        MemPool *pool_ptr; // 内存池地址，用于free寻址
        Unit *next_ptr;    // 指向下一个内存单元的结点指针
        Unit()
        {
            pool_ptr = nullptr;
            next_ptr = nullptr;
        }
    };

    /*采用可用队列和回收队列管理内存单元,自旋锁保证线程安全*/
    class UnitRegu
    {
        friend class MemPool<DataType, AllocSize>; // 便于访问队列

    public:
        /*回收单个内存单元*/
        void Recycle(Unit *unit)
        {
            recycle_queue_lock.Lock();
            recycle_queue.Push(unit);
            recycle_queue_lock.Unlock();
        }

        /*取出单个内存单元*/
        Unit *Fetch()
        {
            availabl_queue_lock.Lock();  // 取内存单元先上锁
            if (available_queue.Empty()) // 可用内存单元队列为空
            {
                recycle_queue_lock.Lock(); // 查看回收内存单元是否存有，先上锁
                if (recycle_queue.Empty()) // 若回收对列也为空，则申请新内存段
                {
                    recycle_queue_lock.Unlock(); // 无需再占用回收队列锁
                    MemPool *pool_ptr = nullptr;
                    REGU_FIND_POOL(this, pool_ptr); // 找到MemPool首地址
                    pool_ptr->ReqMem();
                }
                else
                {
                    recycle_queue.Move(available_queue); // 回收队列所有内存单元全部放入可用内存单元中
                    recycle_queue_lock.Unlock();
                }
            }
            auto unit = available_queue.Fetch();
            availabl_queue_lock.Unlock();
            return unit;
        }

    private:
        UnitQueue<Unit *> available_queue; // 管理可用内存单元的对列
        SpinLock availabl_queue_lock;      // 可用内存单元队列锁
        UnitQueue<Unit *> recycle_queue;   // 管理回收的内存单元
        SpinLock recycle_queue_lock;       // 已回收内存单元队列锁
    };
    UnitRegu unit_regu_; // 管理回收的与可用的内存单元

    unsigned int unit_size_ = META_INFO_SIZE + sizeof(DataType); // 单个内存单元所用内存大小

    HeadAddrForwardList<void *> headaddr_regu_list_; // 管理分配的连续内存段

    /*new 新内存段*/
    void *Alloc()
    {
        void *head_addr = ::operator new((unit_size_)*AllocSize + sizeof(headaddr_type)); // 申请新内存段
        return head_addr;                                                                 // 返回新内存段首地址
    }

    /*将申请的整段内存切片划分为内存单元通过next相互连接*/
    Unit *Splice(char *head_addr)
    {
        Unit *cur;
        for (int i = 0; i < AllocSize; i++)
        {
            cur = reinterpret_cast<Unit *>(head_addr);           // 类型转换获取当前内存单元
            cur->pool_ptr = this;                                // 指向内存池,用于free寻址
            head_addr += unit_size_;                             // 下一个内存单元
            cur->next_ptr = reinterpret_cast<Unit *>(head_addr); // 指向下一个内存单元结点
        }
        cur->next_ptr = nullptr; // 尾节点下一个指向为null
        return cur;              // 返回尾部内存单元
    }

    /*申请新内存片段加入内存池中*/
    void ReqMem()
    {
        auto head_addr = Alloc(); // 申请新内存
        char *headunit_addr = nullptr;
        GET_HEADUNIT_ADDR(head_addr, headunit_addr);
        auto tail_unit = Splice(headunit_addr); // 对整段内存切片
        auto head_unit = reinterpret_cast<Unit *>(headunit_addr);
        unit_regu_.available_queue.Push(head_unit, tail_unit); // 放入可用内存单元队列中
        headaddr_regu_list_.Insert(head_addr);                 // 首地址放入分配段地址管理队列中
    }

public:
    MemPool() { ReqMem(); }

    ~MemPool() {}

    DataType *Malloc()
    {
        auto unit_ptr = unit_regu_.Fetch(); // 取出一块内存单元
        DataType *data_ptr = nullptr;
        UNIT_FIND_DATA(unit_ptr, data_ptr, DataType); // 做偏移得到存储数据首地址
        return data_ptr;
    }

    static void Free(DataType *unit_data_ptr)
    {
        MemPool *pool_ptr = nullptr;
        Unit *unit_ptr = nullptr;
        DATA_FIND_UNIT(unit_data_ptr, unit_ptr); // 根据存储数据首地址找到内存单元地址
        pool_ptr = unit_ptr->pool_ptr;
        pool_ptr->unit_regu_.Recycle(unit_ptr);
    }
};

#endif