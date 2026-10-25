#ifdef VS_ONLINE
#include "online_transport.h"
#endif
// Dear ImGui: standalone example application for Windows API + DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#ifdef VS_NATIVE_START
#include "launcher_process_native.h"
#else
#include "launcher_process.h"
#endif
#include <future>
#ifdef VS_NATIVE_START
#include "equipment_menu.h"
#ifdef VS_ONLINE
#include "online_menu.h"
#include "embedded_catalogs.h"
#endif
#endif

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR arguments, int)
{
    wchar_t programPath[32768]={};
    GetModuleFileNameW(nullptr,programPath,32768);
    directory=programPath; directory=directory.substr(0,directory.find_last_of(L"\\")+1);
    load_choices(L"mechas.tsv",mechas); load_choices(L"stages.tsv",stages);
#ifdef VS_ONLINE
    if(mechas.empty())embedded_choices(201,mechas);
    if(stages.empty())embedded_choices(202,stages);
    status="Choose a player name and connect with a link code.";
    if(arguments&&!strcmp(arguments,"--verify-catalogs")){
     std::vector<Choice> a,b,c;
     return embedded_choices(201,a)&&a.size()==99&&embedded_choices(202,b)&&b.size()==220&&embedded_choices(203,c)&&c.size()==92?0:22;
    }
#endif
#ifdef VS_NATIVE_START
    music={{0xffffffffu,"Auto"},{0xfffffffeu,"Random"}};
    load_equipment();
    std::vector<Choice> tracks;load_choices(L"music.tsv",tracks);
#ifdef VS_ONLINE
    if(tracks.empty())embedded_choices(203,tracks);
#endif
    music.insert(music.end(),tracks.begin(),tracks.end());
#endif
    if(arguments && !strcmp(arguments,"--smoke")){
        std::ofstream log(directory+L"launcher-smoke.txt");
        start_battle(0,1,1);log<<status<<std::endl;
        for(int i=0;i<100 && battle;i++){Sleep(2000);poll_battle();log<<status<<std::endl;}
        if(battle)CloseHandle(battle);
        return 0;
    }
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"BaldrVSLauncher", nullptr };
#ifdef VS_NATIVE_START
    wc.hIcon=::LoadIconW(wc.hInstance,MAKEINTRESOURCEW(101));
    wc.hIconSm=(HICON)::LoadImageW(wc.hInstance,MAKEINTRESOURCEW(101),IMAGE_ICON,
        ::GetSystemMetrics(SM_CXSMICON),::GetSystemMetrics(SM_CYSMICON),LR_SHARED);
#endif
    ::RegisterClassExW(&wc);
    const wchar_t* windowTitle=
#ifdef VS_ONLINE
        L"BALDR SKY DiveX - Online VS Development";
#elif defined(VS_NATIVE_START)
        L"BALDR SKY DiveX - Native VS Launcher";
#else
        L"BALDR SKY DiveX - VS Launcher";
#endif
    const int windowHeight=
#ifdef VS_ONLINE
        820;
#elif defined(VS_NATIVE_START)
        490;
#else
        380;
#endif
    const int windowWidth=
#ifdef VS_ONLINE
        1020;
#else
        660;
#endif
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, windowTitle, WS_OVERLAPPEDWINDOW, 100, 100, (int)(windowWidth * main_scale), (int)(windowHeight * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename=nullptr;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\meiryo.ttc",16.0f);
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // Launcher navigation uses mouse and keyboard. Controllers belong to the game.

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    
    
    ImVec4 clear_color = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);

    // Main loop
    bool done = false;
    static std::future<void> startup;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;
        if(startup.valid() && startup.wait_for(std::chrono::seconds(0))==std::future_status::ready)startup.get();
        if(!startup.valid())poll_battle();

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0,0));
        float setupWidth=io.DisplaySize.x;
#ifdef VS_ONLINE
        const float chatWidth=310*main_scale;
        setupWidth-=chatWidth+8*main_scale;
