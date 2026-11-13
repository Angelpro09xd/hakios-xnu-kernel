// (sapphire) HM_BENCH=1: the driver's CPU cost per draw, CoreAnimation-like: frames of 90 draws into a 750x1624 target
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include "test.hpp"
#include "hakios-metal/command.hpp"
#include "hakios-metal/fake_gpu.hpp"
#include "hakios-metal/render.hpp"
#include "hakios-metal/texture.hpp"
#include "fixtures/tri_flat_vs_air.hpp"
#include "fixtures/tri_flat_fs_air.hpp"

using namespace hakios::metal;
using namespace hakios::metal::render;

TEST(zz_bench_draws) {
    if (!getenv("HM_BENCH")) return;
    auto f = std::make_unique<FakeGpu>();
    auto gpu = std::make_unique<Gpu>(std::move(f));
    std::string why;
    auto queue = CommandQueue::make(*gpu, &why);
    TextureDesc td;
    td.format = PixelFormat::kBGRA8Unorm;
    td.width = 750;
    td.height = 1624;
    td.usage = kTextureUsageRenderTarget | kTextureUsageRead;
    auto t = Texture::make(*gpu, td, &why);
    Program p{kernels::tri_flat_vs_air, kernels::tri_flat_fs_air, Linkage{}, "tri_flat"};
    RenderPass pass;
    pass.color0 = t.get();
    const int frames = atoi(getenv("HM_BENCH")) > 1 ? atoi(getenv("HM_BENCH")) : 200, draws = 90;
    auto t0 = std::chrono::steady_clock::now();
    double drawUs = 0;
    for (int fr = 0; fr < frames; fr++) {
        auto cb = queue->commandBuffer();
        for (int i = 0; i < draws; i++) {
            Draw d;
            d.vertexCount = 6;
            d.viewport = Viewport{0, 0, 750, 1624, 0, 1};
            auto a = std::chrono::steady_clock::now();
            bool ok = cb->draw(p, pass, d, Bindings{}, Bindings{});
            drawUs += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - a).count();
            if (!ok) { std::printf("refused: %s\n", cb->error().c_str()); return; }
        }
        cb->commit();
        cb->waitUntilCompleted();
    }
    double all = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::printf("bench: %d frames of %d draws: %.1f ms in all, %.2f us a draw call (encode), %.2f ms a frame\n", frames, draws, all,
                drawUs / (frames * draws), all / frames);
}
