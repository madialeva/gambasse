#include <ui/controller/ShellWindowController.h>

#include <QGuiApplication>
#include <QLibrary>
#include <QScopeGuard>
#include <QScreen>
#include <cstdint>
#include <cstdlib>

#include <logic/AppSettings.h>
#include <logic/ZoomLevels.h>

namespace gambasse {
namespace {

#if defined(Q_OS_LINUX) && QT_CONFIG(xcb)
// Sends ICCCM 4.1.4 WM_CHANGE_STATE (IconicState) directly on Qt's XCB
// connection without clearing _NET_WM_STATE_MAXIMIZED_{HORZ,VERT}.
// QWindow::showMinimized() strips Qt::WindowMaximized first, which makes X11
// window managers (such as KWin) un-maximize the window and play a
// small-to-maximized flash on de-iconify. Resolved dynamically from the
// already-loaded libxcb.so.1 so no compile-time XCB headers or link flags are
// introduced.
bool iconifyOnX11(QWindow* window) {
    auto* x11 = qGuiApp ? qGuiApp->nativeInterface<QNativeInterface::QX11Application>() : nullptr;
    if (!x11 || !x11->connection())
        return false;

    struct AtomCookie {
        uint32_t sequence;
    };
    struct AtomReply {
        uint8_t responseType;
        uint8_t pad0;
        uint16_t sequence;
        uint32_t length;
        uint32_t atom;
    };
    struct Screen {
        uint32_t root;
    };
    struct ScreenIterator {
        Screen* data;
        int rem;
        int index;
    };
    struct ClientMessage {
        uint8_t responseType;
        uint8_t format;
        uint16_t sequence;
        uint32_t window;
        uint32_t type;
        uint32_t data32[5];
    };
    static_assert(sizeof(ClientMessage) == 32);

    using GetSetupFn = const void* (*)(xcb_connection_t*);
    using RootsIterFn = ScreenIterator (*)(const void*);
    using InternAtomFn = AtomCookie (*)(xcb_connection_t*, uint8_t, uint16_t, const char*);
    using InternAtomReplyFn = AtomReply* (*)(xcb_connection_t*, AtomCookie, void**);
    using SendEventFn = uint32_t (*)(xcb_connection_t*, uint8_t, uint32_t, uint32_t, const char*);
    using FlushFn = int (*)(xcb_connection_t*);

    static auto* lib = [] {
        auto* l = new QLibrary(QStringLiteral("xcb"), 1);
        l->load();
        return l;
    }();
    static const auto getSetup = reinterpret_cast<GetSetupFn>(lib->resolve("xcb_get_setup"));
    static const auto rootsIter =
        reinterpret_cast<RootsIterFn>(lib->resolve("xcb_setup_roots_iterator"));
    static const auto internAtom = reinterpret_cast<InternAtomFn>(lib->resolve("xcb_intern_atom"));
    static const auto internAtomReply =
        reinterpret_cast<InternAtomReplyFn>(lib->resolve("xcb_intern_atom_reply"));
    static const auto sendEvent = reinterpret_cast<SendEventFn>(lib->resolve("xcb_send_event"));
    static const auto flush = reinterpret_cast<FlushFn>(lib->resolve("xcb_flush"));
    if (!getSetup || !rootsIter || !internAtom || !internAtomReply || !sendEvent || !flush)
        return false;

    const uint32_t winId = static_cast<uint32_t>(window->winId());
    if (winId == 0)
        return false;

    xcb_connection_t* conn = x11->connection();
    const ScreenIterator iter = rootsIter(getSetup(conn));
    if (!iter.data || iter.rem <= 0)
        return false;
    const uint32_t rootWindow = iter.data->root;

    static constexpr char atomName[] = "WM_CHANGE_STATE";
    AtomReply* reply =
        internAtomReply(conn, internAtom(conn, 0, sizeof(atomName) - 1, atomName), nullptr);
    if (!reply || reply->atom == 0) {
        std::free(reply);
        return false;
    }
    const uint32_t wmChangeState = reply->atom;
    std::free(reply);

    ClientMessage ev{};
    ev.responseType = 33; // XCB_CLIENT_MESSAGE
    ev.format = 32;
    ev.window = winId;
    ev.type = wmChangeState;
    ev.data32[0] = 3; // IconicState (ICCCM 4.1.4)
    // SubstructureRedirectMask (0x100000) | SubstructureNotifyMask (0x080000)
    sendEvent(conn, 0, rootWindow, 0x00180000u, reinterpret_cast<const char*>(&ev));
    flush(conn);
    return true;
}
#endif

} // namespace

ShellWindowController::ShellWindowController(QObject* parent)
    : QObject(parent), m_shown(zoom::baseLetterSize),
      m_minimumSize(zoom::windowSize(m_baseSize, m_shown)) {
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(150);
    connect(&m_saveTimer, &QTimer::timeout, this, &ShellWindowController::persistGeometry);
    followScreen();
}

ShellWindowController::~ShellWindowController() {
    flushPendingSave();
}

void ShellWindowController::setSettings(InterfaceSettings* settings) {
    if (settings == m_settings)
        return;
    disconnect(m_settingsConnection);
    m_settings = settings;
    if (m_settings)
        m_settingsConnection = connect(m_settings, &InterfaceSettings::letterSizeChanged, this,
                                       [this] { recompute(); });
    emit settingsChanged();
    recompute();
}

void ShellWindowController::setWindow(QWindow* window) {
    if (window == m_window)
        return;
    disconnect(m_screenConnection);
    disconnect(m_visibilityConnection);
    disconnect(m_widthConnection);
    disconnect(m_heightConnection);
    m_window = window;
    if (m_window) {
        m_screenConnection =
            connect(m_window, &QWindow::screenChanged, this, &ShellWindowController::followScreen);
        m_visibilityConnection = connect(m_window, &QWindow::visibilityChanged, this,
                                         &ShellWindowController::onVisibilityChanged);
        m_widthConnection = connect(m_window, &QWindow::widthChanged, this,
                                    &ShellWindowController::onWindowResized);
        m_heightConnection = connect(m_window, &QWindow::heightChanged, this,
                                     &ShellWindowController::onWindowResized);
    }
    emit windowChanged();
    followScreen();
}

void ShellWindowController::setBaseSize(QSize size) {
    if (size == m_baseSize)
        return;
    m_baseSize = size;
    emit baseSizeChanged();
    recompute();
}

QRect ShellWindowController::availableArea() const {
    return m_fixedArea.isValid() ? m_fixedArea : m_screenArea;
}

void ShellWindowController::setAvailableArea(QRect area) {
    if (area == m_fixedArea)
        return;
    m_fixedArea = area;
    emit availableAreaChanged();
    recompute(true);
}

void ShellWindowController::resetAvailableArea() {
    setAvailableArea(QRect());
}

qreal ShellWindowController::factor() const {
    return zoom::factor(m_shown);
}

bool ShellWindowController::canZoomOut() const {
    return m_shown > zoom::baseLetterSize;
}

void ShellWindowController::zoomIn() {
    if (m_settings && canZoomIn())
        m_settings->setLetterSize(m_shown + 1);
}

void ShellWindowController::zoomOut() {
    if (m_settings && canZoomOut())
        m_settings->setLetterSize(m_shown - 1);
}

void ShellWindowController::reset() {
    if (m_settings)
        m_settings->setLetterSize(zoom::baseLetterSize);
}

void ShellWindowController::applyInitialGeometry() {
    if (!m_window)
        return;
    const MainWindowGeometry saved = AppSettings().mainWindowGeometry();
    const bool stateChanged = saved.maximized != m_maximized;
    m_maximized = saved.maximized;
    m_preferredNormalSize = mainwindow::clampNormalSize(saved.normalSize);
    if (stateChanged)
        emit windowStateChanged();

    followScreen();
    recompute();

    const QRect area = availableArea();
    const QSize normalSize =
        mainwindow::effectiveNormalSize(m_preferredNormalSize, m_shown, area.size());
    const QRect normalRect = mainwindow::centredRect(normalSize, area);

    const auto guard = qScopeGuard([this] { m_applyingGeometry = false; });
    m_applyingGeometry = true;
    m_lastAppliedSize = normalRect.size();
    m_window->setMinimumSize(m_minimumSize);
    m_window->setGeometry(normalRect);
    m_initialApplied = true;
    if (m_maximized) {
        m_hadNormalPlacement = false;
        m_window->showMaximized();
    } else {
        m_hadNormalPlacement = true;
        m_window->showNormal();
    }
}

void ShellWindowController::flushPendingSave() {
    onWindowResized();
    if (m_saveTimer.isActive()) {
        m_saveTimer.stop();
        persistGeometry();
    }
}

void ShellWindowController::minimize() {
    if (!m_window)
        return;
    const auto guard = qScopeGuard([this] { m_applyingGeometry = false; });
    m_applyingGeometry = true;
    m_minimized = true;
    m_restoringMaximized = false;
#if defined(Q_OS_LINUX) && QT_CONFIG(xcb)
    if (iconifyOnX11(m_window))
        return;
#endif
    m_window->showMinimized();
}

void ShellWindowController::recompute(bool areaChanged) {
    const int wanted = m_settings ? m_settings->letterSize() : zoom::baseLetterSize;
    const QRect area = availableArea();
    const int shown = zoom::fittingLetterSize(m_baseSize, wanted, area.size());
    const QSize minSize = zoom::windowSize(m_baseSize, shown);
    const bool canZoomIn =
        shown < zoom::maximumLetterSize
        && zoom::fittingLetterSize(m_baseSize, shown + 1, area.size()) > shown;
    const int previousShown = m_shown;
    const bool changed =
        shown != m_shown || minSize != m_minimumSize || canZoomIn != m_canZoomIn;
    const QSize previousSize = m_window ? m_window->size() : QSize();
    const QPoint previousPos = m_window ? m_window->position() : QPoint();
    // Guard before emitting zoomChanged(): QML bindings on minimumWidth and
    // minimumHeight can resize the window synchronously while zoomChanged() is
    // delivered, and those programmatic resizes must not overwrite
    // m_preferredNormalSize.
    const auto guard = qScopeGuard([this] { m_applyingGeometry = false; });
    m_applyingGeometry = true;

    m_shown = shown;
    m_minimumSize = minSize;
    m_canZoomIn = canZoomIn;
    if (changed)
        emit zoomChanged();

    if (!m_initialApplied || !m_window)
        return;

    m_window->setMinimumSize(m_minimumSize);
    if (!m_window->isVisible() || m_window->visibility() != QWindow::Windowed)
        return;

    const QSize target =
        mainwindow::effectiveNormalSize(m_preferredNormalSize, m_shown, area.size());
    QRect next(previousPos, target);
    if (areaChanged || m_shown != previousShown || target != previousSize)
        next = mainwindow::clampToArea(next, area);
    m_lastAppliedSize = next.size();
    if (next != m_window->geometry())
        m_window->setGeometry(next);
}

QScreen* ShellWindowController::hostScreen() const {
    if (m_window && m_window->screen())
        return m_window->screen();
    return QGuiApplication::primaryScreen();
}

void ShellWindowController::followScreen() {
    disconnect(m_areaConnection);
    QScreen* screen = hostScreen();
    const auto update = [this, screen] {
        const QRect area = screen ? screen->availableGeometry() : QRect();
        if (area == m_screenArea)
            return;
        m_screenArea = area;
        if (!m_fixedArea.isValid()) {
            emit availableAreaChanged();
            recompute(true);
        }
    };
    if (screen)
        m_areaConnection = connect(screen, &QScreen::availableGeometryChanged, this, update);
    update();
}

void ShellWindowController::onVisibilityChanged(QWindow::Visibility visibility) {
    if (!m_initialApplied || !m_window)
        return;
    if (visibility == QWindow::Maximized)
        m_restoringMaximized = false;
    if (m_applyingGeometry)
        return;
    if (visibility == QWindow::Hidden) {
        flushPendingSave();
        return;
    }
    // Minimizing to the taskbar must never alter the persisted startup state
    // or the dimensions the window had before iconifying.
    if (visibility == QWindow::Minimized) {
        m_minimized = true;
        m_restoringMaximized = false;
        return;
    }

    const bool wasMinimized = m_minimized;
    m_minimized = false;

    if (visibility == QWindow::Maximized) {
        m_saveTimer.stop();
        const bool stateChanged = !m_maximized;
        if (stateChanged) {
            m_maximized = true;
            emit windowStateChanged();
        }
        if (stateChanged && !wasMinimized)
            persistGeometry();
        return;
    }

    if (visibility == QWindow::Windowed) {
        // Coming back from the taskbar (or receiving a stale Windowed event
        // while recovering maximize after un-iconifying): keep the state the
        // window had before minimizing without touching config.ini.
        if ((wasMinimized || m_restoringMaximized) && m_maximized) {
            m_restoringMaximized = true;
            const auto guard = qScopeGuard([this] { m_applyingGeometry = false; });
            m_applyingGeometry = true;
            m_window->showMaximized();
            return;
        }
        if (wasMinimized)
            return;

        m_restoringMaximized = false;
        const bool wasMaximized = m_maximized;
        if (m_maximized) {
            m_maximized = false;
            emit windowStateChanged();
        }
        if (wasMaximized) {
            const QRect area = availableArea();
            const QSize target =
                mainwindow::effectiveNormalSize(m_preferredNormalSize, m_shown, area.size());
            QRect next = !m_hadNormalPlacement
                             ? mainwindow::centredRect(target, area)
                             : mainwindow::clampToArea(QRect(m_window->position(), target), area);
            m_hadNormalPlacement = true;
            const auto guard = qScopeGuard([this] { m_applyingGeometry = false; });
            m_applyingGeometry = true;
            m_lastAppliedSize = next.size();
            m_window->setMinimumSize(m_minimumSize);
            m_window->setGeometry(next);
            m_saveTimer.stop();
            persistGeometry();
        }
    }
}

void ShellWindowController::onWindowResized() {
    if (!m_initialApplied || m_applyingGeometry || !m_window || !m_window->isVisible())
        return;
    if (m_window->visibility() != QWindow::Windowed)
        return;
    const QSize current = m_window->size();
    if (current == m_lastAppliedSize)
        return;
    const QSize clamped = mainwindow::clampNormalSize(current);
    m_lastAppliedSize = current;
    if (clamped == m_preferredNormalSize)
        return;
    m_preferredNormalSize = clamped;
    m_saveTimer.start();
}

void ShellWindowController::persistGeometry() {
    AppSettings().setMainWindowGeometry({m_maximized, m_preferredNormalSize});
}

} // namespace gambasse
