pragma Singleton
import QtQuick
import Gambasse

// Application theme: the claro/oscuro palettes measured on the former Widgets
// windows. `mode` follows the persisted InterfaceSettings theme and every
// color below follows `mode`, so switching the theme updates the whole
// interface in hot through bindings.
QtObject {
    // Bound to the settings; the component tests assign it directly.
    property string mode: InterfaceSettings.theme

    readonly property bool dark: mode === "oscuro"

    // Base metrics (same base UI as the Widgets windows).
    readonly property int baseWidth: 1008
    readonly property int baseHeight: 561
    // Title bar: its buttons take the 40 px of the Basic tool buttons
    // (language and theme) and the bar adds its 1 px bottom line, so those
    // buttons fit exactly.
    readonly property int titleBarButtonHeight: 40
    readonly property int titleBarHeight: titleBarButtonHeight + 1
    // Height of a line edit or combo box in the Widgets windows.
    readonly property int fieldHeight: 22
    // Narrowest a squeezed line edit gets beside a long label.
    readonly property int fieldMinimumWidth: 28

    // Title bar: a light (claro) or muted dark (oscuro) blue-green that
    // stands out from the window grey, with a darker separating line.
    readonly property color titleBarBackground: dark ? "#243B3A" : "#DCEEEA"
    readonly property color titleBarBorder: dark ? "#1A2C2B" : "#B7D6CF"

    // Window and text.
    readonly property color windowBackground: dark ? "#353535" : "#F3F2EE"
    readonly property color windowText: dark ? "white" : "#1A1A1A"
    readonly property color mutedText: dark ? "#969696" : "#A6A6A6"
    // Text of a disabled line edit (the display-only fields).
    readonly property color disabledText: dark ? "#7A7A7A" : "#A0A0A0"
    // Placeholder text: the text color at half opacity, like QPalette's.
    readonly property color placeholderText: dark ? Qt.rgba(1, 1, 1, 0.5) : Qt.rgba(0.102, 0.102, 0.102, 0.5)

    // Editable areas.
    readonly property color fieldBackground: dark ? "#232323" : "#FBFAF6"
    readonly property color fieldBorder: dark ? "#1B3550" : "#9FC3E8"
    readonly property color focusBackground: dark ? "#14375A" : "#CCE8FF"
    readonly property color focusBorder: dark ? "#53A0ED" : "#316598"
    readonly property color focusRing: dark ? "#1E4771" : "#B4D4EF"
    readonly property color requiredBackground: "#FFE4C4"

    // Chrome of the standard controls, taken from the Widgets windows (the
    // palette the application installs: line edits, group boxes and check
    // boxes are painted by the platform style, not by a stylesheet).
    readonly property color inputBorder: dark ? "#262626" : "#AEADAA"
    // Fusion's sunken line just inside the top border of a line edit / box.
    readonly property color fieldShadow: dark ? "#212121" : "#E9E8E5"
    readonly property color checkShadow: dark ? "#212121" : "#EDECE8"
    // Combo boxes and tab headers (Fusion button gradient, top and bottom
    // light lines, drop-down arrow), measured on the Widgets windows.
    readonly property color buttonTop: dark ? "#4A4A4A" : "#FDFDFC"
    readonly property color buttonBottom: dark ? "#3F3F3F" : "#EAE9E7"
    readonly property color buttonTopLine: dark ? "#606060" : "#FDFDFC"
    readonly property color buttonBottomLine: dark ? "#565656" : "#ECECEA"
    readonly property color comboArrow: dark ? "#BABABA" : "#6B6B6B"
    readonly property color tabPage: dark ? "#434343" : "#F8F6F1"
    readonly property color tabTop: dark ? "#444444" : "#FFFEFB"
    readonly property color tabTopLine: dark ? "#5B5B5B" : "#FFFEFB"
    readonly property color tabInactiveTop: dark ? "#3E3E3E" : "#E8E7E3"
    readonly property color tabInactiveBottom: dark ? "#3C3C3C" : "#E5E4DF"
    readonly property color tabInactiveTopLine: dark ? "#555555" : "#E8E7E3"
    // Sections (GxGroup): a rounded card in a soft blue that stands out from
    // the window grey without looking like a button (the buttons use the
    // stronger accentBackground).
    readonly property int groupRadius: 6
    readonly property color groupBackground: dark ? "#2E3A46" : "#EAF1F8"
    readonly property color groupBorder: dark ? "#41505E" : "#C5D5E4"
    // Title badge of a section: the theme's text colours inverted.
    readonly property color groupTitleBackground: dark ? "#B7CCE0" : "#5A7FA6"
    readonly property color groupTitleText: dark ? "#1A1A1A" : "white"
    readonly property color checkBorder: dark ? "#2A2A2A" : "#BFBEBB"

    // Highlighted buttons (toolbar, history entry/create, dialog buttons).
    readonly property color accentBackground: dark ? "#27496D" : "#CFE2F3"
    readonly property color accentHover: dark ? "#2F5A85" : "#BBD6EE"
    readonly property color accentPressed: dark ? "#1B3550" : "#A9C9E8"
    readonly property color accentBorder: dark ? "#1B3550" : "#9FC3E8"
    readonly property color accentText: dark ? "white" : "#1A1A1A"
    readonly property color accentDisabledBackground: dark ? "#3A3A3A" : "#E4E3DF"
    readonly property color accentDisabledText: dark ? "#7A7A7A" : "#A6A6A6"
    readonly property color accentDisabledBorder: dark ? "#333333" : "#CFCFCF"

    // Selection and danger.
    readonly property color highlight: dark ? "#2A82DA" : "#3D7EBE"
    readonly property color highlightedText: dark ? "black" : "white"
    readonly property color alternateRowBackground: dark ? "#353535" : "#ECEAE3"
    readonly property color danger: "#E81123"
    readonly property color dangerPressed: "#A00A18"
    // Veil over the patient screen while a history or consultation window
    // is open on top of it, so only the window in front looks active.
    readonly property color modalVeil: dark ? Qt.rgba(0, 0, 0, 0.6) : Qt.rgba(0, 0, 0, 0.3)
    readonly property int modalVeilFade: 150

    // Check boxes and label highlight.
    readonly property color checkFocusBackground: dark ? "#1E3A55" : "#D9ECFA"
    readonly property color checkedText: dark ? "#E89A9A" : "#800000"
    readonly property color labelHighlight: "#1C3A75"
}
