pragma Singleton
import QtQuick

// Application theme: mirrors the Widgets claro/oscuro palettes so both UI
// stacks look the same. `mode` is driven by the uiSettings context property
// (one-way binding); every color below follows it, so switching the theme
// updates the whole interface in hot through bindings.
QtObject {
    // Driven from the root window (see Root.qml); defaults to claro.
    property string mode: "claro"

    readonly property bool dark: mode === "oscuro"

    // Base metrics (same base UI as the Widgets windows).
    readonly property int baseWidth: 1008
    readonly property int baseHeight: 561

    // Window and text.
    readonly property color windowBackground: dark ? "#353535" : "#F3F2EE"
    readonly property color windowText: dark ? "white" : "#1A1A1A"
    readonly property color mutedText: dark ? "#969696" : "#A6A6A6"

    // Editable areas.
    readonly property color fieldBackground: dark ? "#232323" : "#FBFAF6"
    readonly property color fieldBorder: dark ? "#1B3550" : "#9FC3E8"
    readonly property color focusBackground: dark ? "#14375A" : "#CCE8FF"
    readonly property color requiredBackground: "#FFE4C4"

    // Highlighted buttons (toolbar, history entry/create, dialog buttons).
    readonly property color accentBackground: dark ? "#27496D" : "#CFE2F3"
    readonly property color accentHover: dark ? "#2F5A85" : "#BBD6EE"
    readonly property color accentPressed: dark ? "#1B3550" : "#A9C9E8"
    readonly property color accentBorder: dark ? "#1B3550" : "#9FC3E8"
    readonly property color accentText: dark ? "white" : "#1A1A1A"
    readonly property color accentDisabledBackground: dark ? "#3A3A3A" : "#E4E3DF"
    readonly property color accentDisabledText: dark ? "#7A7A7A" : "#A6A6A6"

    // Selection and danger.
    readonly property color highlight: dark ? "#2A82DA" : "#3D7EBE"
    readonly property color highlightedText: dark ? "black" : "white"
    readonly property color alternateRowBackground: dark ? "#353535" : "#ECEAE3"
    readonly property color danger: "#E81123"
    readonly property color dangerPressed: "#A00A18"

    // Check boxes and label highlight.
    readonly property color checkFocusBackground: dark ? "#1E3A55" : "#D9ECFA"
    readonly property color checkedText: dark ? "#E89A9A" : "#800000"
    readonly property color labelHighlight: "#1C3A75"
}
