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
                                       &ZoomController::recompute);
    emit settingsChanged();
    recompute();
}

void ZoomController::setWindow(QWindow* window) {
    if (window == m_window)
        return;
    disconnect(m_screenConnection);
    m_window = window;
    if (m_window)
        m_screenConnection =
            connect(m_window, &QWindow::screenChanged, this, &ZoomController::followScreen);
    emit windowChanged();
    followScreen();
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
    recompute();
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
    const QSize size = windowSize();
    // The first time, and after the size shown changed while it was hidden
    // (a zoom made in the other window), it is centred; otherwise it opens
    // where it was left.
    const QRect wanted =
        m_placedSize != m_shown ? centred(size) : QRect(m_window->position(), size);
    m_placedSize = m_shown;
    applyGeometry(inArea(wanted));
}

void ZoomController::recompute() {
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
    const bool sizeChanged = size != m_windowSize;
    const bool changed = shown != m_shown || sizeChanged || canZoomIn != m_canZoomIn;
    m_shown = shown;
    m_windowSize = size;
    m_canZoomIn = canZoomIn;
    if (changed)
        emit zoomChanged();
    // The bindings of the window have taken the new size by now; a visible
    // window is placed again at once, a hidden one when it is shown.
    if (!m_window || !m_window->isVisible())
        return;
    if (m_shown != m_placedSize) {
        m_placedSize = m_shown;
        applyGeometry(inArea(centred(windowSize())));
    } else if (sizeChanged) {
        applyGeometry(inArea(QRect(m_window->position(), m_windowSize)));
    }
}

void ZoomController::followScreen() {
    disconnect(m_areaConnection);
    QScreen* screen = m_window && m_window->screen() ? m_window->screen()
                                                     : QGuiApplication::primaryScreen();
    const auto update = [this, screen] {
        const QRect area = screen ? screen->availableGeometry() : QRect();
        if (area == m_screenArea)
            return;
        m_screenArea = area;
        if (!m_fixedArea.isValid()) {
            emit availableAreaChanged();
            recompute();
        }
    };
    if (screen)
        m_areaConnection = connect(screen, &QScreen::availableGeometryChanged, this, update);
    update();
}

QRect ZoomController::centred(QSize size) const {
    const QWindow* parent = m_window ? m_window->transientParent() : nullptr;
    const QPoint centre = parent ? parent->geometry().center() : availableArea().center();
    QRect rect(QPoint(0, 0), size);
    rect.moveCenter(centre);
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
    if (m_window && m_window->geometry() != rect)
        m_window->setGeometry(rect);
}

} // namespace gambasse