#endif
        ImGui::SetNextWindowSize(ImVec2(setupWidth,io.DisplaySize.y));
        ImGui::Begin("VS Setup",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
        static int p1=0,p2=1,stage=0;
        ImGui::SetCursorPos(ImVec2(42*main_scale,42*main_scale));
        ImGui::BeginGroup();ImGui::TextUnformatted("1P MEKA SELECT");
        ImGui::SetNextItemWidth(240*main_scale);
        choice_combo("##p1",p1,mechas);
#ifdef VS_NATIVE_START
        ImGui::BeginDisabled(startup.valid()||battle||equipment_catalog.empty());
        if(ImGui::Button("1P Equipment",ImVec2(240*main_scale,0)))open_equipment_editor(0,mechas[p1].id);
        ImGui::EndDisabled();
#endif
        ImGui::EndGroup();
        ImGui::SameLine(340*main_scale);ImGui::BeginGroup();
        ImGui::TextUnformatted("2P MEKA SELECT");ImGui::SetNextItemWidth(240*main_scale);
        choice_combo("##p2",p2,mechas);
#ifdef VS_NATIVE_START
        ImGui::BeginDisabled(startup.valid()||battle||equipment_catalog.empty());
        if(ImGui::Button("2P Equipment",ImVec2(240*main_scale,0)))open_equipment_editor(1,mechas[p2].id);
        ImGui::EndDisabled();
#endif
        ImGui::EndGroup();
        ImGui::SetCursorPos(ImVec2(
#ifdef VS_NATIVE_START
          42
#else
          200
#endif
          *main_scale,155*main_scale));
        ImGui::BeginGroup();ImGui::TextUnformatted("STAGE SELECT");ImGui::SetNextItemWidth(240*main_scale);
        choice_combo("##stage",stage,stages);ImGui::EndGroup();
#ifdef VS_NATIVE_START
        static int track=0,preset=0;
        ImGui::SameLine(340*main_scale);ImGui::BeginGroup();
        ImGui::TextUnformatted("MUSIC");ImGui::SetNextItemWidth(240*main_scale);
        choice_combo("##music",track,music);ImGui::EndGroup();
        const char* presets[]={"FT2","FT3","FT5","Unlimited"};
        const unsigned targets[]={2,3,5,0};
        ImGui::SetCursorPos(ImVec2(200*main_scale,240*main_scale));
        ImGui::BeginGroup();ImGui::TextUnformatted("MATCH SET");ImGui::SetNextItemWidth(240*main_scale);
        ImGui::Combo("##set",&preset,presets,4);ImGui::EndGroup();
        ImGui::SetCursorPos(ImVec2(240*main_scale,325*main_scale));
#else
        ImGui::SetCursorPos(ImVec2(240*main_scale,240*main_scale));
#endif
        ImGui::BeginDisabled(startup.valid() || battle || mechas.empty() || stages.empty()
#ifdef VS_ONLINE
            || (online_mode && !online::native_rollback_verified)
#endif
        );
        if(ImGui::Button("START BATTLE",ImVec2(160*main_scale,40*main_scale))){
            unsigned first=mechas[p1].id,second=mechas[p2].id,map=stages[stage].id;
#ifdef VS_NATIVE_START
            unsigned wins=targets[preset],song=music[track].id;
            VSEquipmentConfig equipment;equipment.p1=first;equipment.p2=second;
            equipment.players[0]=equipment_profiles.get(0,first,equipment_catalog);equipment.players[1]=equipment_profiles.get(1,second,equipment_catalog);
            startup=std::async(std::launch::async,[first,second,map,wins,song,equipment]{start_battle(first,second,map,wins,song,&equipment);});
#else
            startup=std::async(std::launch::async,[first,second,map]{start_battle(first,second,map);});
#endif
        }
        ImGui::EndDisabled();
#ifdef VS_ONLINE
        if(online_mode&&ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))ImGui::SetTooltip("Online battles are in development. Connect and check Ready for the netplay test. Uncheck Online setup for local VS.");
#endif
        ImGui::SameLine(420*main_scale);
        if(ImGui::Button("CLOSE GAME",ImVec2(160*main_scale,40*main_scale)))
#ifdef VS_NATIVE_START
            close_game_from_launcher();
#else
            if(battle){TerminateProcess(battle,0);CloseHandle(battle);battle=nullptr;status="Game closed.";}
            else status="No game is running from this launcher.";
#endif
        ImGui::SetCursorPos(ImVec2(42*main_scale,
#ifdef VS_NATIVE_START
          390
#else
          302
#endif
          *main_scale));
        ImGui::PushTextWrapPos(setupWidth-30*main_scale);
        const char* setupStatus=status.c_str();
#ifdef VS_ONLINE
        if(!battle&&!startup.valid()){
         if(online_mode)setupStatus="Online preview: connect, chat and check Ready. Start Battle is unavailable until native online battles are implemented.";
         else if(GetFileAttributesW((directory+L"BaldrSky.VSNative.exe").c_str())==INVALID_FILE_ATTRIBUTES||GetFileAttributesW((directory+L"BaldrVSPlugin.dll").c_str())==INVALID_FILE_ATTRIBUTES)
          setupStatus="Local VS needs BaldrSky.VSNative.exe and BaldrVSPlugin.dll beside this launcher. Copy the matching pair from your working PC or install the native VS build.";
        }
#endif
        ImGui::TextUnformatted(startup.valid()?"Starting game and preparing VS...":setupStatus);ImGui::PopTextWrapPos();
#ifdef VS_NATIVE_START
        if(equipment_catalog.empty()
#ifdef VS_ONLINE
          && !online_mode
#endif
        ){
         ImGui::BeginDisabled(startup.valid()||battle);
         if(ImGui::Button("Load equipment catalog")){
          equipment_scan=true;
          startup=std::async(std::launch::async,[]{VSEquipmentConfig scan;scan.catalogOnly=1;start_battle(0,1,1,0,0xffffffffu,&scan);});
         }ImGui::EndDisabled();
        }
        equipment_editor(main_scale);
#ifdef VS_ONLINE
        online_panel(main_scale);
#endif
#endif
        ImGui::End();
#ifdef VS_ONLINE
        online_chat_panel(main_scale,setupWidth+8*main_scale,chatWidth);
#endif
        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

#ifdef VS_ONLINE
    online_cancel=true;if(online_code_task.valid())online_code_task.get();if(online_task.valid())online_task.get();
#endif
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

