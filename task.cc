#include "task.h"

Task::Task() throw()
{
}

Task::~Task() throw()
{
}


CancelableTask::CancelableTask() throw()
	: needed( true )
{
}

CancelableTask::~CancelableTask() throw()
{
}

void CancelableTask::cancel() throw()
{
	needed = false;
}

void CancelableTask::task()
{
	if( !needed )
		return;

	real_task();
}
