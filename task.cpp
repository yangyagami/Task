#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <bits/chrono.h>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <iterator>
#include <unistd.h>
#include <functional>
#include <map>
#include <stdint.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#define TASK_FOR_MEMBER(FUNC, ...) std::bind(FUNC, this, __VA_ARGS__)
#define TASK_FOR_MEMBER_NOARG(FUNC) std::bind(FUNC, this)
#define TASK(FUNC, ...) std::bind(FUNC, __VA_ARGS__)
#define TASK_NOARG(FUNC) std::bind(FUNC)

enum class TaskStatus {
	STOP,
	READY,
	RUNNING,
	FINISH
};

template <class T>
class Task {
private:
	using TaskFunction = std::function<T(void)>;
	uint64_t mID;
	TaskFunction mFunction;
	TaskStatus mStatus;
	std::mutex mStatusMutex;
	std::condition_variable cv;
	std::chrono::time_point<std::chrono::steady_clock> mBeginTime;
	T mRetValue;
private:
	void t() {
		std::unique_lock<std::mutex> lock(mStatusMutex);
		while (mStatus != TaskStatus::RUNNING) {
			cv.wait(lock);
		}

		mRetValue = mFunction();

		mStatus = TaskStatus::FINISH;
	}
public:
	Task(TaskFunction func) {
		mID = (uint64_t)this;
		mFunction = func;
		mStatus = TaskStatus::STOP;
	}
	int espladtime() {
		auto endTime = std::chrono::steady_clock::now();
		auto v = endTime - mBeginTime;
		std::chrono::milliseconds ret = std::chrono::duration_cast<std::chrono::milliseconds>(v);
		return ret.count();
	}
	bool reset() {
		bool ret = false;
		if (mStatus == TaskStatus::FINISH) {
			mStatus = TaskStatus::STOP;
			ret = true;
		}
		return ret;
	}
	bool start() {
		bool ret = false;
		if (mStatus == TaskStatus::STOP) {
			mStatus = TaskStatus::READY;
			std::thread thread(&Task::t, this);
			thread.detach();
			ret = true;
		}	
		return ret;
	}
	TaskStatus run() {
		if (mStatus == TaskStatus::READY) {
			mStatus = TaskStatus::RUNNING;
			mBeginTime = std::chrono::steady_clock::now();
			cv.notify_all();
		}
		return mStatus;
	}	
	T retValue() {
		return mRetValue;
	}
};

int f() {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	const char *port = "80";
	auto ret = getaddrinfo("sdk.nuonuo.com", port, &hints, &result);
	if (ret != 0) {
		std::cerr << "fail getaddrinfo" << std::endl;
		freeaddrinfo(result);
		return -1;
	}	
	return 0;
}

int main() {
	int index = 0;
	Task<int> task(TASK_NOARG(f));
	task.start();
	std::cout << "waitting :)  ";
	while (true) {
		std::cout << "\b" << "\\|/-"[index++];
		std::cout.flush();
		usleep(3000);
		if (index >= 4) index = 0;
		if (task.run() == TaskStatus::FINISH) {
			std::cout << std::endl << "finish! value is " << task.retValue() << " elapsedTime: " << task.espladtime() << std::endl;
			break;
		}
		if (task.espladtime() >= 1000 * 20) {
			std::cout << std::endl << "wuwuwu" << std::endl;
			break;
		}
	}
	return 0;
}
