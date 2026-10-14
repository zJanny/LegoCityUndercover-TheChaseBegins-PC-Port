#include "render/front_presenter.hpp"
#include <algorithm>
#include <cmath>
#include <epoxy/gl.h>

namespace chase::render {

class FrontPresenterImpl {
public:
    FrontPresenterImpl(int topWidth, int bottomWidth, int screenHeight);
    ~FrontPresenterImpl();
    void present(const Pixels& top, const Pixels& bottom, int width, int height);

private:
    void upload(int index, const Pixels& image);
    void blit(int index, float x, float yFromTop, float width, float height, int sourceWidth, int windowHeight);

    int topWidth_, bottomWidth_, screenHeight_;
    GLuint textures_[2] = {0, 0}, framebuffers_[2] = {0, 0};
};

FrontPresenterImpl::FrontPresenterImpl(int topWidth, int bottomWidth, int screenHeight)
    : topWidth_(topWidth), bottomWidth_(bottomWidth), screenHeight_(screenHeight) {
    glGenTextures(2, textures_);
    glGenFramebuffers(2, framebuffers_);
    const int widths[2] = {topWidth_, bottomWidth_};
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, textures_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, widths[i], screenHeight_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers_[i]);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures_[i], 0);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

FrontPresenterImpl::~FrontPresenterImpl() {
    glDeleteFramebuffers(2, framebuffers_);
    glDeleteTextures(2, textures_);
}

void FrontPresenterImpl::present(const Pixels& top, const Pixels& bottom, int width, int height) {
    upload(0, top);
    upload(1, bottom);
    setDepthTest(false);
    setCullFace(false);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    setViewport({0, 0, width, height});
    clearWindow({0, 0, 0, 1}, false);
    const float scale = std::min(float(width) / topWidth_, float(height) / (2 * screenHeight_));
    const float originX = (float(width) - scale * topWidth_) / 2, originY = (float(height) - scale * 2 * screenHeight_) / 2;
    blit(0, originX, originY, scale * topWidth_, scale * screenHeight_, topWidth_, height);
    blit(1, originX + scale * (topWidth_ - bottomWidth_) / 2, originY + scale * screenHeight_, scale * bottomWidth_,
         scale * screenHeight_, bottomWidth_, height);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void FrontPresenterImpl::upload(int index, const Pixels& image) {
    glBindTexture(GL_TEXTURE_2D, textures_[index]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.width, image.height, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba);
}

void FrontPresenterImpl::blit(int index, float x, float yFromTop, float width, float height, int sourceWidth, int windowHeight) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers_[index]);
    const int left = int(std::lround(x)), right = int(std::lround(x + width));
    const int bottom = windowHeight - int(std::lround(yFromTop + height)), top = windowHeight - int(std::lround(yFromTop));
    glBlitFramebuffer(0, screenHeight_, sourceWidth, 0, left, bottom, right, top, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

FrontPresenter::FrontPresenter(int topWidth, int bottomWidth, int screenHeight)
    : impl_(std::make_unique<FrontPresenterImpl>(topWidth, bottomWidth, screenHeight)) {}

FrontPresenter::~FrontPresenter() = default;

void FrontPresenter::present(const Pixels& top, const Pixels& bottom, int width, int height) {
    impl_->present(top, bottom, width, height);
}

}
