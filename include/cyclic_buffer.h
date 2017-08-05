#ifndef SPSCbuffer_H_
#define SPSCbuffer_H_

#include <atomic>
#include <boost/lockfree/detail/branch_hints.hpp>
#include <cstring>
#include <memory>
#include "queue_exceptions.h"

#define MEM_RELAXED std::memory_order_relaxed
#define MEM_ACQUIRE std::memory_order_acquire
#define MEM_RELEASE std::memory_order_release
#define CACHE_LINE_SIZE 64

using boost::lockfree::detail::unlikely;
using boost::lockfree::detail::likely;
using size_t = std::size_t;

template <class T>
class CyclicBuffer
{
public:

	CyclicBuffer(const std::size_t _buffer_size)
	: writer_position(0)
	, reader_position(0)
	, buffer_size(_buffer_size)
	{
		initalizeBuffer();
	}

	CyclicBuffer(CyclicBuffer<T>&) = delete;
	CyclicBuffer(CyclicBuffer<T>&&) = default;

	CyclicBuffer(const CyclicBuffer<T>& _buffer, const size_t _buffer_size)
	: writer_position(_buffer.buffer_size - 1)
	, reader_position(0)
	, buffer_size(_buffer_size)
	{
		buffer = new T*[buffer_size];
		const size_t reader_pos = _buffer.reader_position.load(MEM_ACQUIRE);
		const size_t read_size = _buffer.buffer_size - reader_pos;
		if(reader_pos != 0)
		{
			memcpy(buffer, &_buffer.buffer[reader_pos], sizeof(T*) * (read_size));
			memcpy(&buffer[read_size], _buffer.buffer, sizeof(T*) * (reader_pos));
		}
		else
			memcpy(buffer, _buffer.buffer, sizeof(T*) * _buffer.buffer_size);
		for(int i = _buffer.buffer_size; i < buffer_size; ++i)
			buffer[i] = new T();
	}

	~CyclicBuffer()
	{
		if(buffer != nullptr)
		{
			for (int i = 0; i < buffer_size; ++i)
				delete buffer[i];
			deleteBuffer();
		}
	}

	bool push(const T& element)
	{
		const size_t current_position = writer_position.load(MEM_RELAXED);
		const size_t next_pos = calculateNextPosition(current_position, buffer_size);
		if(!canWrite(current_position))
			return false;
		new (buffer[current_position]) T(element);
		writer_position.store(next_pos, MEM_RELEASE);
		return true;
	}

	bool push(T&& element)
	{
		const size_t current_position = writer_position.load(MEM_RELAXED);
		const size_t next_pos = calculateNextPosition(current_position, buffer_size);
		if(!canWrite(current_position))
			return false;
		new (buffer[current_position]) T(element);
		writer_position.store(next_pos, MEM_RELEASE);
		return true;
	}

	bool pop()
	{
		return consumeOne([]{});
	}

	bool tryPop(T& element)
	{
		return consumeOne([&element](T&& current){
			element = current;
		});
	}
	
	std::unique_ptr<T> tryPop()
	{
		T * returned_element = nullptr;
		consumeOne([&returned_element](T&& current){
			returned_element = new T(std::move(current));
		});
		return std::unique_ptr<T>(returned_element);
	}
	
	template <typename Functor>
	bool popOnSuccses(const Functor& function)
	{
		const size_t current_position = reader_position.load(MEM_RELAXED);
		bool result = function(*buffer[current_position]);
		const size_t next_pos = calculateNextPosition(current_position, buffer_size);
		if(!canRead(current_position) | !result)
			return false;
		increaseReaderPos(current_position, next_pos);
		return true;
	}

	template <typename Functor>
	bool consumeOne(const Functor& function)
	{
		const size_t current_position = reader_position.load(MEM_RELAXED);
		const size_t next_pos = calculateNextPosition(current_position, buffer_size);
		if(!canRead(current_position))
			return false;
		function(std::move(*buffer[current_position]));
		increaseReaderPos(current_position, next_pos);
		return true;
	}
	
	template <typename Functor>
	void consumeAll(const Functor& function)
	{
		size_t current_pos = reader_position.load(MEM_RELAXED);
		for(size_t current_size = availableRead(current_pos); current_size > 0;
				current_size = availableRead(current_pos))
		{
			current_pos = consumeSize(function, current_pos, current_size);
			reader_position.store(current_pos, MEM_RELEASE);
		}
	}
	
