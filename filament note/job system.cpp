

class Job 
{
    /*
    * 创建子job的时候会增加父job的 runningJobCount 的值,每一个job在创建的时候其本身就是一个job,所以 runningJobCount 的默认值是1.
    * 通常父job就是 mRootJob,一个没有任务的空job.
    * 
    * job 是一个有执行函数和成员变量的对象,它可以被多次执行(即调用run函数),每调用一次run函数,refCount就增1.
    * 每调用一次release或者finish,refCount就减1.当 refCount 变成默认值1的时候,job对象从 mJobPool 释放.
    */
    std::atomic<uint16_t> runningJobCount = { 1 };  
    mutable std::atomic<uint16_t> refCount = { 1 };       
};

void JobSystem::loop(ThreadState* state) noexcept 
{
    do {
        /*
        * 如果 execute 函数里面能取到job,就不会进到下面的if语句里面, execute 函数里面执行完job之后会继续调用 execute 函数,寻找新的job执行.
        * 如果 execute 函数里面没有取到job,那么会进入下面的if语句里面,然后判断是否有等待执行的job,如果有,就继续进入 execute 函数,寻找新的job执行.
        * 如果 没有等待执行的job了,本线程就会挂起,等待被唤醒.
        */
        if (!execute(*state)) { 
            std::unique_lock<Mutex> lock(mWaiterLock);
            while (!exitRequested() && !hasActiveJobs()) {
                wait(lock);
            }
        }
    } while (!exitRequested());
}

/*
* 如果本线程的队列有job就从本线程的队列取一个job执行.
* 如果本线程的队列没有job就从其它线程的队列取一个job执行.
* 如果都没有job,即job为 nullptr,就返回false.
*/
bool JobSystem::execute(JobSystem::ThreadState& state) noexcept 
{
    Job* job = pop(state.workQueue);
    if (UTILS_UNLIKELY(job == nullptr)) {
        // our queue is empty, try to steal a job
        job = steal(state);
    }

    if (job) {
        if (UTILS_LIKELY(job->function)) {
            job->function(job->storage, *this, job);
        }
        finish(job);
    }
    return job != nullptr;
}

/*
* 该job已经完成, refCount 减1,如果 refCount 已经等于默认值1了,就把该job从 mJobPool 释放.
* 线程池有了新的空间,就可能有新的job被创建,并等待被执行,所以调用wakeAll激活所有挂起的线程.
*/
void JobSystem::finish(Job* job) noexcept 
{

}

