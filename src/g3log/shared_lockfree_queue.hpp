#pragma once

#include <atomic>
#include <exception>
#include <thread>  // this_thread::yield

#include "atomic_ops.h"

#define ARRAY_LOCK_FREE_Q_DEFAULT_SIZE 65536  // (2^16
#define ARRAY_LOCK_FREE_Q_DEFAULT_SIZE 8388608  // (2^23)

// define this variable if calls to "size" must return the real size of the 
// queue. If it is undefined  that function will try to take a snapshot of 
// the queue, but returned value might be bogus
#undef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
//#define ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE 1


/** Multiple producer, multiple consumer thread safe queue **/
template<typename T, uint32_t Q_SIZE = ARRAY_LOCK_FREE_Q_DEFAULT_SIZE>
class shared_lockfree_queue {
private:
    // array based queue
    T queue_[Q_SIZE];

#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
    // @brief number of elements in the queue
    std::atomic_uint32_t count_;
#endif

    // @brief the position when insert a new element
    volatile uint32_t write_index_;

    // @brief the position when extract an element
    volatile uint32_t read_index_;

    /// @brief maximum read index for multiple producer queues
    /// If it's not the same as m_writeIndex it means
    /// there are writes pending to be "committed" to the queue, that means,
    /// the place for the data was reserved (the index in the array) but  
    /// data is still not in the queue, so the thread trying to read will have 
    /// to wait for those other threads to save the data into the queue
    volatile uint32_t maximum_read_index_;

    inline uint32_t count_to_index(uint32_t count) const {
        // if Q_SIEZE is power of 2, this statement could be also written as 
        // return (count & (QSIZE - 1));
        return (count % Q_SIZE);
    }

    shared_lockfree_queue &operator=(const shared_lockfree_queue&) = delete;
    shared_lockfree_queue(const shared_lockfree_queue &other) = delete;
public:
    shared_lockfree_queue() : 
        write_index_(0),
        read_index_(0)
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
        , count_(0)
#endif
    {}

    virtual ~shared_lockfree_queue(){}

    uint32_t size() const {
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
        return count_;
#else
        uint32_t current_write_index = write_index_;
        uint32_t current_read_index = read_index_;
        // let's think of a scenario where this function returns bogus data
        // 1. when the statement 'currentWriteIndex = m_writeIndex' is run
        // m_writeIndex is 3 and m_readIndex is 2. Real size is 1
        // 2. afterwards this thread is preemted. While this thread is inactive 2 
        // elements are inserted and removed from the queue, so m_writeIndex is 5
        // m_readIndex 4. Real size is still 1
        // 3. Now the current thread comes back from preemption and reads m_readIndex.
        // currentReadIndex is 4
        // 4. currentReadIndex is bigger than currentWriteIndex, so
        // m_totalSize + currentWriteIndex - currentReadIndex is returned, that is,
        // it returns that the queue is almost full, when it is almost empty
        if (current_write_index >= current_read_index) {
            return (current_write_index - current_read_index);
        } else {
            return (Q_SIZE + current_write_index - current_read_index);
        }        
#endif  // ARRYA_LOCK_FREE_Q_KEEP_REAL_SIZE
    }

    bool push(const T &item) {
        uint32_t current_read_index;
        uint32_t current_write_index;

        do {
            current_read_index = read_index_;
            current_write_index = write_index_;
            if (count_to_index(current_write_index + 1) == count_to_index(current_read_index)) {
                // the queue is full
                // msg will discard if caller doesn't check return value
                return false;
            }            
        } while (!CAS(&write_index_, current_write_index, (current_write_index + 1)));

        // now current write index is reserved for us. save data now
        queue_[count_to_index(current_write_index)] = std::move(item);
        
        // update the maximum read index after saving the data. It wouldn't fail if there is only one thread 
        // inserting in the queue. It might fail if there are more than 1 producer threads because this
        // operation has to be done in the same order as the previous CAS
        while(!CAS(&maximum_read_index_, current_write_index, (current_write_index + 1))){
            // this is a good place to yield the thread in case there are more
            // software threads than hardware processors and you have more
            // than 1 producer thread
            std::this_thread::yield();
        }

        // the value was successfully inserted into the queue
#ifdef ARRYA_LOCK_FREE_Q_KEEP_REAL_SIZE
        count_.fetch_add(1);
#endif
        return true;
    }

    bool try_and_pop(T& poped_item) {
        uint32_t current_maximum_read_index;
        uint32_t current_read_index;

   
        // to ensure thread-safety when there is more than 1 producer thread
        // a second index is defined (m_maximumReadIndex)
        current_read_index = read_index_;
        current_maximum_read_index = maximum_read_index_;

        if (count_to_index(current_read_index) == count_to_index(current_maximum_read_index)) {
            // the queue is empty or
            // a producer thread has allocate space in the queue but is 
            // waiting to commit the data into it
            // then pop failed
            return false;
        }

        // retrive the data from the queue
        poped_item = std::move(queue_[count_to_index(current_read_index)]);

        // try to perfrom now the CAS operation on the read index. If we succeed
        // a_data already contains what m_readIndex pointed to before we 
        // increased it
        if (CAS(&read_index_, current_read_index, (current_read_index + 1))) {
            // got here. The value was retrieved from the queue. Note that the
            // data inside the m_queue array is not deleted nor reseted
#ifdef ARRYA_LOCK_FREE_Q_KEEP_REAL_SIZE
            count_.fetch_sub(1);
#endif
            return true;
        }

        // it failed retrieving the element off the queue. Someone else must
        // have read the element stored at countToIndex(currentReadIndex)
        // before we could perform the CAS operation 
        // so pop failed
        return false;
    }

    void wait_and_pop(T& poped_item) {
        uint32_t current_maximum_read_index;
        uint32_t current_read_index;

        do {
            // to ensure thread-safety when there is more than 1 producer thread
            // a second index is defined (m_maximumReadIndex)
            current_read_index = read_index_;
            current_maximum_read_index = maximum_read_index_;

            if (count_to_index(current_read_index) == count_to_index(current_maximum_read_index)) {
                // the queue is empty or
                // a producer thread has allocate space in the queue but is 
                // waiting to commit the data into it
                // then continue to wait
                continue;
            }

            // retrive the data from the queue
            poped_item = std::move(queue_[count_to_index(current_read_index)]);

            // try to perfrom now the CAS operation on the read index. If we succeed
            // a_data already contains what m_readIndex pointed to before we 
            // increased it
            if (CAS(&read_index_, current_read_index, (current_read_index + 1))) {
                // got here. The value was retrieved from the queue. Note that the
                // data inside the m_queue array is not deleted nor reseted
#ifdef ARRYA_LOCK_FREE_Q_KEEP_REAL_SIZE
                count_.fetch_sub(1);
#endif
                return;
            }
            // it failed retrieving the element off the queue. Someone else must
            // have read the element stored at countToIndex(currentReadIndex)
            // before we could perform the CAS operation 
            // so try to pop again
                
        } while(1);  // pop failed, try again

        // shouldn't reach here
    }
};