	//should be only used by the consumer , returns true when the queue isnt empty otherwise false
	bool canRead() const
	{
		return canRead(reader_position.load(MEM_RELAXED));
	}

	//should be only used by the producer , returns true when the queue isnt full otherwise false
	bool canWrite() const
	{
		return canWrite(writer_position.load(MEM_RELAXED));
	}
	
	
	/*
	 * those functions should not be used by the users
	 */
	void unSafePush(const T& element)
	{
		const size_t current_position = writer_position.load(MEM_RELAXED);
		const size_t next_position = calculateNextPosition(current_position, buffer_size);
		new (buffer[current_position]) T(element);
		writer_position.store(next_position, MEM_RELEASE);
	}
	
	void unSafePush(T&& element)
	{
		const size_t current_position = writer_position.load(MEM_RELAXED);
		const size_t next_position = calculateNextPosition(current_position, buffer_size);
		new (buffer[current_position]) T(element);
		writer_position.store(next_position, MEM_RELEASE);
	}
	
	void syncReader(const CyclicBuffer<T>& _buffer)
	{
		reader_position.store((_buffer.buffer_size - 1) - 
				_buffer.availableRead(_buffer.reader_position.load(MEM_RELAXED)), MEM_RELEASE);
	}

	void deleteBuffer() const
	{
		delete [] buffer;
	}

private:

	void initalizeBuffer()
	{
		buffer = new T*[buffer_size];
		for (int i = 0; i < buffer_size; ++i)
			buffer[i] = new T();
	}

	bool canRead(const size_t reader_pos) const
	{
		return likely(availableRead(reader_pos) > 0);
	}

	const size_t availableRead(const size_t reader_pos) const
	{
		const size_t writer_pos = writer_position.load(MEM_ACQUIRE);
		return availableRead(reader_pos, writer_pos, buffer_size);
	}

	bool canWrite(const size_t writer_pos) const
	{
		const size_t reader_pos = reader_position.load(MEM_ACQUIRE);
		return likely(availableRead(reader_pos, writer_pos, buffer_size) < buffer_size - 1);
	}

	template <typename Functor>
	const size_t consumeSize(const Functor& function, const size_t current_pos, const size_t consumed_size) const
	{
		const size_t end_index = current_pos + consumed_size;
		if(end_index > buffer_size)
		{
			consumeRange(function, current_pos, buffer_size);
			consumeRange(function, 0, end_index - buffer_size);
			return calculateNextPosition(end_index - buffer_size - 1, buffer_size);
		}
		else
		{
			consumeRange(function, current_pos, end_index);
			return calculateNextPosition(end_index - 1, buffer_size);
		}
	}

	template <typename Functor>
	void consumeRange(const Functor& function, const size_t start_index, const size_t end_index) const
	{
		for(int i = start_index; i < end_index; ++i)
		{
			function(std::move(*buffer[i]));
			buffer[i]->~T();
		}
	}

	static const size_t availableRead(const size_t reader_pos, const size_t writer_pos, const size_t size)
	{
		if(writer_pos >= reader_pos)
			return writer_pos - reader_pos;
		return writer_pos - reader_pos + size;
	}

	static const std::size_t calculateNextPosition(const size_t current_position, const size_t size)
	{
		size_t ret = current_position + 1;
		if(unlikely(ret == size))
			return 0;
		return ret;
	}

	void increaseReaderPos(const size_t current_position, const size_t next_position)
	{
		reader_position.store(next_position, MEM_RELEASE);
		buffer[current_position]->~T();
	}
	
	template <typename Functor>
	bool push(const Functor& function)
	{
		const size_t current_position = writer_position.load(MEM_RELAXED);
		const size_t next_pos = calculateNextPosition(current_position, buffer_size);
		if(!canWrite(current_position))
			return false;
		new (buffer[current_position]) T(function());
		writer_position.store(next_pos, MEM_RELEASE);
		return true;
	}

	T** buffer;

	static const int padding_size = CACHE_LINE_SIZE - sizeof(size_t);

	std::atomic<size_t> reader_position;
	char padding1[padding_size]; /* force reader_position and writer_position to different cache lines */
	std::atomic<size_t> writer_position;
	const size_t buffer_size;

};

#endif
