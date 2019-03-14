#include <chrono>
#include <iostream>
#include <ctime>
#include <thread>
#include <functional>

#include "g3log/g3log.hpp"
#include "g3log/shared_lockfree_queue.hpp"
#include "g3log/shared_queue.hpp"

namespace {
typedef std::chrono::high_resolution_clock::time_point time_point;
typedef std::chrono::duration<uint64_t,std::ratio<1, 1000> > millisecond;
typedef std::chrono::duration<uint64_t,std::ratio<1, 1000000> > microsecond;
typedef std::function<void()> CallBack;
};

shared_queue<CallBack> g_queue;
shared_lockfree_queue<CallBack> g_lckfr_q;

void normal_producer(int times) {
    for(int i = 0; i < times; i++) {
        g_queue.push([i]() { std::this_thread::sleep_for(std::chrono::microseconds(1));});  // sleep 3ms for consumer
    }
}
void normal_comsumer(int times) {
    for(int i = 0; i < times; i++) {
        CallBack func;
        g_queue.wait_and_pop(func);
        func();
    }
}
void lockfree_producer(int times) {
    for(int i = 0; i < times; i++) {
        g_lckfr_q.push([i]() { std::this_thread::sleep_for(std::chrono::microseconds(1)); });
    }
}
void lockfree_comsumer(int times) {
    for(int i = 0; i < times; i++) {
        CallBack func;
        g_lckfr_q.wait_and_pop(func);
        func();
    }
}

int main(int argc, char** argv) {
    int thread_count(1);
    int entry_count(10000);
    if (argc == 3) {
        thread_count = atoi(argv[1]);
        entry_count = atoi(argv[2]);
    } else if (argc == 2) {
        thread_count = atoi(argv[1]);
    } else {
        std::cout << "Usage: " << argv[0] << " thread_count <entry_count>" << std::endl;
        return -1;
    }
    
    std::ostringstream oss;

    // Suppose only 1 comsumer
    std::thread consumer;
    std::thread *producer = new std::thread[thread_count];
    
    
    
    
    {  // test for normal queue
    std::string TAG("Normal");
    auto start_time = std::chrono::high_resolution_clock::now();

    consumer = std::thread(normal_comsumer, thread_count * entry_count);
    std::cout << TAG + "_Consumer_T created!" <<std::endl;
    for(size_t i = 0; i < thread_count; i++) {
        std::ostringstream count;
        
        count << i + 1;
        std::string thread_name = TAG + "_Producer_T" + count.str();
        std::cout << "Creating thread: " << thread_name << std::endl;
        producer[i] = std::thread(normal_producer, entry_count);
    }
    for (size_t idx = 0; idx < thread_count; ++idx) {
        producer[idx].join();
    }
    consumer.join();
    auto end_time = std::chrono::high_resolution_clock::now();

    uint64_t total_time_us = std::chrono::duration_cast<microsecond>(end_time - start_time).count();
    oss << thread_count << " * " << entry_count << "items in total" 
        << "\nTotal time used: " << total_time_us / uint64_t(1000) << "ms"
        << "\nAverage time per entry: " << total_time_us / (thread_count * entry_count) << "us"
        << std::endl;
    }

    {  // test for lock free queue
    std::string TAG("Lock-free");
    auto start_time = std::chrono::high_resolution_clock::now();

    consumer = std::thread(lockfree_comsumer, thread_count * entry_count);
    std::cout << TAG + "_Consumer_T created!" <<std::endl;
    for(size_t i = 0; i < thread_count; i++) {
        std::ostringstream count;
        
        count << i + 1;
        std::string thread_name = TAG + "_Producer_T" + count.str();
        std::cout << "Creating thread: " << thread_name << std::endl;
        producer[i] = std::thread(lockfree_producer, entry_count);
    }
    for (size_t idx = 0; idx < thread_count; ++idx) {
        producer[idx].join();
    }
    consumer.join();
    auto end_time = std::chrono::high_resolution_clock::now();

    uint64_t total_time_us = std::chrono::duration_cast<microsecond>(end_time - start_time).count();
    oss << thread_count << " * " << entry_count << "items in total" 
        << "\nTotal time used: " << total_time_us / uint64_t(1000) << "ms"
        << "\nAverage time per entry: " << total_time_us / (thread_count * entry_count) << "us"
        << std::endl;
    }

    std::cout << oss.str()<< std::endl;
}