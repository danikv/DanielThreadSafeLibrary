/*
 * queue_exceptions.h
 *
 *  Created on: Jun 10, 2017
 *      Author: daniel
 */

#ifndef QUEUE_EXCEPTIONS_H_
#define QUEUE_EXCEPTIONS_H_

class QueueEmpty : public std::exception
{
	const char * what() const noexcept override
	{
		return "queue is empty cannot perform pop operations";
	}
};

class QueueFull : public std::exception
{
	const char * what() const noexcept override
	{
		return "queue is full cannot perform push operations";
	}
};


#endif /* QUEUE_EXCEPTIONS_H_ */
