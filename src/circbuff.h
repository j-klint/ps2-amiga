#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>

template<typename T, uint8_t N>
class CircularBuffer
{
private:
	volatile T buf[N];
	volatile uint8_t front, sz;

public:
	CircularBuffer()
	{
		clear();
	}

	void clear()
	{
		sz = front = 0;
	}

	void push(T val)
	{
		if ( sz != N )
		{
			buf[(front + sz) % N] = val;
			++sz;
		}
	}

	T peek_front() const
	{
		return buf[front];
	}

	T pop_front()
	{
		auto oldFront = front;
		front = (front + 1) % N;
		sz -= sz != 0;
		return buf[oldFront];
	}

	constexpr uint8_t capacity() const
	{
		return N;
	}

	uint8_t size() const
	{
		return sz;
	}
};

#endif //CIRCULAR_BUFFER_H
