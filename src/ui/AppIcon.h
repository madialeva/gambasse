#pragma once

#include <QIcon>

namespace gambasse {

// The application icon: the logo of the title bar (qrc:/img/logo.svg),
// rasterised at the usual desktop sizes so the task bar, the window switcher
// and every window share it without depending on the SVG icon engine plugin.
QIcon applicationIcon();

} // namespace gambasse
