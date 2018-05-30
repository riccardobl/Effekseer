#include "Effekseer.ThreadPool.h"

namespace Effekseer {

void ThreadPool::ThreadFunc()
{
	while (true)
	{
		std::function<void()> task;

		{
			std::unique_lock<std::mutex> lock(task_mutex);

			task_cv.wait(lock, [this]()  -> bool { return isShutdown || !tasks.empty(); });
			if (isShutdown && this->tasks.empty()) return;

			
			task = tasks.front();
			tasks.pop();
			
		}
		
		task();

		{
			//std::lock_guard<std::mutex> lock(wait_mutex);
			executing--;

			if (executing == 0)
			{
				wait_cv.notify_one();
			}
		}
	}
}

ThreadPool::ThreadPool()
{

}

ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(task_mutex);
		isShutdown = true;
	}

	task_cv.notify_all();

	for (size_t i = 0; i < threads.size(); i++)
	{
		threads[i].join();
	}
}

void ThreadPool::Initialize(int32_t threadCount)
{
	for (size_t i = 0; i < threadCount; i++)
	{
		threads.push_back(std::thread([this]() -> void { this->ThreadFunc(); }));
	}
}


void ThreadPool::PushTask(std::function<void()> task)
{
		{
			std::lock_guard<std::mutex> lock(task_mutex);
			tasks.push(task);
			executing++;
		}

		task_cv.notify_one();
}


void ThreadPool::WaitAll()
{
	if (executing > 0)
	{
		std::unique_lock<std::mutex> lock(wait_mutex);
		
		wait_cv.wait(lock, [this]() -> bool {
			auto ret = executing == 0;
			return ret;
		});

		lock.unlock();
	}
}

}