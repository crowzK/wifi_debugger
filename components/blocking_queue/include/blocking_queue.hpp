#ifndef BLOCKING_QUEUE_HPP
#define BLOCKING_QUEUE_HPP

#include <iostream>
#include <assert.h>    

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

template<typename T>
class BlockingQueue
{
public:
    BlockingQueue(uint32_t capacity) :
        mutex(),
        full(), 
        empty(),
        capacity(capacity) 
    {
        
    }

    bool push(const T& data, const std::chrono::milliseconds& waitTime)
    {
        std::unique_lock<std::mutex> lock(mutex);
        while(queue.size() == capacity)
        {
            if(full.wait_for(lock, waitTime) == std::cv_status::timeout)
            {
                return false;
            }
        }

        queue.emplace(std::move(data));
        empty.notify_all();
        return true;
    }

    bool pop(T& out, const std::chrono::milliseconds& waitTime)
    {
        std::unique_lock<std::mutex> lock(mutex);
        while(queue.empty())
        {
            if(empty.wait_for(lock, waitTime) == std::cv_status::timeout)
            {
                return false;
            }
        }

        out = std::move(queue.front());
        queue.pop();
        full.notify_all();
        return true;
    }

    T front()
    {
        std::unique_lock<std::mutex> lock(mutex);
        while(queue.empty())
        {
            empty.wait(lock );
        }

        T front(std::move(queue.front()));
        return front;
    }

    T back()
    {
        std::unique_lock<std::mutex> lock(mutex);
        while(queue.empty())
        {
            empty.wait(lock );
        }

        T back(queue.back());
        return back;
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }

    bool isEmpty()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return queue.empty();
    }

    bool isFull()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return queue.size() >= capacity;
    }

private:
    mutable std::mutex mutex;
    std::condition_variable full;
    std::condition_variable empty;
    std::queue<T> queue;
    const size_t capacity; 
    
    // not copiable assignable
    BlockingQueue(const BlockingQueue& rhs);
    BlockingQueue& operator= (const BlockingQueue& rhs);
};


#endif //BLOCKING_QUEUE_HPP