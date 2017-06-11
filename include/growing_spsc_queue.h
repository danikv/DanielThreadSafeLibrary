#pragma once

#include "ithread_safe_queue.h"
#include "thread_safe_queue_iterator.h"
#include "queue_exceptions.h"

template<typename T>
struct node
{
	T* data;
	node<T>* next;
	node(): data(nullptr), next(nullptr) {}
	~node()
	{
		if(data != nullptr)
			delete data;
	}
};

template<typename T>
class GrowingSpscQueue : public IThreadSafeQueue<T>
{
public:
	GrowingSpscQueue()
	: end(nullptr)
	, head(nullptr)
	, current_queue_size(0)
	{
		end = new node<T>();
		head = end;
	}

	GrowingSpscQueue(GrowingSpscQueue<T>&) = delete;
	GrowingSpscQueue(GrowingSpscQueue<T>&&) = default;

	void push(const T& element) override
	{
		end->data = new T(element);
		end->next = new node<T>();
		end = end->next;
		current_queue_size++;
	}
	
	void pop() override
	{
		throwIfEmpty();
		unsafePop();
	}

	void popOnSuccses(std::function<bool(const T&)> function) override
	{
		throwIfEmpty();
		if(function(*head->data))
			unsafePop();
	}
	
	std::unique_ptr<IThreadSafeQueueIterator<T>> popElements() override
	{
		const auto current_position = [&]() {
                        return head->data;
                };
                const auto calculate_next_position = [&](){
                        unsafePop();
                };
                const auto end_function = [&]() {
                        return isEmpty();
                };
                return std::make_unique<ThreadSafeQueueIterator<T>>(end_function                        ,calculate_next_position, current_position);
	
	}

	bool isEmpty() const override
	{
		return getSize() == 0;
	}
		
private:

	const int getSize() const
	{
		return current_queue_size.load(std::memory_order_acquire);
	}
	
	void unsafePop()
	{
		head = head->next;
		current_queue_size--;
	}

	void throwIfEmpty()
	{
		if(isEmpty())
			throw QueueEmpty();
	}

	node<T> * end;
	node<T> * head;
	std::atomic<int> current_queue_size;		
};
