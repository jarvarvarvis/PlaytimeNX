#include "app.hpp"
#include "nvg_util.hpp"
#include "nanovg/deko3d/nanovg_dk.h"

#include <algorithm>
#include <ranges>
#include <cassert>

#ifndef NDEBUG
    #include <cstdio>
    #define LOG(...) std::printf(__VA_ARGS__)
#else // NDEBUG
    #define LOG(...)
#endif // NDEBUG

namespace tj {
namespace {

// thank you Shchmue ^^
struct ApplicationOccupiedSizeEntry {
    std::uint8_t storageId;
    std::uint64_t sizeApplication;
    std::uint64_t sizePatch;
    std::uint64_t sizeAddOnContent;
};

struct ApplicationOccupiedSize {
    ApplicationOccupiedSizeEntry entry[4];
};

constexpr float SCREEN_WIDTH = 1280.f;
constexpr float SCREEN_HEIGHT = 720.f;

// taken from my gamecard installer
struct PulseColour {
    NVGcolor col{0, 255, 187, 255};
    u8 delay;
    bool increase_blue;
};

PulseColour pulse;

void update_pulse_colour() {
    if (pulse.col.g == 255) {
        pulse.increase_blue = true;
    } else if (pulse.col.b == 255 && pulse.delay == 10) {
        pulse.increase_blue = false;
        pulse.delay = 0;
    }

    if (pulse.col.b == 255 && pulse.increase_blue == true) {
        pulse.delay++;
    } else {
        pulse.col.b = pulse.increase_blue ? pulse.col.b + 2 : pulse.col.b - 2;
        pulse.col.g = pulse.increase_blue ? pulse.col.g - 2 : pulse.col.g + 2;
    }
}

} // namespace

void App::Loop() {
    while (!this->quit && appletMainLoop()) {
        this->Poll();
        this->Update();
        this->Draw();
    }
}

void App::Poll() {
    padUpdate(&this->pad);

    const auto down = padGetButtonsDown(&this->pad);
    const auto held = padGetButtons(&this->pad);

    this->controller.A = down & HidNpadButton_A;
    this->controller.B = down & HidNpadButton_B;
    this->controller.X = down & HidNpadButton_X;
    this->controller.Y = down & HidNpadButton_Y;
    this->controller.L = down & HidNpadButton_L;
    this->controller.R = down & HidNpadButton_R;
    this->controller.L2 = down & HidNpadButton_ZL;
    this->controller.R2 = down & HidNpadButton_ZR;
    this->controller.START = down & HidNpadButton_Plus;
    this->controller.SELECT = down & HidNpadButton_Minus;
    // keep directional keys pressed.
    this->controller.DOWN = (down & HidNpadButton_AnyDown);
    this->controller.UP = (down & HidNpadButton_AnyUp);
    this->controller.LEFT = (down & HidNpadButton_AnyLeft);
    this->controller.RIGHT = (down & HidNpadButton_AnyRight);

    this->controller.UpdateButtonHeld(this->controller.DOWN, held & HidNpadButton_AnyDown);
    this->controller.UpdateButtonHeld(this->controller.UP, held & HidNpadButton_AnyUp);
}

void App::Update() {
    switch (this->menu_mode) {
        case MenuMode::LOAD:
            this->UpdateLoad();
            break;
        case MenuMode::LIST:
            this->UpdateList();
            break;
    }
}

void App::Draw() {
    const auto slot = this->queue.acquireImage(this->swapchain);
    this->queue.submitCommands(this->framebuffer_cmdlists[slot]);
    this->queue.submitCommands(this->render_cmdlist);
    nvgBeginFrame(this->vg, SCREEN_WIDTH, SCREEN_HEIGHT, 1.f);

    this->DrawBackground();

    switch (this->menu_mode) {
        case MenuMode::LOAD:
            this->DrawLoad();
            break;
        case MenuMode::LIST:
            this->DrawList();
            break;
    }

    nvgEndFrame(this->vg);
    this->queue.presentImage(this->swapchain, slot);
}

void App::DrawBackground() {
    gfx::drawRect(this->vg, 0.f, 0.f, SCREEN_WIDTH, SCREEN_HEIGHT, gfx::Colour::BLACK);
    gfx::drawRect(vg, 30.f, 86.0f, 1220.f, 1.f, gfx::Colour::WHITE);
    gfx::drawRect(vg, 30.f, 646.0f, 1220.f, 1.f, gfx::Colour::WHITE);
}

void App::DrawLoad() {
    gfx::drawRect(this->vg, 0.f, 0.f, SCREEN_WIDTH, SCREEN_HEIGHT, gfx::Colour::BLACK);
    gfx::drawTextArgs(this->vg, SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f, 36.f, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE, gfx::Colour::WHITE, "Loading...");
    gfx::drawButtons(this->vg, gfx::pair{gfx::Button::B, "Back"});
}

void App::DrawList() {
    constexpr auto x = 90.f;
    constexpr auto box_height = 120.f;
    constexpr auto box_width = SCREEN_WIDTH - 2 * x;
    constexpr auto icon_spacing = 12.f;
    constexpr auto title_spacing_left = 116.f;
    constexpr auto title_spacing_top = 30.f;
    constexpr auto text_spacing_left = title_spacing_left;
    constexpr auto text_spacing_top = 67.f;
    constexpr auto sidebox_x = 870.f;
    constexpr auto sidebox_y = 87.f;
    constexpr auto sidebox_w = 380.f;
    constexpr auto sidebox_h = 558.f;

// uses the APP_VERSION define in makefile for string version.
// source: https://stackoverflow.com/a/2411008
#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)
    gfx::drawText(this->vg, 70.f, 40.f, 28.f, STRINGIZE_VALUE_OF(APP_TITLE), nullptr, NVG_ALIGN_LEFT | NVG_ALIGN_TOP, gfx::Colour::WHITE);
    gfx::drawText(this->vg, 1224.f, 45.f, 22.f, STRINGIZE_VALUE_OF(APP_VERSION_STRING), nullptr, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP, gfx::Colour::SILVER);
#undef STRINGIZE
#undef STRINGIZE_VALUE_OF

    nvgSave(this->vg);
    nvgScissor(this->vg, 30.f, 86.0f, 1220.f, 646.0f); // clip

    auto y = this->yoff;

    for (size_t i = this->start; i < this->entries.size(); i++) {
        if (i == this->index) {
            // idk how to draw an outline, so i draw a colour rect then draw black rect ontop
            auto col = pulse.col;
            col.r /= 255.f;
            col.g /= 255.f;
            col.b /= 255.f;
            col.a = 1.f;
            update_pulse_colour();
            gfx::drawRect(this->vg, x - 5.f, y - 5.f, box_width + 10.f, box_height + 10.f, col);
            gfx::drawRect(this->vg, x, y, box_width, box_height, gfx::Colour::BLACK);
        }

        gfx::drawRect(this->vg, x, y, box_width, 1.f, gfx::Colour::DARK_GREY);
        gfx::drawRect(this->vg, x, y + box_height, box_width, 1.f, gfx::Colour::DARK_GREY);

        const auto icon_paint = nvgImagePattern(this->vg, x + icon_spacing, y + icon_spacing, 90.f, 90.f, 0.f, this->entries[i].image, 1.f);
        gfx::drawRect(this->vg, x + icon_spacing, y + icon_spacing, 90.f, 90.f, icon_paint);

        nvgSave(this->vg);
        nvgScissor(this->vg, x + title_spacing_left, y, 585.f, box_height); // clip
        gfx::drawText(this->vg, x + title_spacing_left, y + title_spacing_top, 24.f, this->entries[i].name.c_str(), nullptr, NVG_ALIGN_LEFT | NVG_ALIGN_TOP, gfx::Colour::WHITE);
        nvgRestore(this->vg);

        const auto draw_playtime = [&](float x_offset, Playtime playtime) {
            gfx::drawTextArgs(this->vg, x + text_spacing_left + x_offset, y + text_spacing_top + 9.f, 22.f, NVG_ALIGN_LEFT | NVG_ALIGN_TOP, gfx::Colour::SILVER, 
                    "Playtime: %s", playtime.toString().c_str());
        };

        draw_playtime(0.f, this->entries[i].playtime);

        y += box_height;

        // out of bounds (clip)
        if ((y + box_height) > 646.f) {
            break;
        }
    }

    nvgRestore(this->vg);

    AppEntry current_entry = this->entries[this->index];
    gfx::drawTextArgs(this->vg, 55.f, 670.f, 24.f, NVG_ALIGN_LEFT | NVG_ALIGN_TOP, gfx::Colour::WHITE, 
            "Current (%lu / %lu): %s", 
                this->index + 1, this->entries.size(), 
                current_entry.playtime.toString().c_str());

    gfx::drawButtons(this->vg, 
            gfx::pair{gfx::Button::B, "Exit"}, 
            gfx::pair{gfx::Button::R, this->GetSortStr()});

}

void App::Sort()
{
    switch (static_cast<SortType>(this->sort_type))
    {
        case SortType::Alpha_AZ: std::ranges::sort(this->entries, [](AppEntry& a, AppEntry& b) { return a.name < b.name; }); break;
        case SortType::Alpha_ZA: std::ranges::sort(this->entries, [](AppEntry& a, AppEntry& b) { return a.name > b.name; }); break;
        case SortType::Playtime_BigSmall: std::ranges::sort(this->entries, [](AppEntry& a, AppEntry& b) { return a.playtime.totalSeconds() > b.playtime.totalSeconds(); }); break;
        case SortType::Playtime_SmallBig: std::ranges::sort(this->entries, [](AppEntry& a, AppEntry& b) { return a.playtime.totalSeconds() < b.playtime.totalSeconds(); }); break;
    }
}

const char* App::GetSortStr() {
    switch (static_cast<SortType>(this->sort_type)) {
        case SortType::Alpha_AZ: return "Sort Alpha: A-Z";
        case SortType::Alpha_ZA: return "Sort Alpha: Z-A";
        case SortType::Playtime_BigSmall: return "Sort Playtime: 9-0";
        case SortType::Playtime_SmallBig: return "Sort Playtime: 0-9";
    }

    std::unreachable();
}

void App::UpdateLoad() {
    if (this->controller.B) {
        this->async_thread.request_stop();
        this->async_thread.get();
        this->quit = true;
        return;
    }

    {
        std::scoped_lock lock{this->mutex};
        if (this->finished_scanning) {
            this->async_thread.get();
            this->Sort();
            this->menu_mode = MenuMode::LIST;
        }
    }
}

void App::UpdateList() {
    if (this->controller.B) {
        this->quit = true;
    } else if (this->controller.DOWN) { // move down
        if (this->index < (this->entries.size() - 1)) {
            this->index++;
            this->ypos += this->BOX_HEIGHT;
            if ((this->ypos + this->BOX_HEIGHT) > 646.f) {
                this->ypos -= this->BOX_HEIGHT;
                this->yoff = this->ypos - ((this->index - this->start - 1) * this->BOX_HEIGHT);
                this->start++;
            }
        }
    } else if (this->controller.UP) { // move up
        if (this->index != 0 && this->entries.size()) {
            this->index--;
            this->ypos -= this->BOX_HEIGHT;
            if (this->ypos < 86.f) {
                this->ypos += this->BOX_HEIGHT;
                this->yoff = this->ypos;
                this->start--;
            }
        }
    } else if (this->controller.R) {
        this->sort_type++;

        if (this->sort_type == std::to_underlying(SortType::MAX)) {
            this->sort_type = 0;
        }

        this->Sort();
    } 
}

#define SECONDS_PER_HOUR 3600
#define NANOSECONDS_PER_SECOND 1000000000
#define SECONDS_PER_MINUTE 60

// NOTE: there's a chance that we run out of memory here
// if the user has a *lot* of games installed.
void App::Scan(std::stop_token stop_token) {
    Result result{};
    s32 offset{};
    u64 jpeg_size{};
    size_t count{};
    NacpLanguageEntry* language_entry{};
    auto control_data = std::make_unique<NsApplicationControlData>();
    std::array<NsApplicationRecord, 30> record_list;
    ApplicationOccupiedSize size{};
    PdmPlayStatistics pdm_play_statistics[1] = {0};

    for (;;) {
        s32 record_count{};
        result = nsListApplicationRecord(record_list.data(), static_cast<s32>(record_list.size()), offset, &record_count);
        if (R_FAILED(result)) {
            LOG("failed to get record count\n");
            goto done;
        }

        // either we have ran out of games or we have no games installed.
        if (record_count == 0) {
            LOG("record count is 0\n");
            goto done;
        }

        for (auto i = 0; i < record_count && !stop_token.stop_requested(); i++) {
#ifndef NDEBUG
            LOG("Current application: %lX\n", record_list[i].application_id);
#endif

            bool corrupted_install = false;
            AppEntry entry;
            result = nsGetApplicationControlData(NsApplicationControlSource_Storage, record_list[i].application_id, control_data.get(), sizeof(NsApplicationControlData), &jpeg_size);
            // can fail with very messed up piracy installs, it would fail in ofw as well.
            if (R_FAILED(result)) {
                LOG("failed to get control data for %lX\n", record_list[i].application_id);
                corrupted_install = true;
            }

            result = nsGetApplicationDesiredLanguage(&control_data->nacp, &language_entry);
            if (R_FAILED(result)) {
                LOG("failed to get lang data\n");
                corrupted_install = true;
            }

            // get play statistics of application
            result = pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(record_list[i].application_id, this->account_uid, false, pdm_play_statistics);
            if (R_FAILED(result)) {
                LOG("Failed getting time of application. Result: %d\n", result);
                corrupted_install = true;
            }

            if (!corrupted_install) {
                u64 playtimeSeconds = pdm_play_statistics->playtime / NANOSECONDS_PER_SECOND;
                u64 playtimeHours = playtimeSeconds / SECONDS_PER_HOUR;
                playtimeSeconds -= playtimeHours * SECONDS_PER_HOUR;
                u64 playtimeMinutes = playtimeSeconds / SECONDS_PER_MINUTE;
                playtimeSeconds -= playtimeMinutes * SECONDS_PER_MINUTE;

                entry.name = language_entry->name;
                entry.author = language_entry->author;
                entry.display_version = control_data->nacp.display_version;
                entry.id = record_list[i].application_id;
                assert((jpeg_size - sizeof(NacpStruct)) > 0 && "jpeg size is smaller than the size of NacpStruct");
                entry.image = nvgCreateImageMem(this->vg, 0, control_data->icon, jpeg_size - sizeof(NacpStruct));
                entry.own_image = true; // we own it
                entry.playtime = Playtime(playtimeHours, playtimeMinutes, playtimeSeconds);
                this->entries.emplace_back(std::move(entry));
                count++;
            } else {
                entry.name = "Corrupted";
                entry.author = "Corrupted";
                entry.display_version = "Corrupted";
                entry.id = record_list[i].application_id;
                entry.image = this->default_icon_image;
                entry.own_image = false; // we don't own it
                entry.playtime = Playtime(0, 0, 0);
                this->entries.emplace_back(std::move(entry));
                this->has_corrupted = true;
                count++;
            }
        }

        // if we have less than count, then we are done!
        if (static_cast<size_t>(record_count) < record_list.size()) {
            goto done;
        }

        offset += record_count;
    }

done:
    std::scoped_lock lock{this->mutex};
    this->finished_scanning = true;
}

void App::SpawnScanThread() {
    // todo: handle errors
    this->async_thread = util::async([this](std::stop_token stop_token){
            this->Scan(stop_token);
        }
    );
}

void App::RequestAccountUid() {
    PselUserSelectionSettings pselUserSelectionSettings;

    memset(&pselUserSelectionSettings, 0, sizeof(pselUserSelectionSettings));
    memset(&this->account_uid, 0, sizeof(this->account_uid));

    Result result = pselShowUserSelector(&this->account_uid, &pselUserSelectionSettings);
    if (R_FAILED(result)) {
        LOG("Failed getting user id. Result: %d\n", result);
    }

#ifndef NDEBUG
    LOG("Selected user.\n");
#endif
}

App::App() {
    PlFontData font_standard, font_extended;
    plGetSharedFontByType(&font_standard, PlSharedFontType_Standard);
    plGetSharedFontByType(&font_extended, PlSharedFontType_NintendoExt);

    // Create the deko3d device
    this->device = dk::DeviceMaker{}.create();

    // Create the main queue
    this->queue = dk::QueueMaker{this->device}.setFlags(DkQueueFlags_Graphics).create();

    // Create the memory pools
    this->pool_images.emplace(device, DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image, 16*1024*1024);
    this->pool_code.emplace(device, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code, 128*1024);
    this->pool_data.emplace(device, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, 1*1024*1024);

    // Create the static command buffer and feed it freshly allocated memory
    this->cmdbuf = dk::CmdBufMaker{this->device}.create();
    const CMemPool::Handle cmdmem = this->pool_data->allocate(this->StaticCmdSize);
    this->cmdbuf.addMemory(cmdmem.getMemBlock(), cmdmem.getOffset(), cmdmem.getSize());

    // Create the framebuffer resources
    this->createFramebufferResources();

    this->renderer.emplace(1280, 720, this->device, this->queue, *this->pool_images, *this->pool_code, *this->pool_data);
    this->vg = nvgCreateDk(&*this->renderer, NVG_ANTIALIAS | NVG_STENCIL_STROKES);

    // not sure if these are meant to be deleted or not...
    int standard_font = nvgCreateFontMem(this->vg, "Standard", (unsigned char*)font_standard.address, font_standard.size, 0);
    int extended_font = nvgCreateFontMem(this->vg, "Extended", (unsigned char*)font_extended.address, font_extended.size, 0);

    if (standard_font < 0) {
        LOG("failed to load Standard font\n");
    }
    if (extended_font < 0) {
        LOG("failed to load extended font\n");
    }

    nvgAddFallbackFontId(this->vg, standard_font, extended_font);
    this->default_icon_image = nvgCreateImage(this->vg, "romfs:/default_icon.jpg", NVG_IMAGE_NEAREST);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&this->pad);
}

