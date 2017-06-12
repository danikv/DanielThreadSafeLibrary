#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include "spsc_queue.h"
//#include "growing_spsc_queue.h"
//#include "blocking_thread_safe_queue.h"
//#include "iblocking_thread_safe_queue.h"
#include <boost/lockfree/spsc_queue.hpp>

int size = 10000000;
std::vector<int> results(size);
std::vector<int> randoms(size);


template<typename QueueType>
void writer(QueueType& queue)
{
	for (int j = 0; j < size; ++j)
	{
		int random = rand() % size;
		randoms[j] = random;
		while(!queue.push(random));
	}
	std::cout << "finished " << std::endl;
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
void writer2(boost::lockfree::spsc_queue<T,boost::lockfree::capacity<50000>>& queue)
{
	for (int j = 0; j < size; ++j)
	{
		auto random = rand() % size;
		randoms[j] = random;
		while(!queue.push(random));
	}
}

template<typename T>
void reader2(boost::lockfree::spsc_queue<T,boost::lockfree::capacity<50000>>& queue)
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
/*
template<typename T>
void reader3(IBlockingThreadSafeQueue<T>& queue)
{
        int i = 0;
        while (i < size)
        {
                queue.blockingConsumeAll([&](const int& element){
                        results[i++] = element;
                });
        }

}*/



int main()
{
	//create 2 threads
	//1 will push elements to the queue some random numbers and then print them to the console(like 10k of them).
	//the other one will get them and then write those to file.
	SpscQueue<int, 400000> queue;

	srand(time(NULL));
	std::thread t(writer<SpscQueue<int,400000>>, std::ref(queue));
	std::thread t2(reader<SpscQueue<int,400000>, int>, std::ref(queue));
	//writer<SpscQueue<int,400000>>(queue);
	//reader<decltype(queue), int>(queue);

	auto start = std::chrono::high_resolution_clock::now();

	t.join();
	t2.join();

	auto elapsed = std::chrono::high_resolution_clock::now() - start;

	long long nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
	std::cout << "time it takes for one var in nano seconds : " << (nanoseconds / size) << std::endl;
	
/*	GrowingSpscQueue<int> queue2;

	srand(time(NULL));
	std::thread t3(writer<GrowingSpscQueue<int>>, std::ref(queue2));
	std::thread t4(reader<GrowingSpscQueue<int>>, std::ref(queue2));

	auto start2 = std::chrono::high_resolution_clock::now();

	t3.join();
	t4.join();

	auto elapsed2 = std::chrono::high_resolution_clock::now() - start2;

	long long nanoseconds2 = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed2).count();

	std::cout << "time it takes for one var in nano seconds : " << (nanoseconds2 / size) << std::endl;

	for(int i = 0; i < size; ++i)
		if(results[i] != randoms[i])
			std::cout << "bad" << std::endl;


	boost::lockfree::spsc_queue<int, boost::lockfree::capacity<50000>> queue3;
	
	srand(time(NULL));
	std::thread t5(writer2<int>, std::ref(queue3));
	std::thread t6(reader2<int>, std::ref(queue3));

	auto start3 = std::chrono::high_resolution_clock::now();

	t5.join();
	t6.join();

	auto elapsed3 = std::chrono::high_resolution_clock::now() - start3;

	long long nanoseconds3 = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed3).count();
	std::cout << "time it takes for one var in nano seconds : " << (nanoseconds3 / size) << std::endl;

	for(int i = 0; i < size; ++i)
		if(results[i] != randoms[i])
			std::cout << "bad" << std::endl;


/*	BlockingThreadSafeQueue<int, SpscQueue<int, 400000>> queue4(100000);

        srand(time(NULL));
        std::thread t7(writer<BlockingThreadSafeQueue<int, SpscQueue<int,400000>>>,
			 std::ref(queue4));
        std::thread t8(reader3<int>, std::ref(queue4));

        auto start4 = std::chrono::high_resolution_clock::now();

        t7.join();
	queue4.notifyReaders();
        t8.join();

        auto elapsed4 = std::chrono::high_resolution_clock::now() - start4;

        long long nanoseconds4 = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed4).count();
        std::cout << "time it takes for one var in nano seconds : " << (nanoseconds4 / size) << std::endl;

        for(int i = 0; i < size; ++i)
                if(results[i] != randoms[i])
                        std::cout << "bad" << std::endl;

*/
}
