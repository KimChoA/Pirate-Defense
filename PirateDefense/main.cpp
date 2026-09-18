// ============================================================================
// [김초아 담당] Pirate Defense 메인 SFML 시스템 / UI / 네트워크 연동
// - SFML 기반 메인 화면 및 게임 상태 전환 구조
// - 방 생성·참가·대기실 UI와 방 코드 처리
// - TCP/IP NetworkManager 연동 및 멀티플레이 진입 흐름
// - 실시간 채팅 입력/표시 시스템 통합
// - 게임 UI/HUD 및 전체 화면 디자인 통합
// ============================================================================
#include <SFML/Graphics.hpp>
#include <optional>
#include <random>
#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <windows.h>
#include <imm.h>
#pragma comment(lib, "imm32.lib")
#include "NetworkManager.h"
#include "SingleGame.h"
#include "Render.hpp"
#include "MultiGame.h"

// [김초아 담당] 메인 화면 상태 머신
// 닉네임 입력 -> 메인 메뉴 -> 방 생성/참가 -> 로비 -> 싱글/멀티 게임 흐름을 관리합니다.
enum class GameState
{
    NicknameInput,
    MainMenu,
    CreateRoom,
    JoinRoom,
    SinglePlayer,
    Lobby,
    MultiGame
};

// [김초아 담당] 방 코드 기반 멀티플레이 입장 지원
std::string generateRoomCode()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    const std::string chars =
        "ABCDEFGHJKLMNPQRSTUVWXYZ123456789";

    std::uniform_int_distribution<int> dist(
        0,
        static_cast<int>(chars.size()) - 1
    );

    std::string code;

    for (int i = 0; i < 6; i++)
    {
        code += chars[dist(gen)];
    }

    return code;
}

bool copyToClipboard(const std::string& text)
{
    if (!OpenClipboard(nullptr))
    {
        return false;
    }

    EmptyClipboard();

    HGLOBAL hMemory =
        GlobalAlloc(
            GMEM_MOVEABLE,
            text.size() + 1
        );

    if (hMemory == nullptr)
    {
        CloseClipboard();
        return false;
    }

    char* memory =
        static_cast<char*>(
            GlobalLock(hMemory)
            );

    if (memory == nullptr)
    {
        GlobalFree(hMemory);
        CloseClipboard();
        return false;
    }

    std::memcpy(
        memory,
        text.c_str(),
        text.size() + 1
    );

    GlobalUnlock(hMemory);

    if (SetClipboardData(
        CF_TEXT,
        hMemory
    ) == nullptr)
    {
        GlobalFree(hMemory);
        CloseClipboard();
        return false;
    }

    CloseClipboard();

    return true;
}

std::string getRoomCodeFromClipboard()
{
    std::string result;

    if (!OpenClipboard(nullptr))
    {
        return result;
    }

    HANDLE hUnicode =
        GetClipboardData(CF_UNICODETEXT);

    if (hUnicode != nullptr)
    {
        const wchar_t* text =
            static_cast<const wchar_t*>(
                GlobalLock(hUnicode)
                );

        if (text != nullptr)
        {
            for (
                std::size_t i = 0;
                text[i] != L'\0' &&
                result.size() < 6;
                ++i
                )
            {
                wchar_t ch = text[i];

                if (ch >= L'a' &&
                    ch <= L'z')
                {
                    ch =
                        ch - L'a' + L'A';
                }

                if (
                    (ch >= L'A' &&
                        ch <= L'Z') ||
                    (ch >= L'0' &&
                        ch <= L'9')
                    )
                {
                    result +=
                        static_cast<char>(ch);
                }
            }

            GlobalUnlock(hUnicode);
        }
    }

    if (result.empty())
    {
        HANDLE hText =
            GetClipboardData(CF_TEXT);

        if (hText != nullptr)
        {
            const char* text =
                static_cast<const char*>(
                    GlobalLock(hText)
                    );

            if (text != nullptr)
            {
                for (
                    std::size_t i = 0;
                    text[i] != '\0' &&
                    result.size() < 6;
                    ++i
                    )
                {
                    char ch = text[i];

                    if (ch >= 'a' &&
                        ch <= 'z')
                    {
                        ch =
                            static_cast<char>(
                                ch - 'a' + 'A'
                                );
                    }

                    if (
                        (ch >= 'A' &&
                            ch <= 'Z') ||
                        (ch >= '0' &&
                            ch <= '9')
                        )
                    {
                        result += ch;
                    }
                }

                GlobalUnlock(hText);
            }
        }
    }

    CloseClipboard();

    return result;
}

void drawHover(
    sf::RenderWindow& window,
    sf::RectangleShape& hover,
    const sf::RectangleShape& button,
    const sf::Vector2f& mousePos)
{
    if (button.getGlobalBounds().contains(mousePos))
    {
        hover.setSize(button.getSize());
        hover.setPosition(button.getPosition());

        window.draw(hover);
    }
}

void centerText(
    sf::Text& text,
    float x,
    float y,
    float width,
    float height)
{
    sf::FloatRect bounds =
        text.getLocalBounds();

    text.setPosition({
        x +
        (width - bounds.size.x) / 2.f -
        bounds.position.x,

        y +
        (height - bounds.size.y) / 2.f -
        bounds.position.y
        });
}

void leftCenterText(
    sf::Text& text,
    float x,
    float y,
    float height)
{
    sf::FloatRect bounds =
        text.getLocalBounds();

    text.setPosition({
        x - bounds.position.x,

        y +
        (height - bounds.size.y) / 2.f -
        bounds.position.y
        });
}

std::string toUtf8String(const sf::String& value)
{
    sf::U8String utf8 = value.toUtf8();
    return std::string(utf8.begin(), utf8.end());
}

sf::Vector2f windowToGamePoint(
    const sf::RenderWindow& window,
    sf::Vector2i pixel,
    bool integerScale)
{
    const auto size = window.getSize();

    float scale = std::min(
        size.x / 640.f,
        size.y / 360.f
    );

    if (integerScale && scale >= 1.f)
    {
        scale = std::floor(scale);
    }

    if (scale <= 0.f)
    {
        scale = 1.f;
    }

    const float offsetX =
        (size.x - 640.f * scale) / 2.f;
    const float offsetY =
        (size.y - 360.f * scale) / 2.f;

    return {
        (static_cast<float>(pixel.x) - offsetX) / scale,
        (static_cast<float>(pixel.y) - offsetY) / scale
    };
}

int shopItemAt(sf::Vector2f point)
{
    for (int i = 0; i < dw::ShopItemCount; ++i)
    {
        const int col = i % 5;
        const int row = i / 5;

        const float x = 34.f + col * 116.f;
        const float y = 108.f + row * 74.f;

        if (point.x >= x && point.x <= x + 108.f &&
            point.y >= y && point.y <= y + 64.f)
        {
            return i;
        }
    }

    return -1;
}

int shopVoteChoiceAt(sf::Vector2f point)
{
    if (point.x >= 330.f && point.x <= 418.f &&
        point.y >= 255.f && point.y <= 281.f)
    {
        return 1;
    }

    if (point.x >= 426.f && point.x <= 514.f &&
        point.y >= 255.f && point.y <= 281.f)
    {
        return 0;
    }

    return -1;
}

bool shopReadyAt(sf::Vector2f point)
{
    return
        point.x >= 500.f && point.x <= 606.f &&
        point.y >= 286.f && point.y <= 314.f;
}

bool playerNearAmmoRack(const dw::Game& game, int playerId)
{
    if (playerId < 0 || playerId >= dw::MaxPlayers)
        return false;

    const auto& player = game.players[playerId];

    return
        player.active &&
        player.held < 0 &&
        dw::dist(player.p, dw::AmmoPoint) < 32.f;
}


// ============================================================================
// [김초아 담당] Windows IME 기반 실시간 멀티 채팅 입력기
// ----------------------------------------------------------------------------
// 게임 HWND를 직접 서브클래싱해 IME 메시지를 받습니다. 숨은 EDIT 생성/표시와
// SetFocus 왕복을 하지 않으므로 한/영 전환이나 조합 시작이 게임 창의 활성 상태,
// 스왑 체인, 전체 화면 재도장을 건드리지 않습니다.
// ============================================================================
class NativeChatInput
{
private:
    HWND parentWindow = nullptr;
    WNDPROC oldWindowProc = nullptr;
    HIMC imeContext = nullptr;
    bool imeDetached = false;
    bool koreanMode = false;

    bool active = false;
    bool submitRequested = false;
    bool deactivateRequested = false;

    std::wstring committedText;
    std::wstring compositionText;

    void detachImeContext()
    {
        if (
            parentWindow == nullptr ||
            !IsWindow(parentWindow) ||
            imeDetached
            )
        {
            return;
        }

        // 채팅이 꺼진 동안에는 게임 HWND에서 IME를 분리한다. 이렇게 해야
        // 한/영 키가 Windows IME 상태창(A/가)을 띄우거나 OpenGL 창을 다시
        // 그리게 만들지 않는다. 기존 HIMC는 보관했다가 Tab 채팅 때 재사용한다.
        HIMC previous = ImmAssociateContext(parentWindow, nullptr);
        if (previous != nullptr)
            imeContext = previous;
        imeDetached = true;
    }

    void attachImeContext()
    {
        if (
            parentWindow == nullptr ||
            !IsWindow(parentWindow) ||
            !imeDetached
            )
        {
            return;
        }

        ImmAssociateContext(parentWindow, imeContext);
        imeDetached = false;
    }

    void applyKoreanMode()
    {
        if (
            parentWindow == nullptr ||
            !IsWindow(parentWindow) ||
            imeDetached
            )
        {
            return;
        }

        HIMC ime = ImmGetContext(parentWindow);
        if (ime != nullptr)
        {
            if ((ImmGetOpenStatus(ime) != FALSE) != koreanMode)
                ImmSetOpenStatus(ime, koreanMode ? TRUE : FALSE);
            ImmReleaseContext(parentWindow, ime);
        }
    }

    static bool isHangulToggleKey(HWND hwnd, WPARAM wParam)
    {
        UINT virtualKey = static_cast<UINT>(wParam);
        if (virtualKey == VK_PROCESSKEY)
            virtualKey = ImmGetVirtualKey(hwnd);

        return virtualKey == VK_HANGUL || virtualKey == VK_KANA;
    }

    static const wchar_t* propertyName()
    {
        return L"PirateDefense.NativeChatInput";
    }

    void appendCommitted(const std::wstring& value)
    {
        if (value.empty() || committedText.size() >= 80)
            return;

        const std::size_t remaining = 80 - committedText.size();
        committedText.append(
            value.data(),
            std::min(remaining, value.size())
        );
    }

    void eraseLastCharacter()
    {
        if (committedText.empty())
            return;

        // UTF-16 surrogate pair까지 안전하게 한 글자 단위로 삭제합니다.
        const wchar_t last = committedText.back();
        committedText.pop_back();

        if (
            last >= 0xDC00 && last <= 0xDFFF &&
            !committedText.empty()
            )
        {
            const wchar_t high = committedText.back();
            if (high >= 0xD800 && high <= 0xDBFF)
                committedText.pop_back();
        }
    }

    void pasteUnicodeText()
    {
        if (!OpenClipboard(parentWindow))
            return;

        HANDLE handle = GetClipboardData(CF_UNICODETEXT);
        if (handle != nullptr)
        {
            const wchar_t* text = static_cast<const wchar_t*>(
                GlobalLock(handle)
                );

            if (text != nullptr)
            {
                appendCommitted(std::wstring(text));
                GlobalUnlock(handle);
            }
        }

        CloseClipboard();
    }

