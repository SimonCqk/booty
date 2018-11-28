#ifndef BOOTY_BASE_SLIDE_WINDOW_HPP
#define BOOTY_BASE_SLIDE_WINDOW_HPP

#include <atomic>
#include <mutex>
#include <vector>

/// SilidWindows implements a slide window utility class which can be used for
/// multiply scenarios, such as flow control, in-progress control...
///
/// T indicates the elements stored in window;
/// if enableContinuous is set to `true`, there will be a strong
/// monotonically increasing guarantee applied over elements in window.
///
/// thread-safe.
template <typename T, bool enableContinuous = false>
class SlideWindow {
   public:
    explicit SlideWindow(size_t windowSize) : sizeLimit_(windowSize) {
        reset();
    }

    bool add(T&& ele) {
        size_t next = startIdx_.load(std::memory_order_acquire) +
                      count_.load(std::memory_order_acquire);
        // next may rotate
        if (next >= sizeLimit_) next -= sizeLimit_;

        if (count_.load() > 0 && enableContinuous) {
            // check monotonically inscreasement
            size_t prior = next > 0 ? next - 1 : buff_.size() - 1;
            if (buff_[prior] >= buff_[next]) return false;
        }
    }

    void freeToIndex(size_t index) {}

    void freeToVal(T&& ele) {}

    // clear and reset the window.
    void reset() {
        count_.store(0, std::memory_order_relaxed);
        startIdx_.store(0, std::memory_order_relaxed);
    }

    // if window is full, no more elements can be added.
    bool full() const {
        return count_.load(std::memory_order_relaxed) == sizeLimit_;
    }

    // forbid copy-ctor & copy-operator
    SlideWindow(const SlideWindow&) = delete;
    SlideWindow& operator=(const SlideWindow&) = delete;

   private:
    // size limitation
    const size_t sizeLimit_;
    // for sync
    std::mutex mutex_;
    // current number of elements in this window
    std::atomic<size_t> count_;
    // index of first element in this window
    std::atomic<size_t> startIdx_;
    // window buffer
    std::vector<T> buff_;
};

#endif