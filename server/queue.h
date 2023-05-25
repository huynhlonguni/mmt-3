
//https://stackoverflow.com/a/36763257

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class SharedQueue {
public:
	SharedQueue() {}
	~SharedQueue() {}

	T& front() {
		std::unique_lock<std::mutex> mlock(mutex_);
		while (queue_.empty()) {
			cond_.wait(mlock);
		}
		return queue_.front();
	}

	void pop_front() {
		std::unique_lock<std::mutex> mlock(mutex_);
		while (queue_.empty()) {
			cond_.wait(mlock);
		}
		queue_.pop_front();
	}  

	void push_back(const T& item) {
		std::unique_lock<std::mutex> mlock(mutex_);
		queue_.push_back(item);
		mlock.unlock();     // unlock before notificiation to minimize mutex con
		cond_.notify_one(); // notify one waiting thread
	}

	void push_back(T&& item){
		std::unique_lock<std::mutex> mlock(mutex_);
		queue_.push_back(std::move(item));
		mlock.unlock();     // unlock before notificiation to minimize mutex con
		cond_.notify_one(); // notify one waiting thread
	}

	int size() {
		std::unique_lock<std::mutex> mlock(mutex_);
		int size = queue_.size();
		mlock.unlock();
		return size;
	}

	bool empty() {
		return size() == 0;
	}

private:
	std::deque<T> queue_;
	std::mutex mutex_;
	std::condition_variable cond_;
}; 