App::~App() {
    if (this->async_thread.valid()) {
        this->async_thread.request_stop();
        this->async_thread.get();
    }

    for (auto&p : this->entries) {
        if (p.own_image) {
            nvgDeleteImage(this->vg, p.image);
        }
    }

    nvgDeleteImage(this->vg, default_icon_image);
    this->destroyFramebufferResources();
    nvgDeleteDk(this->vg);
    this->renderer.reset();
}

void App::createFramebufferResources() {
    // Create layout for the depth buffer
    dk::ImageLayout layout_depthbuffer;
    dk::ImageLayoutMaker{device}
        .setFlags(DkImageFlags_UsageRender | DkImageFlags_HwCompression)
        .setFormat(DkImageFormat_S8)
        .setDimensions(1280, 720)
        .initialize(layout_depthbuffer);

    // Create the depth buffer
    this->depthBuffer_mem = this->pool_images->allocate(layout_depthbuffer.getSize(), layout_depthbuffer.getAlignment());
    this->depthBuffer.initialize(layout_depthbuffer, this->depthBuffer_mem.getMemBlock(), this->depthBuffer_mem.getOffset());

    // Create layout for the framebuffers
    dk::ImageLayout layout_framebuffer;
    dk::ImageLayoutMaker{device}
        .setFlags(DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression)
        .setFormat(DkImageFormat_RGBA8_Unorm)
        .setDimensions(1280, 720)
        .initialize(layout_framebuffer);

    // Create the framebuffers
    std::array<DkImage const*, NumFramebuffers> fb_array;
    const uint64_t fb_size  = layout_framebuffer.getSize();
    const uint32_t fb_align = layout_framebuffer.getAlignment();
    for (unsigned i = 0; i < fb_array.size(); i++) {
        // Allocate a framebuffer
        this->framebuffers_mem[i] = pool_images->allocate(fb_size, fb_align);
        this->framebuffers[i].initialize(layout_framebuffer, framebuffers_mem[i].getMemBlock(), framebuffers_mem[i].getOffset());

        // Generate a command list that binds it
        dk::ImageView colorTarget{ framebuffers[i] }, depthTarget{ depthBuffer };
        this->cmdbuf.bindRenderTargets(&colorTarget, &depthTarget);
        this->framebuffer_cmdlists[i] = cmdbuf.finishList();

        // Fill in the array for use later by the swapchain creation code
        fb_array[i] = &framebuffers[i];
    }

    // Create the swapchain using the framebuffers
    this->swapchain = dk::SwapchainMaker{device, nwindowGetDefault(), fb_array}.create();

    // Generate the main rendering cmdlist
    this->recordStaticCommands();
}

