
#ifndef	__EFFEKSEER_THREAD_POOL_H__
#define	__EFFEKSEER_THREAD_POOL_H__

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace Effekseer {

class ThreadPool
{
private:
	std::vector<std::thread> threads;
	std::queue<std::function<void()>> tasks;
	
	std::mutex task_mutex;
	std::condition_variable task_cv;

	std::mutex wait_mutex;
	std::condition_variable wait_cv;

	std::atomic_int	executing = 0;

	bool isShutdown = false;

	void ThreadFunc();
public:

	ThreadPool();
	virtual ~ThreadPool();

	void Initialize(int32_t threadCount);

	void PushTask(std::function<void()> task);

	void WaitAll();
};

}

#endif	// __EFFEKSEER_THREAD_POOL_H__
