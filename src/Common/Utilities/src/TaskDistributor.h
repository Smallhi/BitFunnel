// The MIT License (MIT)

// Copyright (c) 2016, Microsoft

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include <memory>                                   // For std::unique_ptr.
#include <mutex>                                    // std::mutex member.
#include <vector>                                   // std::vector member.

#include "BitFunnel/Utilities/ITaskDistributor.h"   // Inherits from ITaskDistributor.
#include "BitFunnel/NonCopyable.h"                  // Inherits from NonCopyable.


namespace BitFunnel
{
    class ITaskProcessor;
    class IThreadBase;
    class TaskCoordinatorThread;
    class ThreadManager;

    //*************************************************************************
    //
    // Class TaskDistributor
    //
    // Uses multiple threads to dispatch numbered tasks to a vector of objects
    // that derive from ITaskProcessor.
    //
    // One thread is started for each ITaskProcessor. The ITaskProcessors are
    // assigned task ids via ITaskProcessor::ProcessTask(). Task ids are handed
    // out starting from 0 and increasing until all ITaskProcessors are busy.
    // Each time a thread returns from ProcessTask(), a new task id will be
    // assigned until all tasks have been processed.
    //
    // Task coordinator only knows about task ids. The interpretation of the
    // work associated with a particular task id is up to the ITaskProcessor.
    //
    //*************************************************************************
    class TaskDistributor : public ITaskDistributor, NonCopyable
    {
    public:
        TaskDistributor(
            const std::vector<std::unique_ptr<ITaskProcessor>>& processors,
            size_t taskCount);

        // TaskDistributorThreads call TryAllocateTask() to get their next task
        // assignment. If there is work remaining, taskId will be set to the id
        // of the assigned task and the method will return true. If there are
        // no tasks remaining, the method will return false.
        bool TryAllocateTask(size_t& taskId);

        // Wait for all tasks to complete.
        void WaitForCompletion();

    private:
        std::vector<std::unique_ptr<ITaskProcessor>> const & m_processors;
        size_t m_taskCount;
        size_t m_nextTaskId;

        std::vector<std::unique_ptr<IThreadBase>> m_threads;
        std::unique_ptr<ThreadManager> m_threadManager;

        std::mutex m_lock;
    };
}