void App::destroyFramebufferResources() {
    // Return early if we have nothing to destroy
    if (!this->swapchain) {
        return;
    }

    this->queue.waitIdle();
    this->cmdbuf.clear();
    swapchain.destroy();

    // Destroy the framebuffers
    for (unsigned i = 0; i < NumFramebuffers; i++) {
        framebuffers_mem[i].destroy();
    }

    // Destroy the depth buffer
    this->depthBuffer_mem.destroy();
}

void App::recordStaticCommands() {
    // Initialize state structs with deko3d defaults
    dk::RasterizerState rasterizerState;
    dk::ColorState colorState;
    dk::ColorWriteState colorWriteState;
    dk::BlendState blendState;

    // Configure the viewport and scissor
    this->cmdbuf.setViewports(0, { { 0.0f, 0.0f, 1280, 720, 0.0f, 1.0f } });
    this->cmdbuf.setScissors(0, { { 0, 0, 1280, 720 } });

    // Clear the color and depth buffers
    this->cmdbuf.clearColor(0, DkColorMask_RGBA, 0.2f, 0.3f, 0.3f, 1.0f);
    this->cmdbuf.clearDepthStencil(true, 1.0f, 0xFF, 0);

    // Bind required state
    this->cmdbuf.bindRasterizerState(rasterizerState);
    this->cmdbuf.bindColorState(colorState);
    this->cmdbuf.bindColorWriteState(colorWriteState);

    this->render_cmdlist = this->cmdbuf.finishList();
}

} // namespace tj
