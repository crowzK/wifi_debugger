/*
Copyright (C) Yudoc Kim <craven@crowz.kr>
 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef TASK_HPP
#define TASK_HPP

#include <thread>
#include <atomic>

//! A wrapping class for a TASK
class Task
{
public:
    //! \brief Start TASK
    void start();

    //! \brief Stop TASK
    void stop();

    //! \brief is running task?
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
