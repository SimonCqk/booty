#ifndef BOOTY_BASE_SLIDE_WINDOW_HPP
#define BOOTY_BASE_SLIDE_WINDOW_HPP

#include <shared_mutex>
#include <vector>

/// SilidWindows implements a slide window utility class which can be used for
/// multiply scenarios, such as flow control, in-progress control...
///
/// T indicates the elements stored in window;
/// if enableContinuous is set to `true`, there will be a strong
/// monotonically increasing guarantee applied over elements in window.
template <typename T, bool enableContinuous = false>
class SlideWindow {
    constexpr static size_t kInitWindowCapacity = 8;

   public:
    explicit SlideWindow(size_t windowSize) : sizeLimit_(windowSize) {
        reset();
        buff_.reserve(kInitWindowCapacity);
    }

    // add push a new element into slide window, and return the results.
    bool add(T&& ele) {
        if (full()) return false;
        
        std::unique_lock<std::shared_mutex> lock(mutex_);
        size_t next = startIdx_ + count_;
        // next may rotate
        if (next >= sizeLimit_) next -= sizeLimit_;

        if (count_ > 0 && enableContinuous) {
            // check monotonically inscreasement
            size_t prior = next > 0 ? next - 1 : buff_.size() - 1;
            if (buff_[prior] >= buff_[next]) return false;
        }

        // grow
        if (next >= buff_.size()) growBuff();
        buff_[next] = std::forward(ele);
        count_++;
    }

    // free the space out-bound of index...start+count
    void freeToIndex(size_t index) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        assert(index >= 0 && index < sizeLimit_);
        startIdx_ = index;
    }

    // free the space from index...ele
    // if enableContinuous is true, the bound is start...somewhere >= ele
    // otherwise, the bound is start...ele its self.
    void freeToVal(T&& ele) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        size_t idx = 0, start = startIdx_;
        for (size_t cnt = count_; idx < cnt; idx++) {
            // find first element equal or greater
            if (ele <= buff_[start]) break;
            start++;
            if (start >= sizeLimit_) start -= sizeLimit_;
        }
        count_ -= idx;
        startIdx_ = start;
        if (count_ == 0) startIdx_ = 0;
    }

    // clear and reset the window.
    void reset() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        count_ = 0;
        startIdx_ = 0;
    }

    // if window is full, no more elements can be added.
    bool full() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return count_ == sizeLimit_;
    }

    size_t maxSize() const { return sizeLimit_; }

    // forbid copy-ctor & copy-operator
    SlideWindow(const SlideWindow&) = delete;
    SlideWindow& operator=(const SlideWindow&) = delete;

   private:
    void growBuff() {
        size_t newSize = buff_.empty() ? 1 : buff_.size() * 2;
        if (newSize >= sizeLimit_) newSize = sizeLimit_;
        buff_.resize(newSize);
    }

    // size limitation
    const size_t sizeLimit_;
    // for sync
    std::shared_mutex mutex_;
    // current number of elements in this window
    size_t count_;
    // index of first element in this window
    size_t startIdx_;
    // window buffer
    std::vector<T> buff_;
};

#endif