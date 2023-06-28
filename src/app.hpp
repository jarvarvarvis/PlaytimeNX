#pragma once

#include "nanovg/nanovg.h"
#include "nanovg/deko3d/dk_renderer.hpp"
#include "async.hpp"
#include "playtime.hpp"
#include "controller.hpp"

#include <switch.h>
#include <cstdint>
#include <vector>
#include <string>
#include <future>
#include <mutex>
#include <optional>
#include <functional>
#include <stop_token>
#include <utility>

namespace tj {

using AppID = std::uint64_t;

enum class MenuMode { LOAD, LIST };

struct AppEntry final {
    std::string name;
    std::string author;
    std::string display_version;
    Playtime playtime;
    AppID id;
    int image;
    bool own_image{false};
};

class App final {
public:
    App();
    ~App();
    void Loop();

    void SpawnScanThread();
    void RequestAccountUid();

private:
    NVGcontext* vg{nullptr};
    std::vector<AppEntry> entries;
    PadState pad{};
    Controller controller{};
    AccountUid account_uid;

    util::AsyncFuture<void> async_thread;
    std::mutex mutex{};
    bool finished_scanning{false}; // mutex locked

    // this is just bad code, ignore it
    static constexpr float BOX_HEIGHT{120.f};
    float yoff{130.f};
    float ypos{130.f};
    std::size_t start{0};
    std::size_t index{}; // where i am in the array
    MenuMode menu_mode{MenuMode::LOAD};
    int default_icon_image{};
    bool has_corrupted{false};
    bool quit{false};

    enum class SortType {
        Alpha_AZ,
        Alpha_ZA,
        Playtime_BigSmall,
        Playtime_SmallBig,
        MAX,
    };

    uint8_t sort_type{std::to_underlying(SortType::Playtime_BigSmall)};

    void Draw();
    void Update();
    void Poll();
    void Scan(std::stop_token stop_token); // called on init
    void Sort();
    const char* GetSortStr();

    void UpdateLoad();
    void UpdateList();
    void UpdateConfirm();
    void UpdateProgress();

    void DrawBackground();
    void DrawLoad();
    void DrawList();

private: // from nanovg decko3d example by adubbz
    static constexpr unsigned NumFramebuffers = 2;
    static constexpr unsigned StaticCmdSize = 0x1000;
    dk::UniqueDevice device;
    dk::UniqueQueue queue;
    std::optional<CMemPool> pool_images;
    std::optional<CMemPool> pool_code;
    std::optional<CMemPool> pool_data;
    dk::UniqueCmdBuf cmdbuf;
    CMemPool::Handle depthBuffer_mem;
    CMemPool::Handle framebuffers_mem[NumFramebuffers];
    dk::Image depthBuffer;
    dk::Image framebuffers[NumFramebuffers];
    DkCmdList framebuffer_cmdlists[NumFramebuffers];
    dk::UniqueSwapchain swapchain;
    DkCmdList render_cmdlist;
    std::optional<nvg::DkRenderer> renderer;
    void createFramebufferResources();
    void destroyFramebufferResources();
    void recordStaticCommands();
};

} // namespace tj
