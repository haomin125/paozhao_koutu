#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "safe_queue.h"

class ThreadPool {
public:
	ThreadPool(const int n_threads);
	ThreadPool(const ThreadPool &) = delete;
	ThreadPool(ThreadPool &&) = delete;

	ThreadPool & operator=(const ThreadPool &) = delete;
	ThreadPool & operator=(ThreadPool &&) = delete;

	// Inits thread pool
	void init();

	// Waits until threads finish their current task and shutdowns the pool
	void shutdown();

	// Submit a function to be executed asynchronously by the pool
	template<typename F, typename...Args>
	auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
		// Create a function with bounded parameters ready to execute
		std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		// Encapsulate it into a shared ptr in order to be able to copy construct / assign 
		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

		// Wrap packaged task into void function
		std::function<void()> wrapper_func = [task_ptr]() {
			(*task_ptr)();
		};

		// Enqueue generic wrapper function, and wake up one thread if its waiting
		std::unique_lock<std::mutex> lock(m_mutex);

		m_queue.enqueue(wrapper_func);
		m_conditional.notify_one();

		lock.unlock();

		// Return future from promise
		return task_ptr->get_future();
	}
private:
	class ThreadWorker {
	public:
		ThreadWorker(ThreadPool *pool, const int id);

		void operator()();
	private:
		int m_iId;
		ThreadPool *m_pThreadPool;
	};

	bool m_shutdown;
	SafeQueue<std::function<void()>> m_queue;
	std::vector<std::thread> m_threads;
	std::mutex m_mutex;
	std::condition_variable m_conditional;
};

#endif // THREAD_POOL_H
