#ifndef TASK_HPP
#define TASK_HPP

#include <thread>
#include <atomic>

class Task
{
public:
    void start();
    void stop();
    bool isRun() const { return mRun; };
    
protected:
    const char* cName;
    std::atomic<bool> mRun;
    std::thread mThreadHandle;
    
    Task(const char* cName);
    virtual ~Task();

    //! \brief Child class must implemnt this class
    virtual void task() = 0;
};

#endif 
