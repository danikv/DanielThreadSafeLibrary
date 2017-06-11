#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include "spsc_queue.h"
#include "growing_spsc_queue.h"

int size = 200000;
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
		std::cout << "j : " << j << std::endl;
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

/*template<typename T>
void writer2(ThreadSafeQueue<T>& queue)
{
	for (int j = 0; j < size; ++j)
	{
		auto random = rand() % size;
		queue.push(random);
	}
}

template<typename T>
void reader(ThreadSafeQueue<T>& queue)
{
	while (true)
	{
		queue.blockUntilSizeReached();
		if (!queue.isEmpty())
			const auto element(std::move(queue.pop()));
		else
			return;
	}
}*/

template<typename QueueType>
void reader2(QueueType& queue)
{
	int i = 0;
	while (i < size)
	{
		auto iterator = queue.popElements();
		while (!iterator->end())
		{
			const auto& element = **iterator;
			results[i++] = element;
			++(*iterator);
		}
	}
}

int main()
{
	//create 2 threads
	//1 will push elements to the queue some random numbers and then print them to the console(like 10k of them).
	//the other one will get them and then write those to file.
	SpsqQueue<int, 200000> queue;

	srand(time(NULL));

	auto start = std::chrono::high_resolution_clock::now();

	std::thread t(writer<SpsqQueue<int, 200000>> , std::ref(queue));
        std::thread t1(reader2<SpsqQueue<int, 200000>>, std::ref(queue));

	t.join();
	t1.join();	

	auto elapsed = std::chrono::high_resolution_clock::now() - start;

	long long nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
	
	std::cout << "time it takes for one var in nano seconds : " << (nanoseconds / size) << std::endl;

	GrowingSpscQueue<int> queue2;
	
	srand(time(NULL));
	std::thread t3(writer<GrowingSpscQueue<int>>, std::ref(queue2));
	std::thread t4(reader2<GrowingSpscQueue<int>>, std::ref(queue2));

	auto start2 = std::chrono::high_resolution_clock::now();

	t3.join();
	t4.join();

	auto elapsed2 = std::chrono::high_resolution_clock::now() - start2;

	long long nanoseconds2 = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed2).count();
	std::cout << "time it takes for one var in nano seconds : " << (nanoseconds2 / size) << std::endl;
	std::fstream file("daniel.txt", std::ios::out);
	for(int i = 0; i < size; ++i)
		file << "pushed : " << randoms[i] << " received : " << results[i] << std::endl;
}