    void updateComposition(HWND hwnd, LPARAM flags)
    {
        HIMC ime = ImmGetContext(hwnd);
        if (ime == nullptr)
            return;

        auto readString = [&](DWORD index) -> std::wstring
        {
            const LONG byteCount = ImmGetCompositionStringW(
                ime, index, nullptr, 0);
            if (byteCount <= 0)
                return {};

            std::wstring value(
                static_cast<std::size_t>(byteCount) / sizeof(wchar_t),
                L'\0');
            ImmGetCompositionStringW(
                ime, index, value.data(), byteCount);
            return value;
        };

        if ((flags & GCS_RESULTSTR) != 0)
        {
            appendCommitted(readString(GCS_RESULTSTR));
            compositionText.clear();
        }

        if ((flags & GCS_COMPSTR) != 0)
            compositionText = readString(GCS_COMPSTR);

        ImmReleaseContext(hwnd, ime);
    }

    void finishCompositionForSubmit(HWND hwnd)
    {
        if (compositionText.empty())
            return;

        // Enter 직전의 조합 문자열은 이미 완성된 UTF-16 음절입니다. 직접 확정한 뒤
        // IME 쪽 조합만 취소해 결과 문자열이 두 번 들어오는 것을 막습니다.
        appendCommitted(compositionText);
        compositionText.clear();

        HIMC ime = ImmGetContext(hwnd);
        if (ime != nullptr)
        {
            ImmNotifyIME(ime, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
            ImmReleaseContext(hwnd, ime);
        }
    }

    void cancelComposition()
    {
        compositionText.clear();
        if (parentWindow == nullptr || !IsWindow(parentWindow))
            return;

        HIMC ime = ImmGetContext(parentWindow);
        if (ime != nullptr)
        {
            ImmNotifyIME(ime, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
            ImmReleaseContext(parentWindow, ime);
        }
    }

    static LRESULT CALLBACK windowProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
    {
        auto* self = reinterpret_cast<NativeChatInput*>(
            GetPropW(hwnd, propertyName())
            );

        // Input-language/IME UI updates can ask Windows to erase the OpenGL client
        // area before SFML presents the next frame. The renderer covers the complete
        // client every frame, so accepting that erase only creates a visible flash.
        if (self != nullptr && message == WM_ERASEBKGND)
            return 1;

        // 조합 문자열은 게임이 직접 그리므로 기본 IME의 조합/상태 UI는 열지
        // 않는다. 특히 IME가 활성화될 때 왼쪽 위에 나타나는 A/가 상태창을 막는다.
        if (self != nullptr && message == WM_IME_SETCONTEXT)
        {
            const LPARAM imeUiFlags = (wParam != FALSE) ? 0 : lParam;
            if (self->oldWindowProc != nullptr)
            {
                return CallWindowProcW(
                    self->oldWindowProc,
                    hwnd,
                    message,
                    wParam,
                    imeUiFlags);
            }
            return DefWindowProcW(hwnd, message, wParam, imeUiFlags);
        }

        if (self != nullptr && message == WM_IME_NOTIFY)
        {
            switch (wParam)
            {
            case IMN_OPENSTATUSWINDOW:
            case IMN_CLOSESTATUSWINDOW:
            case IMN_SETSTATUSWINDOWPOS:
            case IMN_SETOPENSTATUS:
            case IMN_SETCONVERSIONMODE:
                return 0;
            default:
                break;
            }
        }

        if (
            self != nullptr &&
            (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) &&
            isHangulToggleKey(hwnd, wParam)
            )
        {
            // 자동 반복은 무시하고 실제 누름마다 한 번만 전환한다. 비활성 채팅일
            // 때도 상태는 기억하되 OS에 키를 넘기지 않아 상태창/화면 깜빡임을 막는다.
            if ((lParam & (static_cast<LPARAM>(1) << 30)) == 0)
            {
                self->koreanMode = !self->koreanMode;
                if (self->active)
                    self->applyKoreanMode();
            }
            return 0;
        }

        if (self != nullptr && self->active)
        {
            if (message == WM_IME_STARTCOMPOSITION)
                self->compositionText.clear();

            if (message == WM_IME_COMPOSITION)
                self->updateComposition(hwnd, lParam);

            if (message == WM_IME_ENDCOMPOSITION)
                self->compositionText.clear();

            if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
            {
                if (wParam == VK_RETURN)
                {
                    self->finishCompositionForSubmit(hwnd);
                    self->submitRequested = true;
                    return 0;
                }

                if (wParam == VK_TAB)
                {
                    self->cancelComposition();
                    self->deactivateRequested = true;
                    return 0;
                }
            }

            if (message == WM_CHAR)
            {
                if (wParam == L'\b')
                {
                    if (self->compositionText.empty())
                        self->eraseLastCharacter();
                    return 0;
                }

                if (wParam == L'\r' || wParam == L'\n' || wParam == L'\t')
                    return 0;

                // 한글은 GCS_RESULTSTR, 영문/숫자/기호는 WM_CHAR에서 한 번만 받습니다.
                if (wParam >= 32 && wParam < 0x80)
                {
                    self->appendCommitted(
                        std::wstring(1, static_cast<wchar_t>(wParam))
                    );
                }

                return 0;
            }

            if (message == WM_PASTE)
            {
                self->pasteUnicodeText();
                return 0;
            }

            if (message == WM_GETDLGCODE)
            {
                return DLGC_WANTALLKEYS | DLGC_WANTCHARS | DLGC_WANTTAB;
            }
        }

        if (self != nullptr && self->oldWindowProc != nullptr)
            return CallWindowProcW(
                self->oldWindowProc, hwnd, message, wParam, lParam);

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    void detachWindow()
    {
        if (parentWindow != nullptr && IsWindow(parentWindow))
        {
            // 살아 있는 창에서 빠질 때는 원래 IME 컨텍스트를 돌려놓는다.
            if (imeDetached)
            {
                ImmAssociateContext(parentWindow, imeContext);
                imeDetached = false;
            }

            if (reinterpret_cast<WNDPROC>(
                GetWindowLongPtrW(parentWindow, GWLP_WNDPROC)) == &windowProc &&
                oldWindowProc != nullptr)
            {
                SetWindowLongPtrW(
                    parentWindow,
                    GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(oldWindowProc));
            }
            RemovePropW(parentWindow, propertyName());
        }

        oldWindowProc = nullptr;
        parentWindow = nullptr;
        imeContext = nullptr;
        imeDetached = false;
        active = false;
        submitRequested = false;
        deactivateRequested = false;
        compositionText.clear();
    }

public:
    NativeChatInput() = default;

    ~NativeChatInput()
    {
        detachWindow();
    }

    bool ensureAttached(sf::RenderWindow& window)
    {
        HWND newParent = window.getNativeHandle();

        if (newParent == nullptr)
            return false;

        if (
            IsWindow(parentWindow) &&
            parentWindow == newParent
            )
        {
            return true;
        }

        detachWindow();
        parentWindow = newParent;

        if (!SetPropW(parentWindow, propertyName(), this))
        {
            parentWindow = nullptr;
            return false;
        }

        oldWindowProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(
                parentWindow,
                GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(&NativeChatInput::windowProc)
            )
            );

        if (oldWindowProc == nullptr)
        {
            RemovePropW(parentWindow, propertyName());
            parentWindow = nullptr;
            return false;
        }

        // 최초 연결 시의 한/영 상태와 HIMC를 저장하고, 채팅이 꺼진 기본 상태에서는
        // 즉시 분리한다. 프레임마다 생성/해제하지 않고 채팅 경계에서만 연결한다.
        imeContext = ImmGetContext(parentWindow);
        if (imeContext != nullptr)
        {
            koreanMode = ImmGetOpenStatus(imeContext) != FALSE;
            ImmReleaseContext(parentWindow, imeContext);
        }
        detachImeContext();

        return true;
    }

    void setActive(sf::RenderWindow& window, bool enable)
    {
        if (!ensureAttached(window))
            return;

        submitRequested = false;
        deactivateRequested = false;

        if (enable)
        {
            attachImeContext();
            active = true;
            applyKoreanMode();

            // 포커스는 게임 HWND에 그대로 둔다. 조합/후보 창만 채팅 패널 근처에 둔다.
            HIMC ime = ImmGetContext(parentWindow);
            if (ime != nullptr)
            {
                COMPOSITIONFORM composition{};
                composition.dwStyle = CFS_POINT;
                composition.ptCurrentPos = { 28, 646 };
                ImmSetCompositionWindow(ime, &composition);

                CANDIDATEFORM candidate{};
                candidate.dwIndex = 0;
                candidate.dwStyle = CFS_CANDIDATEPOS;
                candidate.ptCurrentPos = { 28, 616 };
                ImmSetCandidateWindow(ime, &candidate);
                ImmReleaseContext(parentWindow, ime);
            }
        }
        else
        {
            if (active)
            {
                HIMC ime = ImmGetContext(parentWindow);
                if (ime != nullptr)
                {
                    koreanMode = ImmGetOpenStatus(ime) != FALSE;
                    ImmReleaseContext(parentWindow, ime);
                }
                cancelComposition();
            }
            active = false;
            detachImeContext();
        }
    }

    bool isActive() const
    {
        return active;
    }

    void pumpMessages()
    {
        // SFML pollEvent가 같은 게임 HWND의 메시지를 처리하며 windowProc가 먼저 받습니다.
    }

    bool takeSubmitRequest()
    {
        const bool result = submitRequested;
        submitRequested = false;
        return result;
    }

    bool takeDeactivateRequest()
    {
        const bool result = deactivateRequested;
        deactivateRequested = false;
        return result;
    }

    sf::String getText() const
    {
        std::wstring visible = committedText;
        const std::size_t remaining = visible.size() < 80
            ? 80 - visible.size()
            : 0;
        visible.append(
            compositionText.data(),
            std::min(remaining, compositionText.size()));
        return sf::String(visible);
    }

    void setText(const sf::String& text)
    {
        committedText = text.toWideString();
        compositionText.clear();

        if (committedText.size() > 80)
            committedText.resize(80);
    }

    void clear()
    {
        committedText.clear();
        compositionText.clear();
    }
};

// [김초아 담당] 전체 화면형 SFML 게임 창 구성 및 IME 호환 처리
void createBorderlessGameWindow(sf::RenderWindow& window)
{
    // Exclusive fullscreen is forced out of the compositor when the Windows IME
    // language indicator appears. A desktop-sized borderless window keeps the same
    // full-screen presentation without a display-mode switch or focus transition.
    window.create(
        sf::VideoMode::getDesktopMode(),
        "Pirate Defense",
        sf::Style::None,
        sf::State::Windowed
    );
    window.setPosition({0, 0});
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);
}

// ============================================================================
// [김초아 담당] 메인 SFML 시스템 통합
// 화면/입력/UI/NetworkManager/SingleGame/MultiGame/Renderer를 하나의 실행 흐름으로 연결합니다.
// ============================================================================
int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    sf::RenderWindow window(
        sf::VideoMode({ 1280, 720 }),
        "Pirate Defense"
    );

    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);

    GameState currentState =
        GameState::NicknameInput;

    NetworkManager networkManager;

    dw::Game singleGame;
    dw::SingleGame singleGameMode;
    dw::Renderer singleRenderer;
    dw::Screen singleScreen;
    dw::Input singleInput;

    sf::RenderTexture singleCanvas({
        640,
        360
        });

    singleCanvas.setSmooth(false);

    sf::Clock singleGameClock;

    dw::Game multiGame;
    dw::MultiGame multiGameMode(networkManager);
    dw::Screen multiScreen;
    dw::Input multiInput;
    dw::Renderer multiRenderer;

    sf::RenderTexture multiCanvas({
        640,
        360
        });

    multiCanvas.setSmooth(false);

    sf::Clock multiGameClock;

    // [김초아 담당] 멀티플레이 실시간 채팅 상태 및 네이티브 IME 입력 연결
    bool multiChatActive = false;
    sf::String multiChatInput;
    NativeChatInput nativeChatInput;
    nativeChatInput.ensureAttached(window);

    sf::Font font;

    if (!font.openFromFile(
        "C:/Windows/Fonts/malgun.ttf"))
    {
        return -1;
    }

    sf::Texture nicknameTexture;

    if (!nicknameTexture.loadFromFile(
        "assets/images/nickname_background.png"))
    {
        return -1;
    }

    sf::Sprite nicknameBackground(
        nicknameTexture
    );

    nicknameBackground.setScale({
        1280.f /
        static_cast<float>(
            nicknameTexture.getSize().x
        ),

        720.f /
        static_cast<float>(
            nicknameTexture.getSize().y
        )
        });

    sf::Texture mainMenuTexture;

    if (!mainMenuTexture.loadFromFile(
        "assets/images/mainmenu_background.png"))
    {
        return -1;
    }

    sf::Sprite mainMenuBackground(
        mainMenuTexture
    );

    mainMenuBackground.setScale({
        1280.f /
        static_cast<float>(
            mainMenuTexture.getSize().x
        ),

        720.f /
        static_cast<float>(
            mainMenuTexture.getSize().y
        )
        });

    sf::Texture createRoomTexture;

    if (!createRoomTexture.loadFromFile(
        "assets/images/createroom_background.png"))
    {
        return -1;
    }

    sf::Sprite createRoomBackground(
        createRoomTexture
    );

    createRoomBackground.setScale({
        1280.f /
        static_cast<float>(
            createRoomTexture.getSize().x
        ),

        720.f /
        static_cast<float>(
            createRoomTexture.getSize().y
        )
        });

    sf::Texture joinRoomTexture;

    if (!joinRoomTexture.loadFromFile(
        "assets/images/joinroom_background.png"))
    {
        return -1;
    }

    sf::Sprite joinRoomBackground(
        joinRoomTexture
    );

    joinRoomBackground.setScale({
        1280.f /
        static_cast<float>(
            joinRoomTexture.getSize().x
        ),

        720.f /
        static_cast<float>(
            joinRoomTexture.getSize().y
        )
        });

    sf::Texture lobbyBackgroundTexture;

    if (!lobbyBackgroundTexture.loadFromFile(
        "assets/images/lobby_background.png"))
    {
        return -1;
    }

    sf::Sprite lobbyBackground(
        lobbyBackgroundTexture
    );

    lobbyBackground.setScale({
        1280.f /
        static_cast<float>(
            lobbyBackgroundTexture.getSize().x
        ),

        720.f /
        static_cast<float>(
            lobbyBackgroundTexture.getSize().y
        )
        });

    sf::RectangleShape buttonHover;

    buttonHover.setFillColor(
        sf::Color(
            255,
            225,
            130,
            55
        )
    );

    sf::String nickname;

    bool nicknameInputActive =
        true;

    sf::RectangleShape nicknameInputArea({
        338.f,
        68.f
        });

    nicknameInputArea.setPosition({
        438.f,
        354.f
        });

    nicknameInputArea.setFillColor(
        sf::Color::Transparent
    );

    sf::Text nicknameText(font);

    nicknameText.setCharacterSize(22);

    nicknameText.setFillColor(
        sf::Color::White
    );

    nicknameText.setPosition({
        468.f,
        374.f
        });

    sf::RectangleShape nicknameCursor({
        2.f,
        25.f
        });

    nicknameCursor.setFillColor(
        sf::Color::White
    );

    sf::Clock nicknameCursorClock;

    sf::RectangleShape confirmButton({
        260.f,
        72.f
        });

    confirmButton.setPosition({
        510.f,
        442.f
        });

    confirmButton.setFillColor(
        sf::Color::Transparent
    );

    sf::Text welcomeText(font);

    welcomeText.setCharacterSize(22);

    welcomeText.setFillColor(
        sf::Color(
            245,
            232,
            200
        )
    );

    sf::RectangleShape createRoomButton({
        345.f,
        67.f
        });

    createRoomButton.setPosition({
        468.f,
        300.f
        });

    createRoomButton.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape joinRoomButton({
        345.f,
        67.f
        });

    joinRoomButton.setPosition({
        468.f,
        374.f
        });

    joinRoomButton.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape singleButton({
        345.f,
        67.f
        });

    singleButton.setPosition({
        468.f,
        448.f
        });

    singleButton.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape exitButton({
        345.f,
        67.f
        });

    exitButton.setPosition({
        468.f,
        522.f
        });

    exitButton.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape mainBackButton({
        245.f,
        60.f
        });

    mainBackButton.setPosition({
        36.f,
        620.f
        });

    mainBackButton.setFillColor(
        sf::Color::Transparent
    );

    // 메인 메뉴에서 바로 열 수 있는 별도 조작법 버튼
    bool mainHelpVisible = false;

    sf::RectangleShape mainHelpButton({
        190.f,
        54.f
        });

    mainHelpButton.setPosition({
        1045.f,
        622.f
        });

    mainHelpButton.setFillColor(
        sf::Color(18, 38, 52, 225)
    );
    mainHelpButton.setOutlineThickness(2.f);
    mainHelpButton.setOutlineColor(
        sf::Color(205, 177, 118)
    );

    sf::Text mainHelpButtonText(font);
    mainHelpButtonText.setString(sf::String(L"조작법"));
    mainHelpButtonText.setCharacterSize(20);
    mainHelpButtonText.setFillColor(
        sf::Color(245, 232, 200)
    );

    int selectedPlayers = 0;

    sf::String roomName;

    bool roomNameInputActive =
        false;

    sf::RectangleShape roomNameInputArea({
        340.f,
        55.f
        });

    roomNameInputArea.setPosition({
        560.f,
        278.f
        });

    roomNameInputArea.setFillColor(
        sf::Color::Transparent
    );

    sf::Text roomNameText(font);

    roomNameText.setCharacterSize(22);

    roomNameText.setFillColor(
        sf::Color::White
    );

    std::string roomCode =
        generateRoomCode();

    std::string currentRoomCode;

    sf::Text roomCodeText(font);

    roomCodeText.setString(
        roomCode
    );

    roomCodeText.setCharacterSize(27);

    roomCodeText.setFillColor(
        sf::Color(
            245,
            210,
            120
        )
    );

    sf::RectangleShape roomCodeCopyArea({
        350.f,
        56.f
        });

    roomCodeCopyArea.setPosition({
        371.f,
        505.f
        });

    roomCodeCopyArea.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape player2Button({
        135.f,
        70.f
        });

    player2Button.setPosition({
        363.f,
        388.f
        });

    player2Button.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape player3Button({
        135.f,
        70.f
        });

    player3Button.setPosition({
        505.f,
        388.f
        });

    player3Button.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape player4Button({
        135.f,
        70.f
        });

    player4Button.setPosition({
        644.f,
        388.f
        });

    player4Button.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape player5Button({
        140.f,
        70.f
        });

    player5Button.setPosition({
        782.f,
        388.f
        });

    player5Button.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape regenerateButton({
        175.f,
        67.f
        });

    regenerateButton.setPosition({
        735.f,
        504.f
        });

    regenerateButton.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape finalCreateRoomButton({
        330.f,
        75.f
        });

    finalCreateRoomButton.setPosition({
        474.f,
        575.f
        });

    finalCreateRoomButton.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape createBackButton({
        240.f,
        60.f
        });

    createBackButton.setPosition({
        36.f,
        623.f
        });

    createBackButton.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape selectedHighlight;

    selectedHighlight.setFillColor(
        sf::Color(
            255,
            190,
            40,
            75
        )
    );

    sf::String joinCode;

    bool joinInputActive =
        false;

    sf::Text joinCodeText(font);

    joinCodeText.setCharacterSize(24);

    joinCodeText.setFillColor(
        sf::Color::White
    );

    sf::RectangleShape joinInputArea({
        380.f,
        70.f
        });

    joinInputArea.setPosition({
        450.f,
        388.f
        });

    joinInputArea.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape joinConfirmButton({
        300.f,
        73.f
        });

    joinConfirmButton.setPosition({
        491.f,
        477.f
        });

    joinConfirmButton.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape joinBackButton({
        250.f,
        58.f
        });

    joinBackButton.setPosition({
        35.f,
        594.f
        });

    joinBackButton.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape lobbyStartButton({
        300.f,
        63.f
        });

    lobbyStartButton.setPosition({
        490.f,
        565.f
        });

    lobbyStartButton.setFillColor(
        sf::Color::Transparent
    );

    sf::RectangleShape lobbyBackButton({
        240.f,
        58.f
        });

    lobbyBackButton.setPosition({
        38.f,
        623.f
        });

    lobbyBackButton.setFillColor(
        sf::Color::Transparent
    );

    sf::Text lobbyRoomCodeText(font);

    lobbyRoomCodeText.setCharacterSize(15);

    lobbyRoomCodeText.setFillColor(
        sf::Color(
            55,
            30,
            15
        )
    );

    sf::RectangleShape lobbyRoomCodeCopyArea({
        190.f,
        30.f
        });

    lobbyRoomCodeCopyArea.setPosition({
        665.f,
        207.f
        });

    lobbyRoomCodeCopyArea.setFillColor(
        sf::Color::Transparent
    );

    std::vector<sf::RectangleShape>
        lobbyKickButtons;

    const sf::Vector2f
        kickPositions[4] =
    {
        { 758.f, 318.f },
        { 758.f, 380.f },
        { 758.f, 442.f },
        { 758.f, 504.f }
    };

    for (int i = 0; i < 4; ++i)
    {
        sf::RectangleShape kickButton({
            100.f,
            48.f
            });

        kickButton.setPosition(
            kickPositions[i]
        );

        kickButton.setFillColor(
            sf::Color::Transparent
        );

        lobbyKickButtons.push_back(
            kickButton
        );
    }

    std::vector<sf::Text>
        lobbyPlayerTexts;

    lobbyPlayerTexts.reserve(5);

    for (int i = 0; i < 5; ++i)
    {
        sf::Text playerText(font);

        playerText.setCharacterSize(22);

        playerText.setFillColor(
            sf::Color(
                55,
                30,
                15
            )
        );

        lobbyPlayerTexts.push_back(
            playerText
        );
    }

    std::size_t lobbyVisiblePlayerCount =
        0;

    sf::Text lobbyRoomNameText(font);

    lobbyRoomNameText.setCharacterSize(20);

    lobbyRoomNameText.setFillColor(
        sf::Color(
            55,
            30,
            15
        )
    );

    sf::Text tempText(font);

    tempText.setCharacterSize(32);

    tempText.setFillColor(
        sf::Color::White
    );

    tempText.setPosition({
        470.f,
        330.f
        });

    sf::Text multiGameText(font);

    multiGameText.setString(
        U"멀티플레이 게임 시작"
    );

    multiGameText.setCharacterSize(42);

    multiGameText.setFillColor(
        sf::Color::White
    );

    centerText(
        multiGameText,
        0.f,
        0.f,
        1280.f,
        720.f
    );

    while (window.isOpen())
    {
        // window.create() 이후 HWND가 바뀌어도 자동으로 다시 연결합니다.
        nativeChatInput.ensureAttached(window);

        // 채팅 활성 중에는 같은 게임 HWND가 한글 IME 조합을 직접 처리합니다.
        nativeChatInput.pumpMessages();

        if (multiChatActive)
        {
            multiChatInput = nativeChatInput.getText();

            if (nativeChatInput.takeSubmitRequest())
            {
                if (!multiChatInput.isEmpty())
                {
                    networkManager.sendChatMessage(
                        nickname,
                        multiChatInput
                    );

                    nativeChatInput.clear();
                    multiChatInput.clear();
                }
            }

            if (nativeChatInput.takeDeactivateRequest())
            {
                multiChatActive = false;
                nativeChatInput.setActive(window, false);
            }
        }

        while (const std::optional event =
            window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                if (
                    networkManager
                    .isServerRunning()
                    )
                {
                    networkManager
                        .stopServer();
                }

                window.close();
            }

            if (
                currentState ==
                GameState::NicknameInput
                )
            {
                nicknameInputActive =
                    true;

                if (const auto* mousePressed =
                    event->getIf<
                    sf::Event::MouseButtonPressed>())
                {
                    if (
                        mousePressed->button ==
                        sf::Mouse::Button::Left
                        )
                    {
                        sf::Vector2f mousePos(
                            static_cast<float>(
                                mousePressed
                                ->position.x
                                ),
                            static_cast<float>(
                                mousePressed
                                ->position.y
                                )
                        );

                        if (
                            nicknameInputArea
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            nicknameInputActive =
                                true;
                        }

                        if (
                            confirmButton
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            if (
                                !nickname.isEmpty()
                                )
                            {
                                sf::String welcome =
                                    nickname;

                                welcome +=
                                    U"님, 환영합니다!";

                                welcomeText
                                    .setString(
                                        welcome
                                    );

                                centerText(
                                    welcomeText,
                                    438.f,
                                    211.f,
                                    338.f,
                                    77.f
                                );

                                nicknameInputActive =
                                    false;

                                currentState =
                                    GameState::MainMenu;
                            }
                        }
                    }
                }

                if (const auto* keyPressed =
                    event->getIf<
                    sf::Event::KeyPressed>())
                {
                    if (
                        keyPressed->code ==
                        sf::Keyboard::Key::Enter
                        )
                    {
                        if (
                            !nickname.isEmpty()
                            )
                        {
                            sf::String welcome =
                                nickname;

                            welcome +=
                                U"님, 환영합니다!";

                            welcomeText.setString(
                                welcome
                            );

                            centerText(
                                welcomeText,
                                438.f,
                                211.f,
                                338.f,
                                77.f
                            );

                            nicknameInputActive =
                                false;

                            currentState =
                                GameState::MainMenu;
                        }
                    }
                }

                if (const auto* textEntered =
                    event->getIf<
                    sf::Event::TextEntered>())
                {
                    char32_t unicode =
                        textEntered->unicode;

                    if (unicode == 8)
                    {
                        if (
                            !nickname.isEmpty()
                            )
                        {
                            nickname.erase(
                                nickname.getSize() -
                                1,
                                1
                            );
                        }
                    }
                    else if (unicode >= 32)
                    {
                        if (
                            nickname.getSize() <
                            12
                            )
                        {
                            nickname += unicode;
                        }
                    }

                    nicknameText.setString(
                        nickname
                    );
                }
            }

            else if (
                currentState ==
                GameState::MainMenu
                )
            {
                if (const auto* mousePressed =
                    event->getIf<
                    sf::Event::MouseButtonPressed>())
                {
                    if (
                        mousePressed->button ==
                        sf::Mouse::Button::Left
                        )
                    {
                        sf::Vector2f mousePos(
                            static_cast<float>(
                                mousePressed
                                ->position.x
                                ),
                            static_cast<float>(
                                mousePressed
                                ->position.y
                                )
                        );

                        if (
                            mainHelpButton
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            mainHelpVisible =
                                !mainHelpVisible;
                            continue;
                        }

                        if (mainHelpVisible)
                        {
                            // 도움말이 열린 동안 뒤쪽 메뉴 버튼은 눌리지 않게 합니다.
                            continue;
                        }

                        if (
                            createRoomButton
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            selectedPlayers = 0;

                            roomName.clear();

                            roomNameText.setString(
                                roomName
                            );

                            roomNameInputActive =
                                true;

                            roomCode =
                                generateRoomCode();

                            roomCodeText.setString(
                                roomCode
                            );

                            currentRoomCode.clear();

                            currentState =
                                GameState::CreateRoom;
                        }

                        else if (
                            joinRoomButton
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            joinCode.clear();

                            joinCodeText.setString(
                                joinCode
                            );

                            joinInputActive = true;

                            currentRoomCode.clear();

                            currentState =
                                GameState::JoinRoom;
                        }

                        else if (
                            singleButton
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            singleGame =
                                dw::Game{};

                            singleGameMode.reset();

                            singleGameMode.start(
                                singleGame
                            );

                            singleScreen =
                                dw::Screen{};

                            singleScreen.menu =
                                false;

                            singleScreen.help =
                                false;

                            singleScreen.paused =
                                false;

                            singleScreen.online =
                                false;

                            singleScreen.client =
                                false;

                            singleScreen.local =
                                0;

                            singleInput = {};

                            singleGameClock.restart();

                            createBorderlessGameWindow(window);

                            currentState =
                                GameState::
                                SinglePlayer;
                        }

                        else if (
                            exitButton
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            window.close();
                        }

                        else if (
                            mainBackButton
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            currentState =
                                GameState::
                                NicknameInput;

                            nicknameInputActive =
                                true;
                        }
                    }
                }

                if (
                    const auto* keyPressed =
                    event->getIf<sf::Event::KeyPressed>()
                    )
                {
                    if (
                        keyPressed->code == sf::Keyboard::Key::F1 ||
                        keyPressed->scancode == sf::Keyboard::Scancode::F1
                        )
                    {
                        mainHelpVisible = !mainHelpVisible;
                    }
                    else if (
                        mainHelpVisible &&
                        (keyPressed->code == sf::Keyboard::Key::Escape ||
                            keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                        )
                    {
                        mainHelpVisible = false;
                    }
                }
            }

            else if (
                currentState ==
                GameState::CreateRoom
                )
            {
                if (const auto* mousePressed =
                    event->getIf<
                    sf::Event::MouseButtonPressed>())
                {
                    if (
                        mousePressed->button ==
                        sf::Mouse::Button::Left
                        )
                    {
                        sf::Vector2f mousePos(
                            static_cast<float>(
                                mousePressed
                                ->position.x
                                ),
                            static_cast<float>(
                                mousePressed
                                ->position.y
                                )
                        );

                        if (
                            roomNameInputArea
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            roomNameInputActive =
                                true;
                        }
                        else
                        {
                            roomNameInputActive =
                                false;
                        }

                        if (
                            player2Button
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            selectedPlayers = 2;
                        }

                        else if (
                            player3Button
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            selectedPlayers = 3;
                        }

                        else if (
                            player4Button
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            selectedPlayers = 4;
                        }

                        else if (
                            player5Button
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            selectedPlayers = 5;
                        }

                        else if (
                            roomCodeCopyArea
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            if (
                                copyToClipboard(
                                    roomCode
                                )
                                )
                            {
                                MessageBoxW(
                                    nullptr,
                                    L"방 코드가 복사되었습니다.",
                                    L"방 코드 복사",
                                    MB_OK |
                                    MB_ICONINFORMATION
                                );
                            }
                            else
                            {
                                MessageBoxW(
                                    nullptr,
                                    L"방 코드 복사에 실패했습니다.",
                                    L"복사 실패",
                                    MB_OK |
                                    MB_ICONERROR
                                );
                            }
                        }

                        else if (
                            regenerateButton
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            roomCode =
                                generateRoomCode();

                            roomCodeText.setString(
                                roomCode
                            );
                        }

                        else if (
                            createBackButton
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            currentState =
                                GameState::
                                MainMenu;
                        }

                        else if (
                            finalCreateRoomButton
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            if (
                                roomName.isEmpty()
                                )
                            {
                                MessageBoxW(
                                    nullptr,
                                    L"방 이름을 입력해주세요.",
                                    L"방 만들기",
                                    MB_OK |
                                    MB_ICONWARNING
                                );
                            }
                            else if (
                                selectedPlayers ==
                                0
                                )
                            {
                                MessageBoxW(
                                    nullptr,
                                    L"방 인원을 먼저 선택해주세요.",
                                    L"방 만들기",
                                    MB_OK |
                                    MB_ICONWARNING
                                );
                            }
                            else
                            {
                                networkManager
                                    .setRoomName(
                                        roomName
                                    );

                                networkManager
                                    .setRoomCode(
                                        roomCode
                                    );

                                networkManager
                                    .setMaxPlayers(
                                        selectedPlayers
                                    );

                                bool serverStarted =
                                    networkManager
                                    .startServer(
                                        54000
                                    );

                                if (serverStarted)
                                {
                                    currentRoomCode =
                                        roomCode;

                                    currentState =
                                        GameState::
                                        Lobby;
                                }
                                else
                                {
                                    MessageBoxW(
                                        nullptr,
                                        L"방을 만들 수 없습니다.\n서버 상태를 확인해주세요.",
                                        L"서버 오류",
                                        MB_OK |
                                        MB_ICONERROR
                                    );
                                }
                            }
                        }
                    }
                }

                if (roomNameInputActive)
                {
                    if (
                        const auto*
                        textEntered =
                        event->getIf<
                        sf::Event::
                        TextEntered>()
                        )
                    {
                        char32_t unicode =
                            textEntered
                            ->unicode;

                        if (unicode == 8)
                        {
                            if (
                                !roomName
                                .isEmpty()
                                )
                            {
                                roomName.erase(
                                    roomName
                                    .getSize() -
                                    1,
                                    1
                                );
                            }
                        }

                        else if (
                            unicode >= 32
                            )
                        {
                            if (
                                roomName
                                .getSize() <
                                12
                                )
                            {
                                roomName +=
                                    unicode;
                            }
                        }

                        roomNameText.setString(
                            roomName
                        );
                    }
                }
            }

            else if (
                currentState ==
                GameState::JoinRoom
                )
            {
                if (
                    const auto*
                    keyPressed =
                    event->getIf<
                    sf::Event::
                    KeyPressed>()
                    )
                {
                    using K =
                        sf::Keyboard::Key;

                    if (
                        keyPressed->control &&
                        keyPressed->code ==
                        K::V
                        )
                    {
                        std::string pastedCode =
                            getRoomCodeFromClipboard();

                        if (
                            !pastedCode.empty()
                            )
                        {
                            joinCode =
                                sf::String::
                                fromUtf8(
                                    pastedCode
                                    .begin(),
                                    pastedCode
                                    .end()
                                );

                            joinCodeText
                                .setString(
                                    joinCode
                                );

                            joinInputActive =
                                true;
                        }

                        continue;
                    }
                }

                if (
                    const auto*
                    mousePressed =
                    event->getIf<
                    sf::Event::
                    MouseButtonPressed>()
                    )
                {
                    if (
                        mousePressed->button ==
                        sf::Mouse::Button::Left
                        )
                    {
                        sf::Vector2f mousePos(
                            static_cast<float>(
                                mousePressed
                                ->position.x
                                ),
                            static_cast<float>(
                                mousePressed
                                ->position.y
                                )
                        );

                        if (
                            joinInputArea
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            joinInputActive =
                                true;
                        }

                        if (
                            joinBackButton
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            currentState =
                                GameState::
                                MainMenu;
                        }

                        else if (
                            joinConfirmButton
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            if (
                                joinCode
                                .getSize() !=
                                6
                                )
                            {
                                MessageBoxW(
                                    nullptr,
                                    L"방 코드는 6자리입니다.",
                                    L"방 참가",
                                    MB_OK |
                                    MB_ICONWARNING
                                );
                            }
                            else
                            {
                                sf::U8String
                                    utf8Code =
                                    joinCode
                                    .toUtf8();

                                std::string
                                    enteredRoomCode(
                                        utf8Code
                                        .begin(),
                                        utf8Code
                                        .end()
                                    );

                                auto roomServer =
                                    networkManager
                                    .findRoomServer(
                                        enteredRoomCode
                                    );

                                if (
                                    !roomServer
                                    .has_value()
                                    )
                                {
                                    MessageBoxW(
                                        nullptr,
                                        L"해당 방 코드를 가진 방을 찾을 수 없습니다.\n같은 네트워크인지 확인해주세요.",
                                        L"방 찾기 실패",
                                        MB_OK |
                                        MB_ICONWARNING
                                    );
                                }
                                else
                                {
                                    bool connected =
                                        networkManager
                                        .connectToServer(
                                            roomServer
                                            ->address,
                                            roomServer
                                            ->port
                                        );

                                    if (connected)
                                    {
                                        bool sent =
                                            networkManager
                                            .sendJoinRequest(
                                                nickname,
                                                enteredRoomCode
                                            );

                                        if (!sent)
                                        {
                                            MessageBoxW(
                                                nullptr,
                                                L"참가 요청을 전송하지 못했습니다.",
                                                L"접속 오류",
                                                MB_OK |
                                                MB_ICONERROR
                                            );

                                            networkManager
                                                .disconnect();
                                        }
                                        else
                                        {
                                            currentRoomCode =
                                                enteredRoomCode;
                                        }
                                    }
                                    else
                                    {
                                        MessageBoxW(
                                            nullptr,
                                            L"찾은 방의 서버에 연결할 수 없습니다.",
                                            L"접속 실패",
                                            MB_OK |
                                            MB_ICONERROR
                                        );
                                    }
                                }
                            }
                        }
                    }
                }

                if (
                    const auto*
                    textEntered =
                    event->getIf<
                    sf::Event::
                    TextEntered>()
                    )
                {
                    char32_t unicode =
                        textEntered
                        ->unicode;

                    if (unicode == 8)
                    {
                        if (
                            !joinCode
                            .isEmpty()
                            )
                        {
                            joinCode.erase(
                                joinCode
                                .getSize() -
                                1,
                                1
                            );
                        }
                    }

                    else if (
                        joinCode.getSize() <
                        6
                        )
                    {
                        if (
                            (
                                unicode >= U'A' &&
                                unicode <= U'Z'
                                )
                            ||
                            (
                                unicode >= U'a' &&
                                unicode <= U'z'
                                )
                            ||
                            (
                                unicode >= U'0' &&
                                unicode <= U'9'
                                )
                            )
                        {
                            if (
                                unicode >= U'a' &&
                                unicode <= U'z'
                                )
                            {
                                unicode =
                                    unicode -
                                    U'a' +
                                    U'A';
                            }

                            joinCode += unicode;
                        }
                    }

                    joinCodeText.setString(
                        joinCode
                    );
                }
            }

            else if (
                currentState ==
                GameState::Lobby
                )
            {
                if (
                    const auto*
                    mousePressed =
                    event->getIf<
                    sf::Event::
                    MouseButtonPressed>()
                    )
                {
                    if (
                        mousePressed->button ==
                        sf::Mouse::Button::Left
                        )
                    {
                        sf::Vector2f mousePos(
                            static_cast<float>(
                                mousePressed
                                ->position.x
                                ),
                            static_cast<float>(
                                mousePressed
                                ->position.y
                                )
                        );

                        if (
                            lobbyBackButton
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            if (
                                networkManager
                                .isServerRunning()
                                )
                            {
                                networkManager
                                    .stopServer();

                                window.close();

                                continue;
                            }

                            if (
                                networkManager
                                .isConnected()
                                )
                            {
                                networkManager
                                    .disconnect();
                            }

                            currentRoomCode.clear();

                            currentState =
                                GameState::
                                MainMenu;
                        }

                        else if (
                            lobbyRoomCodeCopyArea
                            .getGlobalBounds()
                            .contains(mousePos)
                            )
                        {
                            if (
                                !currentRoomCode
                                .empty()
                                )
                            {
                                if (
                                    copyToClipboard(
                                        currentRoomCode
                                    )
                                    )
                                {
                                    MessageBoxW(
                                        nullptr,
                                        L"방 코드가 복사되었습니다.",
                                        L"방 코드 복사",
                                        MB_OK |
                                        MB_ICONINFORMATION
                                    );
                                }
                            }
                        }
                        else
                        {
                            bool kickClicked =
                                false;

                            for (
                                std::size_t i =
                                0;

                                i <
                                lobbyKickButtons
                                .size();

                                ++i
                                )
                            {
                                if (
                                    lobbyKickButtons[i]
                                    .getGlobalBounds()
                                    .contains(mousePos)
                                    )
                                {
                                    kickClicked =
                                        true;

                                    if (
                                        !networkManager
                                        .isServerRunning()
                                        )
                                    {
                                        MessageBoxW(
                                            nullptr,
                                            L"방장 권한입니다.",
                                            L"내보내기",
                                            MB_OK |
                                            MB_ICONINFORMATION
                                        );
                                    }

                                    else if (
                                        i + 1 <
                                        lobbyVisiblePlayerCount
                                        )
                                    {
                                        networkManager
                                            .kickPlayer(i);
                                    }

                                    break;
                                }
                            }

                            if (
                                !kickClicked &&
                                lobbyStartButton
                                .getGlobalBounds()
                                .contains(mousePos)
                                )
                            {
                                if (
                                    !networkManager
                                    .isServerRunning()
                                    )
                                {
                                    MessageBoxW(
                                        nullptr,
                                        L"방장만 게임을 시작할 수 있습니다.",
                                        L"게임 시작",
                                        MB_OK |
                                        MB_ICONINFORMATION
                                    );
                                }
                                else
                                {
                                    int currentPlayers =
                                        1;

                                    for (
                                        const auto& name :
                                        networkManager
                                        .getPlayerNicknames()
                                        )
                                    {
                                        if (
                                            !name.isEmpty()
                                            )
                                        {
                                            ++currentPlayers;
                                        }
                                    }

                                    int requiredPlayers =
                                        networkManager
                                        .getMaxPlayers();

                                    if (
                                        currentPlayers <
                                        requiredPlayers
                                        )
                                    {
                                        std::wstring message =
                                            L"인원이 부족합니다.\n현재 인원 : "
                                            +
                                            std::to_wstring(
                                                currentPlayers
                                            )
                                            +
                                            L" / "
                                            +
                                            std::to_wstring(
                                                requiredPlayers
                                            );

                                        MessageBoxW(
                                            nullptr,
                                            message.c_str(),
                                            L"게임 시작",
                                            MB_OK |
                                            MB_ICONWARNING
                                        );
                                    }
                                    else
                                    {
                                        bool started =
                                            networkManager
                                            .startGame();

                                        if (started)
                                        {
                                            multiGame =
                                                dw::Game{};

                                            multiInput = {};

                                            multiScreen =
                                                dw::Screen{};

                                            multiScreen.menu =
                                                false;

                                            multiScreen.help =
                                                false;

                                            multiScreen.paused =
                                                false;

                                            multiGameMode
                                                .start(
                                                    multiGame
                                                );

                                            multiGameMode
                                                .updateScreen(
                                                    multiScreen
                                                );

                                            multiGameClock
                                                .restart();

                                            createBorderlessGameWindow(window);

                                            currentState =
                                                GameState::
                                                MultiGame;
                                        }
                                        else
                                        {
                                            MessageBoxW(
                                                nullptr,
                                                L"게임을 시작할 수 없습니다.",
                                                L"게임 시작",
                                                MB_OK |
                                                MB_ICONERROR
                                            );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            else if (
                currentState ==
                GameState::SinglePlayer
                )
            {
                if (
                    event->is<
                    sf::Event::FocusLost>() &&
                    singleGame.phase ==
                    dw::Phase::Play
                    )
                {
                    singleScreen.paused =
                        true;
                }

                if (
                    const auto* mousePressed =
                    event->getIf<
                    sf::Event::MouseButtonPressed>()
                    )
                {
                    if (
                        mousePressed->button ==
                        sf::Mouse::Button::Left
                        )
                    {
                        const sf::Vector2f gamePoint =
                            windowToGamePoint(
                                window,
                                mousePressed->position,
                                true
                            );

                        if (singleGame.phase == dw::Phase::Shop)
                        {
                            singleInput.shopVisit = singleGame.shopVisit;
                            singleInput.purchaseVoteId = singleGame.purchaseVoteId;

                            if (singleGame.purchaseVoteActive)
                            {
                                const int vote = shopVoteChoiceAt(gamePoint);
                                if (vote >= 0)
                                    singleInput.purchaseVote = vote;
                                continue;
                            }

                            if (shopReadyAt(gamePoint))
                            {
                                singleInput.ready = true;
                                continue;
                            }

                            const int item = shopItemAt(gamePoint);
                            if (item >= 0)
                                singleInput.buy = item;
                        }
                    }
                }

                if (
                    const auto*
                    keyPressed =
                    event->getIf<
                    sf::Event::
                    KeyPressed>()
                    )
                {
                    const auto key =
                        keyPressed->code;

                    const auto scan =
                        keyPressed->scancode;

                    using K =
                        sf::Keyboard::Key;

                    using S =
                        sf::Keyboard::Scancode;

                    if (key == K::F10)
                    {
                        singleGameMode
                            .reset();

                        singleInput = {};

                        singleScreen =
                            dw::Screen{};

                        window.create(
                            sf::VideoMode({
                                1280,
                                720
                                }),

                            "Pirate Defense",

                            sf::State::Windowed
                        );

                        window.setFramerateLimit(
                            60
                        );
                        window.setKeyRepeatEnabled(false);

                        currentState =
                            GameState::
                            MainMenu;

                        continue;
                    }

                    if (key == K::F1)
                    {
                        singleScreen.help =
                            !singleScreen.help;

                        continue;
                    }

                    // ESC: 싱글 게임 종료 후 메인 메뉴로 복귀
                    if (key == K::Escape || scan == S::Escape)
                    {
                        singleGameMode
                            .reset();

                        singleGame =
                            dw::Game{};

                        singleInput = {};

                        singleScreen =
                            dw::Screen{};

                        window.create(
                            sf::VideoMode({
                                1280,
                                720
                                }),

                            "Pirate Defense",

                            sf::State::Windowed
                        );

                        window.setFramerateLimit(
                            60
                        );
                        window.setKeyRepeatEnabled(false);

                        currentState =
                            GameState::MainMenu;

                        continue;
                    }

                    if (
                        key == K::Enter &&
                        (
                            singleGame.phase ==
                            dw::Phase::Lobby ||

                            singleGame.phase ==
                            dw::Phase::Won ||

                            singleGame.phase ==
                            dw::Phase::Lost
                            )
                        )
                    {
                        singleGameMode.start(
                            singleGame
                        );

                        singleInput = {};

                        singleScreen.paused =
                            false;

                        singleScreen.help =
                            false;

                        continue;
                    }

                    if (
                        singleScreen.help ||
                        singleScreen.paused
                        )
                    {
                        continue;
                    }

                    if (
                        singleGame.phase ==
                        dw::Phase::Shop
                        )
                    {
                        singleInput.shopVisit =
                            singleGame.shopVisit;

                        singleInput.purchaseVoteId =
                            singleGame
                            .purchaseVoteId;

                        // 상품/찬반 투표는 마우스로 진행하고, Enter로 출항 준비합니다.
                        if (
                            key == K::Enter ||
                            scan == S::Enter
                            )
                        {
                            singleInput.ready = true;
                        }

                        continue;
                    }

                    if (
                        key >= K::Num1 &&
                        key <= K::Num6
                        )
                    {
                        const int number =
                            static_cast<int>(key) -
                            static_cast<int>(K::Num1);

                        if (singleGame.phase == dw::Phase::Play)
                        {
                            singleInput.select = number;
                            continue;
                        }

                        if (singleGame.phase == dw::Phase::Reward && number < 3)
                        {
                            singleInput.vote = number;
                            continue;
                        }
                    }

                    if (key == K::E || scan == S::E)
                    {
                        if (
                            singleGame.phase == dw::Phase::Play &&
                            playerNearAmmoRack(singleGame, 0)
                            )
                        {
                            singleInput.tap = true;
                            continue;
                        }

                        singleInput.tap = true;
                    }

                    if (key == K::Q || scan == S::Q)
                    {
                        singleInput.drop =
                            true;
                    }

                    if (key == K::Space)
                    {
                        singleInput.fire =
                            true;
                    }

                    if (key == K::C || scan == S::C)
                    {
                        singleInput.melee =
                            true;
                    }

                    // 유동호 병합: 물고기 1마리를 사용해 개인 체력 회복
                    if (key == K::H || scan == S::H)
                    {
                        singleInput.useFish =
                            true;
                    }
                }
            }

            else if (
                currentState ==
                GameState::MultiGame
                )
            {
                if (
                    const auto* mousePressed =
                    event->getIf<
                    sf::Event::MouseButtonPressed>()
                    )
                {
                    if (
                        mousePressed->button == sf::Mouse::Button::Left &&
                        !multiChatActive
                        )
                    {
                        const sf::Vector2f gamePoint =
                            windowToGamePoint(
                                window,
                                mousePressed->position,
                                false
                            );

                        if (multiGame.phase == dw::Phase::Shop)
                        {
                            multiInput.shopVisit = multiGame.shopVisit;
                            multiInput.purchaseVoteId = multiGame.purchaseVoteId;

                            if (multiGame.purchaseVoteActive)
                            {
                                const int vote = shopVoteChoiceAt(gamePoint);
                                if (vote >= 0)
                                    multiInput.purchaseVote = vote;
                                continue;
                            }

                            if (shopReadyAt(gamePoint))
                            {
                                multiInput.ready = true;
                                continue;
                            }

                            const int item = shopItemAt(gamePoint);
                            if (item >= 0)
                                multiInput.buy = item;
                        }
                    }
                }

                if (
                    const auto*
                    keyPressed =
                    event->getIf<
                    sf::Event::
                    KeyPressed>()
                    )
                {
                    const auto key =
                        keyPressed->code;

                    const auto scan =
                        keyPressed->scancode;

                    using K =
                        sf::Keyboard::Key;

                    using S =
                        sf::Keyboard::Scancode;

                    if (key == K::Tab || scan == S::Tab)
                    {
                        // 채팅창은 항상 보입니다.
                        // 비활성 상태에서 Tab을 누르면 현재 게임 HWND의 IME 입력을 엽니다.
                        if (!multiChatActive)
                        {
                            multiChatActive = true;
                            nativeChatInput.setText(multiChatInput);
                            nativeChatInput.setActive(window, true);
                        }
                        else
                        {
                            multiChatActive = false;
                            nativeChatInput.setActive(window, false);
                        }

                        multiInput.x = 0.f;
                        multiInput.y = 0.f;
                        multiInput.hold = false;
                        multiInput.brace = false;
                        multiInput.edgesOff();
                        continue;
                    }

                    if (multiChatActive)
                    {
                        // 채팅 활성 중의 문자/Enter/Tab은 Win32 IME 처리기가 받습니다.
                        // SFML 게임 단축키는 실행하지 않습니다.
                        continue;
                    }

                    if (key == K::F10 || scan == S::F10)
                    {
                        multiGameMode.stop();

                        if (
                            networkManager
                            .isServerRunning()
                            )
                        {
                            networkManager
                                .stopServer();
                        }
                        else if (
                            networkManager
                            .isConnected()
                            )
                        {
                            networkManager
                                .disconnect();
                        }

                        multiGame =
                            dw::Game{};

                        multiInput = {};

                        multiScreen =
                            dw::Screen{};

                        currentRoomCode.clear();
                        multiChatActive = false;
                        multiChatInput.clear();
                        nativeChatInput.clear();
                        nativeChatInput.setActive(window, false);

                        window.create(
                            sf::VideoMode({
                                1280,
                                720
                                }),

                            "Pirate Defense",

                            sf::State::Windowed
                        );

                        window.setFramerateLimit(
                            60
                        );
                        window.setKeyRepeatEnabled(false);

                        currentState =
                            GameState::
                            MainMenu;

                        continue;
                    }

                    // ESC: 멀티 게임 종료 후 메인 메뉴로 복귀
                    // 방장은 stopServer()에서 참가자에게 HOST_CLOSED를 전송
                    if (key == K::Escape || scan == S::Escape)
                    {
                        multiGameMode.stop();

                        if (
                            networkManager
                            .isServerRunning()
                            )
                        {
                            networkManager
                                .stopServer();
                        }
                        else if (
                            networkManager
                            .isConnected()
                            )
                        {
                            networkManager
                                .disconnect();
                        }

                        multiGame =
                            dw::Game{};

                        multiInput = {};

                        multiScreen =
                            dw::Screen{};

                        currentRoomCode.clear();
                        multiChatActive = false;
                        multiChatInput.clear();
                        nativeChatInput.clear();
                        nativeChatInput.setActive(window, false);

                        window.create(
                            sf::VideoMode({
                                1280,
                                720
                                }),

                            "Pirate Defense",

                            sf::State::Windowed
                        );

                        window.setFramerateLimit(
                            60
                        );
                        window.setKeyRepeatEnabled(false);

                        currentState =
                            GameState::MainMenu;

                        continue;
                    }

                    if (key == K::F1 || scan == S::F1)
                    {
                        multiScreen.help =
                            !multiScreen.help;

                        continue;
                    }

                    if (multiScreen.help)
                    {
                        continue;
                    }

                    if (
                        (key == K::Enter || scan == S::Enter) &&
                        (
                            multiGame.phase ==
                            dw::Phase::Won ||

                            multiGame.phase ==
                            dw::Phase::Lost
                            )
                        )
                    {
                        multiGameMode.restart(
                            multiGame
                        );

                        multiInput = {};

                        continue;
                    }

                    if (
                        multiGame.phase ==
                        dw::Phase::Shop
                        )
                    {
                        multiInput.shopVisit =
                            multiGame.shopVisit;

                        multiInput.purchaseVoteId =
                            multiGame
                            .purchaseVoteId;

                        // 상품/찬반 투표는 마우스로 진행하고, Enter로 출항 준비합니다.
                        if (
                            key == K::Enter ||
                            scan == S::Enter
                            )
                        {
                            multiInput.ready = true;
                        }

                        continue;
                    }

                    if (
                        key >= K::Num1 &&
                        key <= K::Num6
                        )
                    {
                        const int number =
                            static_cast<int>(key) -
                            static_cast<int>(K::Num1);

                        if (multiGame.phase == dw::Phase::Play)
                        {
                            multiInput.select = number;
                            continue;
                        }

                        if (multiGame.phase == dw::Phase::Reward && number < 3)
                        {
                            multiInput.vote = number;
                            continue;
                        }
                    }

                    if (key == K::E || scan == S::E)
                    {
                        const int localId =
                            std::clamp(
                                multiScreen.local,
                                0,
                                dw::MaxPlayers - 1
                            );

                        if (
                            multiGame.phase == dw::Phase::Play &&
                            playerNearAmmoRack(multiGame, localId)
                            )
                        {
                            multiInput.tap = true;
                            continue;
                        }

                        multiInput.tap = true;
                    }

                    if (key == K::Q || scan == S::Q)
                    {
                        multiInput.drop =
                            true;
                    }

                    if (key == K::Space)
                    {
                        multiInput.fire =
                            true;
                    }

                    if (key == K::C || scan == S::C)
                    {
                        multiInput.melee =
                            true;
                    }

                    // 유동호 병합: 물고기 1마리를 사용해 개인 체력 회복
                    if (key == K::H || scan == S::H)
                    {
                        multiInput.useFish =
                            true;
                    }
                }
            }
        }

        if (
            networkManager
            .isServerRunning()
            )
        {
            networkManager
                .updateServer();

            networkManager
                .broadcastPlayerList(
                    nickname
                );
        }

        if (
            networkManager
            .isConnected()
            )
        {
            networkManager
                .updateClient();

            if (
                networkManager
                .isHostClosed()
                )
            {
                // 방장이 나가도 클라이언트 창은 닫지 않고 메인 메뉴로 복귀
                multiGameMode.stop();
                networkManager.disconnect();

                multiGame = dw::Game{};
                multiInput = {};
                multiScreen = dw::Screen{};
                currentRoomCode.clear();

                window.create(
                    sf::VideoMode({ 1280, 720 }),
                    "Pirate Defense",
                    sf::State::Windowed
                );
                window.setFramerateLimit(60);
                window.setKeyRepeatEnabled(false);
                currentState = GameState::MainMenu;

                MessageBoxW(
                    nullptr,
                    L"방장이 나갔습니다. 메인화면으로 돌아갑니다.",
                    L"방장 연결 종료",
                    MB_OK |
                    MB_ICONINFORMATION
                );

                continue;
            }

            if (
                networkManager
                .isKicked()
                )
            {
                MessageBoxW(
                    nullptr,
                    L"방장에 의해 방에서 내보내졌습니다.",
                    L"내보내기",
                    MB_OK |
                    MB_ICONWARNING
                );

                networkManager
                    .disconnect();

                currentRoomCode.clear();

                currentState =
                    GameState::MainMenu;
            }

            else if (
                networkManager
                .isJoinRejected()
                )
            {
                MessageBoxW(
                    nullptr,
                    L"방 코드가 올바르지 않습니다.",
                    L"입장 실패",
                    MB_OK |
                    MB_ICONWARNING
                );

                networkManager
                    .disconnect();

                currentRoomCode.clear();
            }

            else if (
                networkManager
                .isRoomFull()
                )
            {
                MessageBoxW(
                    nullptr,
                    L"방 인원이 가득 찼습니다.",
                    L"입장 실패",
                    MB_OK |
                    MB_ICONWARNING
                );

                networkManager
                    .disconnect();

                currentRoomCode.clear();
            }

            else if (
                networkManager
                .isGameStarted()
                )
            {
                multiGame =
                    dw::Game{};

                multiInput = {};

                multiScreen =
                    dw::Screen{};

                multiScreen.menu =
                    false;

                multiScreen.help =
                    false;

                multiScreen.paused =
                    false;

                multiGameMode.start(
                    multiGame
                );

                multiGameMode.updateScreen(
                    multiScreen
                );

                multiGameClock.restart();

                createBorderlessGameWindow(window);

                currentState =
                    GameState::
                    MultiGame;
            }

            else if (
                networkManager
                .isJoinAccepted()
                )
            {
                currentState =
                    GameState::Lobby;
            }
        }

        if (
            networkManager
            .isHostClosed()
            )
        {
            // 비정상 연결 종료/방장 강제 종료도 동일하게 메인 메뉴로 복귀
            multiGameMode.stop();
            networkManager.disconnect();

            multiGame = dw::Game{};
            multiInput = {};
            multiScreen = dw::Screen{};
            currentRoomCode.clear();

            window.create(
                sf::VideoMode({ 1280, 720 }),
                "Pirate Defense",
                sf::State::Windowed
            );
            window.setFramerateLimit(60);
            window.setKeyRepeatEnabled(false);
            currentState = GameState::MainMenu;

            MessageBoxW(
                nullptr,
                L"방장이 나갔습니다. 메인화면으로 돌아갑니다.",
                L"방장 연결 종료",
                MB_OK |
                MB_ICONINFORMATION
            );

            continue;
        }

        if (
            currentState ==
            GameState::Lobby
            )
        {
            std::vector<sf::String>
                displayNames;

            if (
                networkManager
                .isServerRunning()
                )
            {
                displayNames.push_back(
                    nickname
                );

                const auto&
                    receivedNames =
                    networkManager
                    .getPlayerNicknames();

                for (
                    const auto&
                    playerName :
                    receivedNames
                    )
                {
                    if (
                        !playerName
                        .isEmpty()
                        )
                    {
                        displayNames
                            .push_back(
                                playerName
                            );
                    }
                }
            }
            else
            {
                const auto&
                    syncedNames =
                    networkManager
                    .getSyncedPlayerNicknames();

                if (
                    !syncedNames.empty()
                    )
                {
                    for (
                        const auto&
                        name :
                        syncedNames
                        )
                    {
                        displayNames
                            .push_back(
                                name
                            );
                    }
                }
                else
                {
                    displayNames
                        .push_back(
                            nickname
                        );
                }
            }

            lobbyVisiblePlayerCount =
                displayNames.size();

            sf::String roomNameDisplay =
                U"방 이름 : ";

            if (
                networkManager
                .isServerRunning()
                )
            {
                roomNameDisplay +=
                    networkManager
                    .getRoomName();
            }
            else
            {
                roomNameDisplay +=
                    networkManager
                    .getSyncedRoomName();
            }

            lobbyRoomNameText.setString(
                roomNameDisplay
            );

            lobbyRoomNameText
                .setCharacterSize(
                    16
                );

            centerText(
                lobbyRoomNameText,
                425.f,
                207.f,
                190.f,
                30.f
            );

            sf::String roomCodeDisplay =
                U"방 코드 : ";

            roomCodeDisplay +=
                sf::String::fromUtf8(
                    currentRoomCode.begin(),
                    currentRoomCode.end()
                );

            lobbyRoomCodeText.setString(
                roomCodeDisplay
            );

            lobbyRoomCodeText
                .setCharacterSize(
                    16
                );

            centerText(
                lobbyRoomCodeText,
                665.f,
                207.f,
                190.f,
                30.f
            );

            const sf::Vector2f
                lobbyNamePositions[5] =
            {
                { 505.f, 266.f },
                { 505.f, 328.f },
                { 505.f, 390.f },
                { 505.f, 452.f },
                { 505.f, 514.f }
            };

            for (
                std::size_t i = 0;
                i <
                lobbyPlayerTexts.size();
                ++i
                )
            {
                if (
                    i <
                    displayNames.size()
                    )
                {
                    lobbyPlayerTexts[i]
                        .setString(
                            displayNames[i]
                        );

                    lobbyPlayerTexts[i]
                        .setPosition(
                            lobbyNamePositions[i]
                        );
                }
                else
                {
                    lobbyPlayerTexts[i]
                        .setString("");
                }
            }
        }

        if (
            currentState ==
            GameState::SinglePlayer
            )
        {
            float elapsed =
                std::min(
                    singleGameClock
                    .restart()
                    .asSeconds(),

                    0.25f
                );

            // 물리 키 위치 기준 입력: 한/영이 한글 상태여도 조작 가능
            auto keyDown =
                [](
                    sf::Keyboard::Scancode key
                    )
                {
                    return
                        sf::Keyboard::
                        isKeyPressed(key);
                };

            bool control =
                window.hasFocus() &&
                !singleScreen.menu &&
                !singleScreen.help &&
                !singleScreen.paused;

            singleInput.shopVisit =
                singleGame.shopVisit;

            singleInput.purchaseVoteId =
                singleGame
                .purchaseVoteId;

            sf::Vector2i gameMouse =
                sf::Mouse::
                getPosition(window);

            auto windowSize =
                window.getSize();

            float gameScale =
                std::min(
                    windowSize.x /
                    640.f,

                    windowSize.y /
                    360.f
                );

            if (gameScale >= 1.f)
            {
                gameScale =
                    std::floor(
                        gameScale
                    );
            }

            float gameOffsetX =
                (
                    windowSize.x -
                    640.f *
                    gameScale
                    )
                / 2.f;

            float gameOffsetY =
                (
                    windowSize.y -
                    360.f *
                    gameScale
                    )
                / 2.f;

            singleInput.aim =
            {
                (
                    static_cast<float>(
                        gameMouse.x
                    )
                    -
                    gameOffsetX
                )
                /
                gameScale,

                (
                    static_cast<float>(
                        gameMouse.y
                    )
                    -
                    gameOffsetY
                )
                /
                gameScale
            };

            singleInput.x =
                control
                ?
                static_cast<float>(
                    keyDown(
                        sf::Keyboard::
                        Scancode::D
                    )
                    )
                -
                static_cast<float>(
                    keyDown(
                        sf::Keyboard::
                        Scancode::A
                    )
                    )
                :
                0.f;

            singleInput.y =
                control
                ?
                static_cast<float>(
                    keyDown(
                        sf::Keyboard::
                        Scancode::S
                    )
                    )
                -
                static_cast<float>(
                    keyDown(
                        sf::Keyboard::
                        Scancode::W
                    )
                    )
                :
                0.f;

            singleInput.hold =
                control &&
                keyDown(
                    sf::Keyboard::
                    Scancode::E
                );

            singleInput.brace =
                control &&
                keyDown(
                    sf::Keyboard::
                    Scancode::C
                );

            if (!control)
            {
                singleInput.brace =
                    false;

                singleInput
                    .edgesOff();
            }

            singleGameMode.update(
                singleGame,
                singleInput,
                elapsed,
                control
            );
        }
        else
        {
            singleGameClock.restart();
        }

        if (
            currentState ==
            GameState::MultiGame
            )
        {
            float elapsed =
                std::min(
                    multiGameClock
                    .restart()
                    .asSeconds(),

                    0.25f
                );

            // 물리 키 위치 기준 입력: 한/영이 한글 상태여도 조작 가능
            auto keyDown =
                [](
                    sf::Keyboard::Scancode key
                    )
                {
                    return
                        sf::Keyboard::
                        isKeyPressed(key);
                };

            bool control =
                window.hasFocus() &&
                !multiScreen.help &&
                !multiScreen.paused &&
                !multiChatActive;

            multiInput.shopVisit =
                multiGame.shopVisit;

            multiInput.purchaseVoteId =
                multiGame
                .purchaseVoteId;

            sf::Vector2i gameMouse =
                sf::Mouse::
                getPosition(window);

            auto windowSize =
                window.getSize();

            float gameScale =
                std::min(
                    windowSize.x /
                    640.f,

                    windowSize.y /
                    360.f
                );

            if (gameScale <= 0.f)
            {
                gameScale = 1.f;
            }

            float gameOffsetX =
                (
                    windowSize.x -
                    640.f *
                    gameScale
                    )
                / 2.f;

            float gameOffsetY =
                (
                    windowSize.y -
                    360.f *
                    gameScale
                    )
                / 2.f;

            multiInput.aim =
            {
                (
                    static_cast<float>(
                        gameMouse.x
                    )
                    -
                    gameOffsetX
                )
                /
                gameScale,

                (
                    static_cast<float>(
                        gameMouse.y
                    )
                    -
                    gameOffsetY
                )
                /
                gameScale
            };

            multiInput.x =
                control
                ?
                static_cast<float>(
                    keyDown(
                        sf::Keyboard::
                        Scancode::D
                    )
                    )
                -
                static_cast<float>(
                    keyDown(
                        sf::Keyboard::
                        Scancode::A
                    )
                    )
                :
                0.f;

            multiInput.y =
                control
                ?
                static_cast<float>(
                    keyDown(
                        sf::Keyboard::
                        Scancode::S
                    )
                    )
                -
                static_cast<float>(
                    keyDown(
                        sf::Keyboard::
                        Scancode::W
                    )
                    )
                :
                0.f;

            multiInput.hold =
                control &&
                keyDown(
                    sf::Keyboard::
                    Scancode::E
                );

            multiInput.brace =
                control &&
                keyDown(
                    sf::Keyboard::
                    Scancode::C
                );

            if (!control)
            {
                multiInput.x = 0.f;
                multiInput.y = 0.f;

                multiInput.hold =
                    false;

                multiInput.brace =
                    false;

                multiInput
                    .edgesOff();
            }

            multiGameMode.update(
                multiGame,
                multiInput,
                elapsed
            );

            multiGameMode.updateScreen(
                multiScreen
            );

            // 플레이어 번호 대신 실제 닉네임을 캐릭터 머리 위/HUD에 표시합니다.
            multiScreen.playerNames.fill("");

            if (networkManager.isServerRunning())
            {
                multiScreen.playerNames[0] =
                    toUtf8String(nickname);

                int id = 1;
                for (const auto& name :
                    networkManager.getPlayerNicknames())
                {
                    if (!name.isEmpty() &&
                        id < dw::MaxPlayers)
                    {
                        multiScreen.playerNames[id++] =
                            toUtf8String(name);
                    }
                }
            }
            else
            {
                const auto& names =
                    networkManager.getSyncedPlayerNicknames();

                for (std::size_t i = 0;
                    i < names.size() &&
                    i < static_cast<std::size_t>(dw::MaxPlayers);
                    ++i)
                {
                    multiScreen.playerNames[i] =
                        toUtf8String(names[i]);
                }
            }

            multiScreen.chatVisible = true;
            // [김초아 담당] 네트워크 채팅 데이터를 SFML 렌더링 UI에 전달
            multiScreen.chatActive = multiChatActive;
            multiScreen.chatInput =
                toUtf8String(multiChatInput);
            multiScreen.chatLines.clear();

            for (const auto& chat :
                networkManager.getChatMessages())
            {
                multiScreen.chatLines.push_back(
                    toUtf8String(chat.sender) +
                    ": " +
                    toUtf8String(chat.message)
                );
            }
        }
        else
        {
            multiGameClock.restart();
        }

        singleScreen.playerNames.fill("");
        singleScreen.playerNames[0] =
            toUtf8String(nickname);

        sf::Vector2i mousePixel =
            sf::Mouse::getPosition(
                window
            );

        sf::Vector2f mousePos(
            static_cast<float>(
                mousePixel.x
                ),
            static_cast<float>(
                mousePixel.y
                )
        );

        window.clear();

        if (
            currentState ==
            GameState::NicknameInput
            )
        {
            window.draw(
                nicknameBackground
            );

            leftCenterText(
                nicknameText,
                468.f,
                354.f,
                68.f
            );

            window.draw(
                nicknameText
            );

            if (nicknameInputActive)
            {
                sf::FloatRect textBounds =
                    nicknameText
                    .getLocalBounds();

                nicknameCursor.setPosition({
                    nicknameText
                    .getPosition().x
                    +
                    textBounds.position.x
                    +
                    textBounds.size.x
                    +
                    3.f,

                    375.f
                    });

                int blink =
                    static_cast<int>(
                        nicknameCursorClock
                        .getElapsedTime()
                        .asSeconds()
                        *
                        2
                        )
                    %
                    2;

                if (blink == 0)
                {
                    window.draw(
                        nicknameCursor
                    );
                }
            }

            drawHover(
                window,
                buttonHover,
                confirmButton,
                mousePos
            );

            window.draw(
                nicknameInputArea
            );

            window.draw(
                confirmButton
            );
        }

        else if (
            currentState ==
            GameState::MainMenu
            )
        {
            window.draw(
                mainMenuBackground
            );
            singleRenderer.drawMenuAccents(window);

            window.draw(
                welcomeText
            );

            drawHover(
                window,
                buttonHover,
                createRoomButton,
                mousePos
            );

            drawHover(
                window,
                buttonHover,
                joinRoomButton,
                mousePos
            );

            drawHover(
                window,
                buttonHover,
                singleButton,
                mousePos
            );

            drawHover(
                window,
                buttonHover,
                exitButton,
                mousePos
            );

            drawHover(
                window,
                buttonHover,
                mainBackButton,
                mousePos
            );

            window.draw(
                createRoomButton
            );

            window.draw(
                joinRoomButton
            );

            window.draw(
                singleButton
            );

            window.draw(
                exitButton
            );

            window.draw(
                mainBackButton
            );

            drawHover(
                window,
                buttonHover,
                mainHelpButton,
                mousePos
            );

            window.draw(mainHelpButton);
            centerText(
                mainHelpButtonText,
                1045.f,
                622.f,
                190.f,
                54.f
            );
            window.draw(mainHelpButtonText);

            if (mainHelpVisible)
            {
                sf::RectangleShape shade({ 1280.f, 720.f });
                shade.setFillColor(sf::Color(0, 0, 0, 150));
                window.draw(shade);

                sf::RectangleShape helpPanel({ 780.f, 540.f });
                helpPanel.setPosition({ 250.f, 90.f });
                helpPanel.setFillColor(sf::Color(15, 31, 43, 245));
                helpPanel.setOutlineThickness(2.f);
                helpPanel.setOutlineColor(sf::Color(205, 177, 118));
                window.draw(helpPanel);

                auto drawHelpLine = [&](const wchar_t* line, float y, unsigned size = 18)
                    {
                        sf::Text t(font);
                        t.setString(sf::String(line));
                        t.setCharacterSize(size);
                        t.setFillColor(sf::Color(235, 238, 232));
                        t.setPosition({ 290.f, y });
                        window.draw(t);
                    };

                drawHelpLine(L"최종 조작법", 118.f, 30);
                drawHelpLine(L"WASD : 이동   /   Q : 들고 있는 포탄 내려놓기", 170.f);
                drawHelpLine(L"1~6 : 탄약 선택  (1 일반 / 2 확산 / 3 관통 / 4 폭발 / 5 화염 / 6 중포)", 205.f, 16);
                drawHelpLine(L"탄약고 : 원하는 탄약 선택 후 E → 선택한 포탄 획득", 240.f);
                drawHelpLine(L"대포 : 포탄을 들고 E 길게 장전 → 마우스 조준 → Space 발사", 275.f);
                drawHelpLine(L"파손·화재 : 해당 위치에서 E를 길게 눌러 수리·소화", 310.f);
                drawHelpLine(L"낚시 : 낚시터에서 E 시작/취소   /   H : 물고기 사용(체력 회복)", 345.f, 17);
                drawHelpLine(L"C : 근접 공격   /   큰 파도·강풍 공격은 C를 길게 눌러 방어", 380.f, 17);
                drawHelpLine(L"상점 : 마우스로 구매·찬반 투표   /   Enter : 출항 준비", 415.f, 17);
                drawHelpLine(L"멀티 채팅 : Tab 입력 시작 → Enter 전송 → Tab 입력 종료", 450.f, 17);
                drawHelpLine(L"F1 : 게임 중 조작법 열기/닫기", 485.f, 17);
                drawHelpLine(L"Esc 또는 F10 : 게임 종료 후 메인 메뉴로 복귀", 520.f, 17);
                drawHelpLine(L"진행 : 바다 Stage 1~3 → 섬 Stage 1~3", 565.f, 16);
            }
        }

        else if (
            currentState ==
            GameState::CreateRoom
            )
        {
            window.draw(
                createRoomBackground
            );

            leftCenterText(
                roomNameText,
                580.f,
                278.f,
                55.f
            );

            window.draw(
                roomNameText
            );

            window.draw(
                roomNameInputArea
            );

            drawHover(
                window,
                buttonHover,
                roomCodeCopyArea,
                mousePos
            );

            centerText(
                roomCodeText,
                371.f,
                505.f,
                350.f,
                56.f
            );

            window.draw(
                roomCodeText
            );

            window.draw(
                roomCodeCopyArea
            );

            drawHover(
                window,
                buttonHover,
                player2Button,
                mousePos
            );

            drawHover(
                window,
                buttonHover,
                player3Button,
                mousePos
            );

            drawHover(
                window,
                buttonHover,
                player4Button,
                mousePos
            );

            drawHover(
                window,
                buttonHover,
                player5Button,
                mousePos
            );

            drawHover(
                window,
                buttonHover,
                regenerateButton,
                mousePos
            );

            drawHover(
                window,
                buttonHover,
                finalCreateRoomButton,
                mousePos
            );

            drawHover(
                window,
                buttonHover,
                createBackButton,
                mousePos
            );

            if (selectedPlayers == 2)
            {
                selectedHighlight.setSize(
                    player2Button.getSize()
                );

                selectedHighlight.setPosition(
                    player2Button.getPosition()
                );

                window.draw(
                    selectedHighlight
                );
            }

            else if (
                selectedPlayers == 3
                )
            {
                selectedHighlight.setSize(
                    player3Button.getSize()
                );

                selectedHighlight.setPosition(
                    player3Button.getPosition()
                );

                window.draw(
                    selectedHighlight
                );
            }

            else if (
                selectedPlayers == 4
                )
            {
                selectedHighlight.setSize(
                    player4Button.getSize()
                );

                selectedHighlight.setPosition(
                    player4Button.getPosition()
                );

                window.draw(
                    selectedHighlight
                );
            }

            else if (
                selectedPlayers == 5
                )
            {
                selectedHighlight.setSize(
                    player5Button.getSize()
                );

                selectedHighlight.setPosition(
                    player5Button.getPosition()
                );

                window.draw(
                    selectedHighlight
                );
            }

            window.draw(
                player2Button
            );

            window.draw(
                player3Button
            );

            window.draw(
                player4Button
            );

            window.draw(
                player5Button
            );

            window.draw(
                regenerateButton
            );

            window.draw(
                finalCreateRoomButton
            );

            window.draw(
                createBackButton
            );
        }

        else if (
            currentState ==
            GameState::JoinRoom
            )
        {
            window.draw(
                joinRoomBackground
            );

            leftCenterText(
                joinCodeText,
                472.f,
                389.f,
                68.f
            );

            window.draw(
                joinCodeText
            );

            drawHover(
                window,
                buttonHover,
                joinConfirmButton,
                mousePos
            );

            drawHover(
                window,
                buttonHover,
                joinBackButton,
                mousePos
            );

            window.draw(
                joinInputArea
            );

            window.draw(
                joinConfirmButton
            );

            window.draw(
                joinBackButton
            );
        }

        else if (
            currentState ==
            GameState::Lobby
            )
        {
            window.draw(
                lobbyBackground
            );

            window.draw(
                lobbyRoomNameText
            );

            drawHover(
                window,
                buttonHover,
                lobbyRoomCodeCopyArea,
                mousePos
            );

            window.draw(
                lobbyRoomCodeText
            );

            window.draw(
                lobbyRoomCodeCopyArea
            );

            drawHover(
                window,
                buttonHover,
                lobbyStartButton,
                mousePos
            );

            drawHover(
                window,
                buttonHover,
                lobbyBackButton,
                mousePos
            );

            if (
                networkManager
                .isServerRunning()
                )
            {
                for (
                    std::size_t i = 0;
                    i <
                    lobbyKickButtons.size();
                    ++i
                    )
                {
                    if (
                        i + 1 <
                        lobbyVisiblePlayerCount
                        )
                    {
                        drawHover(
                            window,
                            buttonHover,
                            lobbyKickButtons[i],
                            mousePos
                        );
                    }
                }
            }

            window.draw(
                lobbyStartButton
            );

            window.draw(
                lobbyBackButton
            );

            for (
                const auto& kickButton :
                lobbyKickButtons
                )
            {
                window.draw(
                    kickButton
                );
            }

            for (
                auto& playerText :
                lobbyPlayerTexts
                )
            {
                window.draw(
                    playerText
                );
            }
        }

        else if (
            currentState ==
            GameState::MultiGame
            )
        {
            {
                const auto pixel = sf::Mouse::getPosition(window);
                const auto size = window.getSize();
                const float scale = std::min(size.x / 640.f, size.y / 360.f);
                multiScreen.pointer = {(pixel.x - (size.x - 640.f * scale) * .5f) / scale,
                    (pixel.y - (size.y - 360.f * scale) * .5f) / scale};
            }
            multiRenderer.draw(
                multiCanvas,
                multiGame,
                multiScreen
            );

            multiCanvas.display();

            sf::Sprite multiGameSprite(
                multiCanvas.getTexture()
            );

            auto windowSize =
                window.getSize();

            float scale =
                std::min(
                    windowSize.x /
                    640.f,

                    windowSize.y /
                    360.f
                );

            float offsetX =
                (
                    windowSize.x -
                    640.f *
                    scale
                    )
                / 2.f;

            float offsetY =
                (
                    windowSize.y -
                    360.f *
                    scale
                    )
                / 2.f;

            multiGameSprite.setScale({
                scale,
                scale
                });

            multiGameSprite.setPosition({
                offsetX,
                offsetY
                });

            window.draw(
                multiGameSprite
            );
        }

        else if (
            currentState ==
            GameState::SinglePlayer
            )
        {
            {
                const auto pixel = sf::Mouse::getPosition(window);
                const auto size = window.getSize();
                float scale = std::min(size.x / 640.f, size.y / 360.f);
                if (scale >= 1.f) scale = std::floor(scale);
                singleScreen.pointer = {(pixel.x - (size.x - 640.f * scale) * .5f) / scale,
                    (pixel.y - (size.y - 360.f * scale) * .5f) / scale};
            }
            singleRenderer.draw(
                singleCanvas,
                singleGame,
                singleScreen
            );

            singleCanvas.display();

            sf::Sprite singleGameSprite(
                singleCanvas.getTexture()
            );

            auto windowSize =
                window.getSize();

            float scale =
                std::min(
                    windowSize.x /
                    640.f,

                    windowSize.y /
                    360.f
                );

            if (scale >= 1.f)
            {
                scale =
                    std::floor(scale);
            }

            float offsetX =
                (
                    windowSize.x -
                    640.f *
                    scale
                    )
                / 2.f;

            float offsetY =
                (
                    windowSize.y -
                    360.f *
                    scale
                    )
                / 2.f;

            singleGameSprite.setScale({
                scale,
                scale
                });

            singleGameSprite.setPosition({
                offsetX,
                offsetY
                });

            window.draw(
                singleGameSprite
            );
        }

        window.display();
    }

    return 0;
}
