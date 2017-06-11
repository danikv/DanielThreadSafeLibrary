#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include "spsc_queue.h"
#include "growing_spsc_queue.h"
#include <boost/lockfree/spsc_queue.hpp>

int size = 20000000;
std::vector<int> results(size);
std::vector<int> randoms(size);


template<typename QueueType>
void writer(QueueType& queue)
{
	bool running = true;
	for (int j = 0; j < size; ++j)
	{
		int random = rand() % size;
		randoms[j] = random;
		while(running)
		{
			try
			{
				queue.push(random);
				running = false;
			}
			catch(...)
			{
			}
		}
		running = true;
	}
}

template<typename QueueType>
void reader(QueueType& queue)
{
	int i = 0;
	while (i < size)
	{
		queue.consumeAll([&](const int& element){
			results[i++] = element;
		});
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
		queue.consume_all([&](const T& element){
			results[j++] = element;
		});
	}
}



int main()
{
	//create 2 threads
	//1 will push elements to the queue some random numbers and then print them to the console(like 10k of them).
	//the other one will get them and then write those to file.
	SpscQueue<int, 400000> queue;

	srand(time(NULL));
	std::thread t(writer<SpscQueue<int,400000>>, std::ref(queue));
	std::thread t2(reader<SpscQueue<int,400000>>, std::ref(queue));

	auto start = std::chrono::high_resolution_clock::now();

	t.join();
	t2.join();

	auto elapsed = std::chrono::high_resolution_clock::now() - start;

	long long nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
	std::cout << "time it takes for one var in nano seconds : " << (nanoseconds / size) << std::endl;

	for(int i = 0; i < size; ++i)
		if(results[i] != randoms[i])
			std::cout << "bad" << std::endl;

	std::this_thread::sleep_for(std::chrono::seconds(1));

	GrowingSpscQueue<int> queue2;

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

}
