#include <muduo/base/BlockingQueue.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Timestamp.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <map>
#include <string>
#include <stdio.h>

class Bench
{
public:
	Bench(int numThreads) // �̵߳���Ŀ
		: latch_(numThreads) 
		, threads_(numThreads)
	{
		for (int i = 0; i < numThreads; ++i)
		{
			char name[32];
			snprintf(name, sizeof name, "work thread %d", i);
			threads_.push_back(new muduo::Thread(
			      boost::bind(&Bench::threadFunc, this), // �����Ҫ���еĺ���
				muduo::string(name))); // ���ǹ����̣߳�Ȼ����threads_�����
		}
		for_each(threads_.begin(), threads_.end(), boost::bind(&muduo::Thread::start, _1)); // ������һЩ����ʽ��̵�ζ����
		// ����Ļ�����Ҫ˵һ����boost::bind(&moduo::Thread::start, -1)������һ����������Ϊt��Ȼ�󽫵���t(arg)
	}

	void run(int times)
	{
		printf("waiting for count down latch\n");
		latch_.wait(); // �ȴ��źŰɣ�
		printf("all threads started\n"); // ���е��̶߳��Ѿ���ʼ�����ˡ�
		for (int i = 0; i < times; ++i)
		{
			muduo::Timestamp now(muduo::Timestamp::now());
			queue_.put(now); // ���е㲻�����ǣ�ΪʲôҪ�����������ô���Timestamp
			usleep(1000);
		}
	}

	void joinAll()
	{
		for (size_t i = 0; i < threads_.size(); ++i)
		{
			queue_.put(muduo::Timestamp::invalid());
		}

		for_each(threads_.begin(), threads_.end(), boost::bind(&muduo::Thread::join, _1)); // ����ÿһ�����̶�����join����
	}

private:

	void threadFunc()
	{
		printf("tid=%d, %s started\n",
			muduo::CurrentThread::tid(),  // �̵߳�id��
			muduo::CurrentThread::name()); // �̵߳�����

		std::map<int, int> delays;
		latch_.countDown(); // ����ʱһ���Ķ���
		bool running = true;
		while (running)
		{
			muduo::Timestamp t(queue_.take()); // ���ϵ�ȡʱ���
			muduo::Timestamp now(muduo::Timestamp::now());
			if (t.valid()) // 
			{
				int delay = static_cast<int>(timeDifference(now, t) * 1000000);
				// printf("tid=%d, latency = %d us\n",
				//        muduo::CurrentThread::tid(), delay);
				++delays[delay]; // ��ʱ
			}
			running = t.valid();
		}

		printf("tid=%d, %s stopped\n",
			muduo::CurrentThread::tid(),
			muduo::CurrentThread::name()); // ��ʾ�߳�ֹͣ����
		for (std::map<int, int>::iterator it = delays.begin();
		    it != delays.end(); ++it)
		{
			printf("tid = %d, delay = %d, count = %d\n",
				muduo::CurrentThread::tid(),
				it->first,  // key
				it->second); // value
		}
	}

	muduo::BlockingQueue<muduo::Timestamp> queue_; // ʱ��
	muduo::CountDownLatch latch_; // �ðɣ���Ȼ���������
	boost::ptr_vector<muduo::Thread> threads_; // ָ���vector�����ã�����
};

int main(int argc, char* argv[])
{
	int threads = argc > 1 ? atoi(argv[1]) : 1;
	threads = 120;
	Bench t(threads);
	t.run(10000); // ����1000���߳���
	t.joinAll();
}
