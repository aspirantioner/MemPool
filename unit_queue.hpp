using namespace std;

/*存放回收的与可用的内存单元队列*/
template <typename Elem>
class UnitQueue
{
public:
    UnitQueue() { head_ = tail_ = nullptr; }                      // 初始化为空
    bool Empty() { return head_ == nullptr && tail_ == nullptr; } // 判断队列为空

    /* 插入一整段连续的内存单元 */
    void Push(Elem insert_first_elem, Elem insert_last_elem)
    {
        Elem *p = Empty() ? &head_ : &(tail_->next_ptr); // 队列是否为空决定挂载的头节点
        *p = insert_first_elem;
        tail_ = insert_last_elem;  // 再尾部指向插入片段最后一个
        tail_->next_ptr = nullptr; // 队尾元素next赋为nullptr
    }

    /*插入单个内存单元*/
    void Push(Elem insert_elem)
    {
        Elem *p = Empty() ? &head_ : &(tail_->next_ptr);
        *p = insert_elem;
        tail_ = insert_elem;
        tail_->next_ptr = nullptr;
    }

    Elem Fetch()
    {
        Elem res = head_;
        head_ = head_->next_ptr;

        /*最后一个元素取出后队列已为空*/
        if (head_ == nullptr)
        {
            tail_ = nullptr;
        }
        return res;
    }
    
    void Move(UnitQueue &que)
    {
        que.Push(head_, tail_);
        head_ = tail_ = nullptr;
    }

private:
    Elem head_; // 队列头
    Elem tail_; // 队列尾
};
