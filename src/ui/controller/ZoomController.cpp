#include <ui/controller/ZoomController.h>

#include <QGuiApplication>
#include <QScreen>
#include <algorithm>

#include <logic/ZoomLevels.h>

namespace gambasse {

ZoomController::ZoomController(QObject* parent)
    : QObject(parent), m_shown(zoom::baseLetterSize),
      m_windowSize(zoom::windowSize(m_baseSize, m_shown)) {
    followScreen();
}

void ZoomController::setSettings(InterfaceSettings* settings) {
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

void ZoomController::setWindow(QWindow* window) {
    if (window == m_window)
        return;
    disconnect(m_screenConnection);
    disconnect(m_parentConnection);
    disconnect(m_visibleConnection);
    disconnect(m_parentScreenConnection);
    m_window = window;
    if (m_window) {
        m_screenConnection =
            connect(m_window, &QWindow::screenChanged, this, &ZoomController::followScreen);
        m_parentConnection = connect(m_window, &QWindow::transientParentChanged, this,
                                     &ZoomController::followParent);
        m_visibleConnection =
            connect(m_window, &QWindow::visibleChanged, this, &ZoomController::followScreen);
    }
    emit windowChanged();
    followParent();
}

void ZoomController::setBaseSize(QSize size) {
    if (size == m_baseSize)
        return;
    m_baseSize = size;
    emit baseSizeChanged();
    recompute();
}

QRect ZoomController::availableArea() const {
    return m_fixedArea.isValid() ? m_fixedArea : m_screenArea;
}

void ZoomController::setAvailableArea(QRect area) {
    if (area == m_fixedArea)
        return;
    m_fixedArea = area;
    emit availableAreaChanged();
    recompute(true);
}

void ZoomController::resetAvailableArea() {
    setAvailableArea(QRect());
}

qreal ZoomController::factor() const {
    return zoom::factor(m_shown);
}

bool ZoomController::canZoomOut() const {
    return m_shown > zoom::baseLetterSize;
}

void ZoomController::zoomIn() {
    if (m_settings && canZoomIn())
        m_settings->setLetterSize(m_shown + 1);
}

void ZoomController::zoomOut() {
    // From the size shown, not the chosen one: a window held back by its
    // screen gets smaller at once.
    if (m_settings && canZoomOut())
        m_settings->setLetterSize(m_shown - 1);
}

void ZoomController::reset() {
    if (m_settings)
        m_settings->setLetterSize(zoom::baseLetterSize);
}

void ZoomController::placeForShow() {
    if (!m_window || m_window->isVisible())
        return;
    if (QScreen* screen = hostScreen(); screen && m_window->screen() != screen)
        m_window->setScreen(screen);
    followScreen();
    applyGeometry(inArea(centred(windowSize())));
}

void ZoomController::recompute(bool areaChanged) {
    const int wanted = m_settings ? m_settings->letterSize() : zoom::baseLetterSize;
    const int shown = zoom::fittingLetterSize(m_baseSize, wanted, availableArea().size());
    const QSize size = zoom::windowSize(m_baseSize, shown);
    const bool canZoomIn =
        shown < zoom::maximumLetterSize
        && zoom::fittingLetterSize(m_baseSize, shown + 1, availableArea().size()) > shown;
    // Against what was last announced, not against the inputs, which have
    // already changed when this runs: a form of another height changes the
    // window size at the same letter size, and a larger screen the room to
    // zoom in without changing either.
    const int previousShown = m_shown;
    const bool sizeChanged = size != m_windowSize;
    const bool changed = shown != previousShown || sizeChanged || canZoomIn != m_canZoomIn;
    m_shown = shown;
    m_windowSize = size;
    m_canZoomIn = canZoomIn;
    if (changed)
        emit zoomChanged();
    // The bindings of the window have taken the new size by now; a visible
    // window is placed again at once, a hidden one when it is shown.
    if (!m_window || !m_window->isVisible())
        return;
    if (m_shown != previousShown)
        applyGeometry(inArea(centred(m_windowSize)));
    else if (sizeChanged || areaChanged)
        applyGeometry(inArea(QRect(m_window->position(), m_windowSize)));
}

void ZoomController::followParent() {
    disconnect(m_parentScreenConnection);
    if (m_window) {
        if (QWindow* parent = m_window->transientParent())
            m_parentScreenConnection = connect(parent, &QWindow::screenChanged, this,
                                               &ZoomController::followScreen);
    }
    followScreen();
}

QScreen* ZoomController::hostScreen() const {
    if (!m_window)
        return QGuiApplication::primaryScreen();
    if (!m_window->isVisible()) {
        if (const QWindow* parent = m_window->transientParent();
            parent && parent->screen())
            return parent->screen();
    }
    return m_window->screen() ? m_window->screen() : QGuiApplication::primaryScreen();
}

void ZoomController::followScreen() {
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

QRect ZoomController::centred(QSize size) const {
    const QRect area = availableArea();
    QRect rect(QPoint(0, 0), size);
    if (area.isValid())
        rect.moveCenter(area.center());
    return rect;
}

QRect ZoomController::inArea(QRect rect) const {
    const QRect area = availableArea();
    if (!area.isValid())
        return rect;
    const auto fit = [](int start, int length, int areaStart, int areaLength) {
        if (length >= areaLength)
            return areaStart;
        return std::clamp(start, areaStart, areaStart + areaLength - length);
    };
    rect.moveTo(fit(rect.x(), rect.width(), area.x(), area.width()),
                fit(rect.y(), rect.height(), area.y(), area.height()));
    return rect;
}

void ZoomController::applyGeometry(QRect rect) {
    if (!m_window)
        return;
    if (!m_window->isVisible() || m_window->geometry() != rect)
        m_window->setGeometry(rect);
}

} // namespace gambasse
