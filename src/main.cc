#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include "spsc_queue.h"
#include "growing_spsc_queue.h"
//#include "mpmc_queue.h"
#include "blocking_thread_safe_queue.h"
#include "iblocking_thread_safe_queue.h"
#include <boost/lockfree/spsc_queue.hpp>
#include <mutex>

int size = 10000000;
std::vector<std::string> results(size);
std::vector<std::string> randoms(size);
std::mutex mutex;

void randomStrings()
{
	static const char alpha[] = 
	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	for(int j = 0; j < size; ++j)
	{
		char element [10];
		for(int i = 0; i < 9; ++i)
		{
			element[i] = alpha[rand() % (sizeof(alpha) - 1)];
		}
		element[9] = '\0';
		randoms[j] = element;
	}	
}

template<typename QueueType>
void writer(QueueType& queue)
{
	for (int j = 0; j < size; ++j)
		while(!queue.push(randoms[j]));
}

template<typename QueueType, typename T>
void reader(QueueType& queue)
{
	int i = 0;
	while (i < size)
	{
		static const auto function = [&](const T& element){
			results[i++] = element;
		};
		queue.ConsumeAll(function);
	}
}

template<typename T>
void writer2(boost::lockfree::spsc_queue<T,boost::lockfree::capacity<1024>>& queue)
{
	for (int j = 0; j < size; ++j)
	{
		while(!queue.push(randoms[j]));
	}
}

template<typename T>
void reader2(boost::lockfree::spsc_queue<T,boost::lockfree::capacity<1024>>& queue)
{
	int j = 0;
	while (j < size)
	{
		static const auto function = [&](const T& element){
			results[j++] = element;
		};
		queue.consume_all(function);
	}
}

template<typename T>
void reader3(IBlockingThreadSafeQueue<T>& queue)
{
	int i = 0;
	while (i < size)
	{
		static const auto function = [&](const T& element){
			results[i++] = element;
		};
		queue.blockingConsumeAll(function);
	}

}



int main()
{
	srand(time(NULL));
	/*randomStrings();

	boost::lockfree::spsc_queue<std::string, boost::lockfree::capacity<1024>> queue3;

	std::thread t3(writer2<std::string>, std::ref(queue3));
	std::thread t4(reader2<std::string>, std::ref(queue3));

	auto start3 = std::chrono::high_resolution_clock::now();

	t3.join();
	t4.join();

	auto elapsed3 = std::chrono::high_resolution_clock::now() - start3;

	long long nanoseconds3 = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed3).count();
	std::cout << "time it takes for one var in nano seconds : " << (nanoseconds3 / size) << std::endl;

	for(int i = 0; i < size; ++i)
		if(results[i] != randoms[i])
			std::cout << "bad" << std::endl;

	randomStrings();

	SpscQueue<std::string> queue(1024);

	std::thread t1(writer<decltype(queue)> , std::ref(queue));
	std::thread t2(reader<decltype(queue), std::string>, std::ref(queue));

	auto start = std::chrono::high_resolution_clock::now();

	t1.join();
	t2.join();

	auto elapsed = std::chrono::high_resolution_clock::now() - start;

	long long nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
	std::cout << "time it takes for one var in nano seconds : " << (nanoseconds / size) << std::endl;

	for(int i = 0; i < size; ++i)
		if(results[i] != randoms[i])
			std::cout << "bad" << std::endl;*/

	randomStrings();

	GrowingSpscQueue<std::string> queue2;

	std::thread t5(writer<GrowingSpscQueue<std::string>>, std::ref(queue2));
	std::thread t6(reader<GrowingSpscQueue<std::string>, std::string>, std::ref(queue2));

	auto start2 = std::chrono::high_resolution_clock::now();

	t5.join();
	t6.join();

	auto elapsed2 = std::chrono::high_resolution_clock::now() - start2;

	long long nanoseconds2 = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed2).count();

	std::cout << "time it takes for one var in nano seconds : " << (nanoseconds2 / size) << std::endl;
	int j = 0;
	for(int i = 0; i < size; ++i)
		if(results[i] != randoms[i])
			std::cout << "bad" << std::endl;
	
	
	std::cout << "capacity : " << queue2.capacity() << std::endl;
}
