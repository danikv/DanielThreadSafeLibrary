#pragma once

#include <functional>
#include "ithread_safe_queue_iterator.h"

template<typename T>
class ThreadSafeQueueIterator : public IThreadSafeQueueIterator<T>
{
public:

	ThreadSafeQueueIterator(const std::function<bool(void)>&, const std::function<void(void)>&,
			const std::function<const T*(void)>&);
	ThreadSafeQueueIterator(ThreadSafeQueueIterator<T>&) = delete;
	ThreadSafeQueueIterator(ThreadSafeQueueIterator<T>&&);
	~ThreadSafeQueueIterator();

	const T& operator *() const override;

	ThreadSafeQueueIterator<T>& operator ++() override;

	bool end() const override;

private:

	const std::function<bool(void)> is_end;
	const std::function<void(void)> incerease_position;
	const std::function<const T*(void)> current_element;
};

template<typename T>
ThreadSafeQueueIterator<T>::ThreadSafeQueueIterator(const std::function<bool(void)>& _is_end,
	const std::function<void(void)>& _incerease_position, const std::function<const T*(void)>& _current_element)
	: is_end(_is_end)
	, incerease_position(_incerease_position)
	, current_element(_current_element)
{
}

template<typename T>
ThreadSafeQueueIterator<T>::ThreadSafeQueueIterator(ThreadSafeQueueIterator<T>&& iter)
	: incerease_position(std::move(iter.incerease_position))
	, current_element(std::move(iter.current_element))
	, is_end(std::move(iter.is_end))
{
}

template<typename T>
ThreadSafeQueueIterator<T>::~ThreadSafeQueueIterator()
{
}

template<typename T>
const T& ThreadSafeQueueIterator<T>:: operator *() const
{
	return *current_element();
}

template<typename T>
ThreadSafeQueueIterator<T>& ThreadSafeQueueIterator<T>::operator ++()
{
	incerease_position();
	return *this;
}

template<typename T>
bool ThreadSafeQueueIterator<T>::end() const
{
	return is_end();
}
