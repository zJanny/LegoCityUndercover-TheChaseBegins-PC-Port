#include "render/frame_stats.hpp"
#include <array>
#include <epoxy/gl.h>

namespace chase::render {

namespace {
unsigned drawCalls = 0;
constexpr int kQueryRing = 4;
}

void countDrawCall() { ++drawCalls; }

class GpuFrameTimerImpl {
public:
    GpuFrameTimerImpl() { glGenQueries(kQueryRing, queries_.data()); }
    ~GpuFrameTimerImpl() { glDeleteQueries(kQueryRing, queries_.data()); }

    void begin() {
        drawCalls = 0;
        if (pending_[next_]) collect(next_);
        glBeginQuery(GL_TIME_ELAPSED, queries_[next_]);
        open_ = true;
    }

    FrameStats end() {
        FrameStats stats;
        stats.drawCalls = drawCalls;
        if (open_) {
            glEndQuery(GL_TIME_ELAPSED);
            pending_[next_] = true;
            next_ = (next_ + 1) % kQueryRing;
            open_ = false;
        }
        for (int step = 0; step < kQueryRing; ++step) {
            const int slot = (next_ + step) % kQueryRing;
            if (!pending_[slot]) continue;
            GLuint available = 0;
            glGetQueryObjectuiv(queries_[slot], GL_QUERY_RESULT_AVAILABLE, &available);
            if (available) collect(slot);
        }
        stats.gpuValid = haveResult_;
        stats.gpuMilliseconds = lastMilliseconds_;
        return stats;
    }

private:
    void collect(int slot) {
        GLuint64 nanoseconds = 0;
        glGetQueryObjectui64v(queries_[slot], GL_QUERY_RESULT, &nanoseconds);
        pending_[slot] = false;
        lastMilliseconds_ = double(nanoseconds) / 1.0e6;
        haveResult_ = true;
    }

    std::array<GLuint, kQueryRing> queries_{};
    std::array<bool, kQueryRing> pending_{};
    int next_ = 0;
    bool open_ = false;
    bool haveResult_ = false;
    double lastMilliseconds_ = 0;
};

GpuFrameTimer::GpuFrameTimer() : impl_(std::make_unique<GpuFrameTimerImpl>()) {}
GpuFrameTimer::~GpuFrameTimer() = default;
void GpuFrameTimer::begin() { impl_->begin(); }
FrameStats GpuFrameTimer::end() { return impl_->end(); }

}
