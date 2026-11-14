#include <QFontMetrics>
#include <QGuiApplication>
#include <QImage>
#include <QJSValue>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QRawFont>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>
#include <memory>

#include <Paths.h>
#include <VisibleTextChecks.h>
#include <ZoomTestSupport.h>
#include <ui/InterfaceSettings.h>

namespace gambasse {
namespace {

// Loads one library component inside the test harness (production Theme
// path) parented to a fresh exposed window. `shell` is the harness itself,
// whose `themeMode` switches the theme.
struct Harness {
    QQuickWindow* window = nullptr;
    QObject* component = nullptr;
    QObject* shell = nullptr;
};

QQuickItem* asItem(QObject* object) {
    return qobject_cast<QQuickItem*>(object);
}

bool openHarness(QQmlApplicationEngine* engine, const char* file, const QString& theme,
                 Harness& out) {
    // Up front, like the application: it installs the interface font.
    engine->singletonInstance<gambasse::InterfaceSettings*>(QStringLiteral("Gambasse"),
                                                            QStringLiteral("InterfaceSettings"));
    QQmlComponent harness(
        engine, QUrl::fromLocalFile(QStringLiteral(QML_FIXTURE_DIR "/ComponentHarness.qml")));
    QObject* root = harness.create();
    if (root == nullptr)
        return false;
    out.shell = root;
    root->setProperty("themeMode", theme);
    root->setProperty("componentFile",
                      QUrl::fromLocalFile(QStringLiteral(QML_COMPONENTS_DIR "/") +
                                          QString::fromLatin1(file))
                          .toString());
    auto* window = new QQuickWindow();
    window->resize(1008, 561);
    auto* rootItem = asItem(root);
    if (rootItem == nullptr)
        return false;
    // NOTE: setParentItem only joins the visual tree; the object tree join
    // below is what window-scoped findChildren() traverses.
    rootItem->setParentItem(window->contentItem());
    root->setParent(window);
    rootItem->setSize(QSizeF(window->width(), window->height()));
    out.window = window;
    QObject* loader = root->findChild<QObject*>(QStringLiteral("componentLoader"));
    if (loader == nullptr)
        return false;
    for (int i = 0; i < 100 && loader->property("item").isNull(); ++i)
        QTest::qWait(20);
    out.component = loader->property("item").value<QObject*>();
    return out.component != nullptr;
}

// Types through the real key path (validators apply, like pasting).
void typeText(QQuickWindow* window, const QString& text) {
    for (const QChar& ch : text)
        QTest::keyClick(window, ch.toLatin1());
}

// Collects visual-tree items by objectName. Delegate instances live in the
// visual tree only (their QObject parent is the delegate model), so a
// window-scoped findChildren() never sees them.
void collectVisualItems(QQuickItem* item, const QString& name, QList<QQuickItem*>& out) {
    if (item->objectName() == name)
        out.append(item);
    const QList<QQuickItem*> kids = item->childItems();
    for (QQuickItem* kid : kids)
        collectVisualItems(kid, name, out);
}

QQuickItem* visualItem(QQuickWindow* window, const QString& name) {
    QList<QQuickItem*> found;
    collectVisualItems(window->contentItem(), name, found);
    return found.isEmpty() ? nullptr : found.first();
}

// Opens the calendar of a GxDateInput with its button, like the user.
QObject* openCalendar(const Harness& harness) {
    auto* button = harness.component->findChild<QQuickItem*>(QStringLiteral("calendarButton"));
    if (button == nullptr)
        return nullptr;
    const QPointF centre =
        button->mapToScene(QPointF(button->width() / 2.0, button->height() / 2.0));
    QTest::mouseClick(harness.window, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
    QObject* popup = harness.component->findChild<QObject*>(QStringLiteral("calendarPopup"));
    if (popup == nullptr
        || !QTest::qWaitFor([popup] { return popup->property("opened").toBool(); }))
        return nullptr;
    return popup;
}

// The day names of the open calendar, from left to right.
QList<QQuickItem*> dayNames(QQuickWindow* window) {
    QList<QQuickItem*> names;
    collectVisualItems(window->contentItem(), QStringLiteral("dayName"), names);
    names.removeIf([](const QQuickItem* name) { return !name->isVisible(); });
    std::sort(names.begin(), names.end(), [](const QQuickItem* a, const QQuickItem* b) {
        return a->mapToScene(QPointF()).x() < b->mapToScene(QPointF()).x();
    });
    return names;
}

// Whether the row has laid out its seven day names: columns of the same width
// at a regular pitch. Until then they sit at their natural widths.
bool dayNamesAreLaidOut(QQuickWindow* window) {
    const QList<QQuickItem*> names = dayNames(window);
    if (names.size() != 7)
        return false;
    const qreal left = names.first()->mapToScene(QPointF()).x();
    const qreal width = names.first()->width();
    const qreal pitch = names.at(1)->mapToScene(QPointF()).x() - left;
    if (pitch < width - 0.5)
        return false;
    for (qsizetype i = 0; i < names.size(); ++i) {
        if (qAbs(names.at(i)->width() - width) > 0.5
            || qAbs(names.at(i)->mapToScene(QPointF()).x() - (left + i * pitch)) > 0.5)
            return false;
    }
    return true;
}

QStringList textsOf(const QList<QQuickItem*>& items) {
    QStringList texts;
    for (const QQuickItem* item : items)
        texts.append(item->property("text").toString());
    return texts;
}

// The day cell of `day` in the shown month (not the neighbouring months').
QQuickItem* dayCell(QQuickWindow* window, int day) {
    QList<QQuickItem*> cells;
    collectVisualItems(window->contentItem(), QStringLiteral("dayCell"), cells);
    for (QQuickItem* cell : cells) {
        if (cell->property("other").toBool())
            continue;
        const QObject* model = cell->property("model").value<QObject*>();
        if (model != nullptr && model->property("day").toInt() == day)
            return cell;
    }
    return nullptr;
}

// A colour of the Theme singleton of `engine`, in its current mode.
QColor themeColor(QQmlEngine* engine, const char* name) {
    QQmlComponent component(engine);
    component.setData("import QtQuick\nimport Gambasse\nQtObject { property color c: Theme."
                          + QByteArray(name) + " }",
                      QUrl());
    const std::unique_ptr<QObject> probe(component.create());
    return probe != nullptr ? probe->property("c").value<QColor>() : QColor();
}

// What the calendar shows in each interface language: the header of March
// 1990 and the day names from the first column, Monday.
struct CalendarTexts {
    QString march1990;
    QStringList days;
};

CalendarTexts calendarTexts(const QString& language) {
    if (language == QLatin1String("pt"))
        return {QStringLiteral("Março 1990"),
                {QStringLiteral("Seg"), QStringLiteral("Ter"), QStringLiteral("Qua"),
                 QStringLiteral("Qui"), QStringLiteral("Sex"), QStringLiteral("Sáb"),
                 QStringLiteral("Dom")}};
    if (language == QLatin1String("es"))
        return {QStringLiteral("Marzo 1990"),
                {QStringLiteral("Lun"), QStringLiteral("Mar"), QStringLiteral("Mié"),
                 QStringLiteral("Jue"), QStringLiteral("Vie"), QStringLiteral("Sáb"),
                 QStringLiteral("Dom")}};
    return {QStringLiteral("March 1990"),
            {QStringLiteral("Mon"), QStringLiteral("Tue"), QStringLiteral("Wed"),
             QStringLiteral("Thu"), QStringLiteral("Fri"), QStringLiteral("Sat"),
             QStringLiteral("Sun")}};
}

} // namespace

// Verifies the QML control library against the UxWidgets behaviors: labels,
// limits, validation, focus and required visuals, calendar, combo data,
// read-only checks, in both themes where visuals apply.
class TestQmlComponents : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;

private slots:
    void initTestCase();
    void popupsOfAZoomedWindowPickTheirSide();
    void symbolsAreDrawnWithShapes();
    void textInput();
    void numberInput();
    void dateInput();
    void calendarFollowsTheInterfaceLanguage_data();
    void calendarFollowsTheInterfaceLanguage();
    void comboInput();
    void comboPopupHighlightIsReadable();
    void comboPopupScrollsLongLists();
    void label();
    void checkBox();
    void fieldTextFitsTheBox();
    void group();
    void tabView();
};

void TestQmlComponents::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
}

void TestQmlComponents::textInput() {
    for (const QString& mode : {QStringLiteral("claro"), QStringLiteral("oscuro")}) {
        QQmlApplicationEngine engine;
        Harness harness;
        QVERIFY(openHarness(&engine, "GxTextInput.qml", mode, harness));
        QObject* root = harness.component;
        root->setProperty("labelText", QStringLiteral("Name:"));
        root->setProperty("width", 300);
        root->setProperty("height", 30);
        harness.window->show();
        QVERIFY(QTest::qWaitForWindowExposed(harness.window));

        root->setProperty("text", QStringLiteral("hello"));
        QCOMPARE(root->property("text").toString(), QStringLiteral("hello"));

        // Maximum length applies to typed text (set the limit first: some
        // styles truncate pre-existing longer text when it shrinks).
        auto* field = root->findChild<QQuickItem*>(QStringLiteral("field"));
        QVERIFY(field != nullptr);
        field->forceActiveFocus();
        QTRY_VERIFY(field->hasActiveFocus());
        root->setProperty("maxLength", 2);
        QCOMPARE(field->property("maximumLength").toInt(), 2);
        root->setProperty("text", QStringLiteral(""));
        typeText(harness.window, QStringLiteral("abc"));
        QCOMPARE(root->property("text").toString(), QStringLiteral("ab"));

        // Uppercase forcing.
        root->setProperty("maxLength", 32767);
        root->setProperty("uppercase", true);
        root->setProperty("text", QStringLiteral(""));
        typeText(harness.window, QStringLiteral("ab"));
        QCOMPARE(root->property("text").toString(), QStringLiteral("AB"));

        // Required-empty and focus backgrounds.
        auto* background = root->findChild<QQuickItem*>(QStringLiteral("fieldBackground"));
        QVERIFY(background != nullptr);
        root->setProperty("required", true);
        root->setProperty("text", QStringLiteral(""));
        // Focus first: focus highlight wins while focused.
        QCOMPARE(background->property("color").value<QColor>(),
                 mode == QStringLiteral("oscuro") ? QColor(0x14, 0x37, 0x5A)
                                                  : QColor(0xCC, 0xE8, 0xFF));
        field->setFocus(false);
        QTRY_COMPARE(background->property("color").value<QColor>(), QColor(0xFF, 0xE4, 0xC4));
        delete harness.window;
    }
}

void TestQmlComponents::numberInput() {
    QQmlApplicationEngine engine;
    Harness harness;
    QVERIFY(openHarness(&engine, "GxNumberInput.qml", QStringLiteral("claro"), harness));
    QObject* root = harness.component;
    root->setProperty("integerDigits", 3);
    root->setProperty("decimalDigits", 1);
    root->setProperty("width", 300);
    root->setProperty("height", 30);
    harness.window->show();
    QVERIFY(QTest::qWaitForWindowExposed(harness.window));
    auto* field = root->findChild<QQuickItem*>(QStringLiteral("field"));
    QVERIFY(field != nullptr);
    field->forceActiveFocus();
    QTRY_VERIFY(field->hasActiveFocus());

    // Validator blocks letters and extra decimals while typing.
    typeText(harness.window, QStringLiteral("12a3,456"));
    QCOMPARE(root->property("text").toString(), QStringLiteral("123,4"));

    // Thousands separator formats on focus out, strips on focus in.
    root->setProperty("text", QStringLiteral(""));
    root->setProperty("thousandsSeparator", true);
    root->setProperty("text", QStringLiteral("1234567"));
    field->setFocus(false);
    QTRY_COMPARE(root->property("text").toString(), QStringLiteral("1.234.567"));
    field->forceActiveFocus();
    QTRY_COMPARE(root->property("text").toString(), QStringLiteral("1234567"));
    delete harness.window;
}

void TestQmlComponents::dateInput() {
    QQmlApplicationEngine engine;
    Harness harness;
    QVERIFY(openHarness(&engine, "GxDateInput.qml", QStringLiteral("claro"), harness));
    QObject* root = harness.component;
    root->setProperty("width", 300);
    root->setProperty("height", 30);
    harness.window->show();
    QVERIFY(QTest::qWaitForWindowExposed(harness.window));

    // Format-only validation like UxDateField (nonsense dates pass format).
    root->setProperty("text", QStringLiteral("99/99/9999"));
    QCOMPARE(root->property("acceptableInput").toBool(), true);
    root->setProperty("text", QStringLiteral("abc"));
    QCOMPARE(root->property("acceptableInput").toBool(), false);
    root->setProperty("text", QStringLiteral(""));

    // Calendar button opens the month popup with 42 day cells.
    auto* button = root->findChild<QQuickItem*>(QStringLiteral("calendarButton"));
    QVERIFY(button != nullptr);
    const QPointF center =
        button->mapToScene(QPointF(button->width() / 2.0, button->height() / 2.0));
    QTest::mouseClick(harness.window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
    QObject* popup = root->findChild<QObject*>(QStringLiteral("calendarPopup"));
    QVERIFY(popup != nullptr);
    QTRY_VERIFY(popup->property("visible").toBool());

    // Pick day 15 of the shown month through a real MonthGrid delegate:
    // delegates live in the visual tree, so the search walks childItems()
    // from the window root (the popup reparents into the overlay).
    const int year = root->property("shownYear").toInt();
    const int month = root->property("shownMonth").toInt(); // 0-based
    QList<QQuickItem*> dayCells;
    collectVisualItems(harness.window->contentItem(), QStringLiteral("dayCell"), dayCells);
    QCOMPARE(dayCells.size(), 42);
    QQuickItem* target = nullptr;
    for (QQuickItem* cell : dayCells) {
        if (cell->property("other").toBool())
            continue;
        QObject* model = cell->property("model").value<QObject*>();
        if (model != nullptr && model->property("day").toInt() == 15) {
            target = cell;
            break;
        }
    }
    QVERIFY(target != nullptr);
    const QPointF dayCenter =
        target->mapToScene(QPointF(target->width() / 2.0, target->height() / 2.0));
    QTest::mouseClick(harness.window, Qt::LeftButton, Qt::NoModifier, dayCenter.toPoint());
    const QString expected = QStringLiteral("%1/%2/%3")
                                 .arg(15, 2, 10, QLatin1Char('0'))
                                 .arg(month + 1, 2, 10, QLatin1Char('0'))
                                 .arg(year);
    QCOMPARE(root->property("text").toString(), expected);
    QTRY_VERIFY(!popup->property("visible").toBool());
    delete harness.window;
}

// The calendar speaks the interface language whatever the system's. The
// system is simulated with QLocale::setDefault(), which is what Qt.locale()
// returns; pt_PT has whole words as short day names and starts on Sunday.
void TestQmlComponents::calendarFollowsTheInterfaceLanguage_data() {
    QTest::addColumn<QString>("language");
    QTest::addColumn<QString>("system");
    for (const char* language : {"pt", "es", "en"}) {
        for (const char* system : {"en_US", "pt_PT"})
            QTest::addRow("%s on %s", language, system)
                << QString::fromLatin1(language) << QString::fromLatin1(system);
    }
}

void TestQmlComponents::calendarFollowsTheInterfaceLanguage() {
    QFETCH(QString, language);
    QFETCH(QString, system);
    const QLocale systemBefore;
    QLocale::setDefault(QLocale(system));
    const auto restoreSystem = qScopeGuard([systemBefore] { QLocale::setDefault(systemBefore); });

    QQmlApplicationEngine engine;
    Harness harness;
    QVERIFY(openHarness(&engine, "GxDateInput.qml", QStringLiteral("claro"), harness));
    const auto closeWindow = qScopeGuard([&harness] { delete harness.window; });
    auto* settings = engine.singletonInstance<InterfaceSettings*>(
        QStringLiteral("Gambasse"), QStringLiteral("InterfaceSettings"));
    QVERIFY(settings != nullptr);
    const QString languageBefore = settings->language();
    const auto restoreLanguage =
        qScopeGuard([settings, languageBefore] { settings->setLanguage(languageBefore); });
    settings->setLanguage(language);

    QObject* root = harness.component;
    root->setProperty("width", 300);
    root->setProperty("height", 30);
    harness.window->show();
    QVERIFY(QTest::qWaitForWindowExposed(harness.window));
    root->setProperty("text", QStringLiteral("15/03/1990"));
    QObject* popup = openCalendar(harness);
    QVERIFY(popup != nullptr);
    auto* content = popup->property("contentItem").value<QQuickItem*>();
    auto* month = visualItem(harness.window, QStringLiteral("calendarMonth"));
    QVERIFY(content != nullptr && month != nullptr);

    // Every text whole and apart (Portuguese short names are whole words), once
    // the row has placed its names in their columns.
    QTRY_VERIFY(dayNamesAreLaidOut(harness.window));
    const QStringList cut = cutTexts(content);
    QVERIFY2(cut.isEmpty(), qPrintable(QStringLiteral("cut: ") + cut.join(QStringLiteral("; "))));

    // The month and the day names, the week starting on Monday.
    const CalendarTexts texts = calendarTexts(language);
    QCOMPARE(month->property("text").toString(), texts.march1990);
    const QList<QQuickItem*> names = dayNames(harness.window);
    QCOMPARE(textsOf(names), texts.days);

    // 1 March 1990 was a Thursday: its number sits under the fourth name.
    QQuickItem* first = dayCell(harness.window, 1);
    QVERIFY(first != nullptr);
    const qreal centre = first->mapToScene(QPointF(first->width() / 2.0, 0)).x();
    const qreal left = names.at(3)->mapToScene(QPointF()).x();
    QVERIFY2(centre > left && centre < left + names.at(3)->width(),
             qPrintable(QStringLiteral("day 1 at %1, %2 from %3 to %4")
                            .arg(centre)
                            .arg(names.at(3)->property("text").toString())
                            .arg(left)
                            .arg(left + names.at(3)->width())));

    // Every month: no text cut or overlapping, and only characters of the
    // embedded font (everyTextHasItsGlyphs does not see Qt's names).
    const QRawFont bold(QStringLiteral(":/fonts/NotoSans-Bold.ttf"), 13);
    QVERIFY(bold.isValid());
    QStringList problems;
    const auto checkGlyphs = [&bold, &problems](const QString& text) {
        for (const char32_t character : text.toUcs4()) {
            if (!bold.supportsCharacter(character))
                problems << QStringLiteral("%1: no glyph for U+%2")
                                .arg(text)
                                .arg(quint32(character), 4, 16, QLatin1Char('0'));
        }
    };
    for (const QString& name : textsOf(names))
        checkGlyphs(name);
    for (int shown = 0; shown < 12; ++shown) {
        root->setProperty("shownMonth", shown);
        QTest::qWait(30); // the grid lays out its new days
        const QString header = month->property("text").toString();
        checkGlyphs(header);
        for (const QString& cut : cutTexts(content))
            problems << QStringLiteral("%1: %2 cut").arg(header, cut);
    }
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join(QStringLiteral("; "))));
    root->setProperty("shownMonth", 2);
    QTRY_COMPARE(month->property("text").toString(), texts.march1990);

    // The text colour of the theme, readable on the calendar, in both themes.
    const auto luminance = [](const QColor& c) {
        return 0.2126 * c.redF() + 0.7152 * c.greenF() + 0.0722 * c.blueF();
    };
    for (const QString& mode : {QStringLiteral("claro"), QStringLiteral("oscuro")}) {
        harness.shell->setProperty("themeMode", mode);
        const QColor text = themeColor(&engine, "windowText");
        const QColor background = themeColor(&engine, "fieldBackground");
        QVERIFY(text.isValid() && background.isValid());
        QVERIFY2(qAbs(luminance(text) - luminance(background)) > 0.4,
                 qPrintable(QStringLiteral("%1: %2 on %3").arg(mode, text.name(), background.name())));
        QTRY_COMPARE(month->property("color").value<QColor>(), text);
        for (const QQuickItem* name : names)
            QCOMPARE(name->property("color").value<QColor>(), text);
    }
    harness.shell->setProperty("themeMode", QStringLiteral("claro"));

    // A language change applies at once, on the same month.
    const QString other = language == QLatin1String("pt")   ? QStringLiteral("es")
                          : language == QLatin1String("es") ? QStringLiteral("en")
                                                            : QStringLiteral("pt");
    settings->setLanguage(other);
    QTRY_COMPARE(month->property("text").toString(), calendarTexts(other).march1990);
    QCOMPARE(textsOf(dayNames(harness.window)), calendarTexts(other).days);
    QCOMPARE(root->property("shownMonth").toInt(), 2);
    QCOMPARE(root->property("shownYear").toInt(), 1990);
    QVERIFY(popup->property("opened").toBool());

    // Picking a day writes dd/MM/yyyy whatever the languages.
    QQuickItem* target = dayCell(harness.window, 20);
    QVERIFY(target != nullptr);
    const QPointF targetCentre =
        target->mapToScene(QPointF(target->width() / 2.0, target->height() / 2.0));
    QTest::mouseClick(harness.window, Qt::LeftButton, Qt::NoModifier, targetCentre.toPoint());
    QCOMPARE(root->property("text").toString(), QStringLiteral("20/03/1990"));
    QTRY_VERIFY(!popup->property("visible").toBool());
}

void TestQmlComponents::comboInput() {
    QQmlApplicationEngine engine;
    Harness harness;
    QVERIFY(openHarness(&engine, "GxComboInput.qml", QStringLiteral("claro"), harness));
    QObject* root = harness.component;
    root->setProperty("width", 300);
    root->setProperty("height", 30);
    harness.window->show();
    QVERIFY(QTest::qWaitForWindowExposed(harness.window));

    QVERIFY(QMetaObject::invokeMethod(root, "addItem", Q_ARG(QVariant, QStringLiteral("None")),
                                     Q_ARG(QVariant, 0)));
    QVERIFY(QMetaObject::invokeMethod(root, "addItem", Q_ARG(QVariant, QStringLiteral("Stable")),
                                     Q_ARG(QVariant, 1)));
    QVERIFY(QMetaObject::invokeMethod(root, "addItem",
                                     Q_ARG(QVariant, QStringLiteral("Dwelling")),
                                     Q_ARG(QVariant, 2)));
    root->setProperty("currentIndex", 2);
    QVariant data;
    QVERIFY(QMetaObject::invokeMethod(root, "currentData", Q_RETURN_ARG(QVariant, data)));
    QCOMPARE(data.toInt(), 2);
    QVariant found;
    QVERIFY(QMetaObject::invokeMethod(root, "findData", Q_RETURN_ARG(QVariant, found),
                                     Q_ARG(QVariant, 1)));
    QCOMPARE(found.toInt(), 1);

    // Editable mode accepts custom text, readable through editText (like
    // QComboBox::currentText(), which always reflects the line edit).
    root->setProperty("editable", true);
    QObject* editor = root->property("editor").value<QObject*>();
    QVERIFY(editor != nullptr);
    auto* editorItem = qobject_cast<QQuickItem*>(editor);
    QVERIFY(editorItem != nullptr);
    editorItem->forceActiveFocus();
    QTRY_VERIFY(editorItem->hasActiveFocus());
    QTest::keyClick(harness.window, Qt::Key_A, Qt::ControlModifier);
    typeText(harness.window, QStringLiteral("xyz"));
    QCOMPARE(root->property("editText").toString(), QStringLiteral("xyz"));
    delete harness.window;
}

void TestQmlComponents::comboPopupHighlightIsReadable() {
    // The entry under the cursor must keep a readable text in both themes:
    // the Basic delegate painted it white on near white in the light theme.
    const auto luminance = [](const QColor& c) {
        return 0.2126 * c.redF() + 0.7152 * c.greenF() + 0.0722 * c.blueF();
    };
    for (const QString& mode : {QStringLiteral("claro"), QStringLiteral("oscuro")}) {
        QQmlApplicationEngine engine;
        Harness harness;
        QVERIFY(openHarness(&engine, "GxComboInput.qml", mode, harness));
        QObject* root = harness.component;
        root->setProperty("width", 300);
        root->setProperty("height", 30);
        harness.window->show();
        QVERIFY(QTest::qWaitForWindowExposed(harness.window));
        for (const char* text : {"None", "Stable", "Dwelling"})
            QVERIFY(QMetaObject::invokeMethod(root, "addItem",
                                             Q_ARG(QVariant, QString::fromLatin1(text)),
                                             Q_ARG(QVariant, 0)));
        root->setProperty("currentIndex", 0);

        auto* combo = root->findChild<QQuickItem*>(QStringLiteral("comboBox"));
        QVERIFY(combo != nullptr);
        QObject* popup = combo->property("popup").value<QObject*>();
        QVERIFY(popup != nullptr);
        combo->forceActiveFocus();
        QVERIFY(QMetaObject::invokeMethod(popup, "open"));
        QTRY_VERIFY(popup->property("opened").toBool());
        // Move the highlight to the second entry with the keyboard.
        QTest::keyClick(harness.window, Qt::Key_Down);
        QTRY_COMPARE(combo->property("highlightedIndex").toInt(), 1);

        QList<QQuickItem*> entries;
        collectVisualItems(harness.window->contentItem(), QStringLiteral("comboEntry"), entries);
        QCOMPARE(entries.size(), 3);
        int highlighted = 0;
        for (QQuickItem* entry : entries) {
            auto* text = entry->findChild<QQuickItem*>(QStringLiteral("comboEntryText"));
            auto* background = entry->findChild<QQuickItem*>(QStringLiteral("comboEntryBackground"));
            QVERIFY(text != nullptr && background != nullptr);
            const QColor fg = text->property("color").value<QColor>();
            const QColor bg = background->property("color").value<QColor>();
            QVERIFY2(qAbs(luminance(fg) - luminance(bg)) > 0.4,
                     qPrintable(QStringLiteral("%1: %2 on %3")
                                    .arg(mode, fg.name(), bg.name())));
            if (entry->property("highlighted").toBool())
                ++highlighted;
        }
        QCOMPARE(highlighted, 1);
        delete harness.window;
    }
}

void TestQmlComponents::comboPopupScrollsLongLists() {
    // A long list (the 153 active ingredients) shows about ten entries and a
    // visible scroll bar; a short one shows every entry and no bar.
    for (const QString& mode : {QStringLiteral("claro"), QStringLiteral("oscuro")}) {
        for (int count : {40, 3}) {
            QQmlApplicationEngine engine;
            Harness harness;
            QVERIFY(openHarness(&engine, "GxComboInput.qml", mode, harness));
            QObject* root = harness.component;
            root->setProperty("width", 300);
            root->setProperty("height", 30);
            harness.window->show();
            QVERIFY(QTest::qWaitForWindowExposed(harness.window));
            for (int i = 0; i < count; ++i)
                QVERIFY(QMetaObject::invokeMethod(root, "addItem",
                                                 Q_ARG(QVariant, QStringLiteral("Entry %1").arg(i)),
                                                 Q_ARG(QVariant, i)));
            root->setProperty("currentIndex", 0);
            auto* combo = root->findChild<QQuickItem*>(QStringLiteral("comboBox"));
            QVERIFY(combo != nullptr);
            QObject* popup = combo->property("popup").value<QObject*>();
            QVERIFY(QMetaObject::invokeMethod(popup, "open"));
            QTRY_VERIFY(popup->property("opened").toBool());

            QList<QQuickItem*> found;
            collectVisualItems(harness.window->contentItem(), QStringLiteral("comboPopupList"), found);
            QCOMPARE(found.size(), 1);
            QQuickItem* list = found.first();
            found.clear();
            collectVisualItems(harness.window->contentItem(), QStringLiteral("comboScrollBar"), found);
            QCOMPARE(found.size(), 1);
            QQuickItem* bar = found.first();
            QTRY_VERIFY(list->property("contentHeight").toReal() > 0);
            const qreal entry = list->property("entryHeight").toReal();
            QVERIFY(entry > 0);
            if (count > 10) {
                // Ten entries tall, the rest reached through a visible bar.
                QCOMPARE(qRound(list->height()), qRound(entry * 10));
                QVERIFY(list->property("contentHeight").toReal() > list->height());
                QTRY_VERIFY(bar->isVisible());
                QVERIFY(bar->property("size").toReal() < 1.0);
                QVERIFY(bar->width() > 0);
                auto* handle = bar->findChild<QQuickItem*>(QStringLiteral("comboScrollHandle"));
                QVERIFY(handle != nullptr);
                QVERIFY(handle->property("color").value<QColor>()
                        != list->parentItem()->property("color").value<QColor>());
            } else {
                QCOMPARE(qRound(list->height()), qRound(list->property("contentHeight").toReal()));
                QVERIFY(!bar->isVisible() || bar->property("policy").toInt() == 0);
            }
            delete harness.window;
        }
    }
}

void TestQmlComponents::label() {
    QQmlApplicationEngine engine;
    Harness harness;
    QVERIFY(openHarness(&engine, "GxLabel.qml", QStringLiteral("claro"), harness));
    QObject* root = harness.component;
    root->setProperty("width", 200);
    root->setProperty("height", 40);
    harness.window->show();
    QVERIFY(QTest::qWaitForWindowExposed(harness.window));

    root->setProperty("text", QStringLiteral("Hello"));
    QCOMPARE(root->property("text").toString(), QStringLiteral("Hello"));

    // Multiline wraps long text.
    root->setProperty("multiline", true);
    root->setProperty("text", QStringLiteral("A fairly long label text that must wrap lines"));
    QTRY_VERIFY(root->property("lineCount").toInt() > 1);

    // Hover highlight underlines, recolors and clicks.
    root->setProperty("multiline", false);
    root->setProperty("highlight", true);
    root->setProperty("width", 120);
    root->setProperty("height", 24);
    root->setProperty("text", QStringLiteral("Click me"));
    auto* label = root->findChild<QQuickItem*>(QStringLiteral("labelText"));
    QVERIFY(label != nullptr);
    QSignalSpy spy(root, SIGNAL(clicked()));
    QTest::mouseMove(harness.window, QPoint(60, 12));
    QTRY_VERIFY(label->property("font").value<QFont>().underline());
    QCOMPARE(label->property("color").value<QColor>(), QColor(0x1C, 0x3A, 0x75));
    QTest::mouseClick(harness.window, Qt::LeftButton, Qt::NoModifier, QPoint(60, 12));
    QCOMPARE(spy.count(), 1);

    // Fill with dots pads short text to the control width.
    root->setProperty("highlight", false);
    root->setProperty("fillWithDots", true);
    root->setProperty("width", 300);
    root->setProperty("text", QStringLiteral("AB"));
    QTRY_VERIFY(root->property("text").toString().endsWith(QLatin1Char('.')));
    QVERIFY(root->property("text").toString().size() > 2);

    // Optional image loads from resources.
    auto* icon = root->findChild<QQuickItem*>(QStringLiteral("labelIcon"));
    QVERIFY(icon != nullptr);
    root->setProperty("fillWithDots", false);
    root->setProperty("imageSource", QStringLiteral("qrc:/img/logo.svg"));
    QTRY_COMPARE(icon->property("status").toInt(), 1); // QQuickImage::Ready
    delete harness.window;
}

void TestQmlComponents::checkBox() {
    for (const QString& mode : {QStringLiteral("claro"), QStringLiteral("oscuro")}) {
        QQmlApplicationEngine engine;
        Harness harness;
        QVERIFY(openHarness(&engine, "GxCheck.qml", mode, harness));
        QObject* root = harness.component;
        root->setProperty("text", QStringLiteral("Mosquito net"));
        root->setProperty("width", 300);
        root->setProperty("height", 30);
        harness.window->show();
        QVERIFY(QTest::qWaitForWindowExposed(harness.window));

        // Programmatic check paints the checked text color.
        root->setProperty("checked", true);
        QCOMPARE(root->property("checked").toBool(), true);
        auto* label = root->findChild<QQuickItem*>(QStringLiteral("checkText"));
        QVERIFY(label != nullptr);
        const QColor checkedExpected =
            mode == QStringLiteral("oscuro") ? QColor(0xE8, 0x9A, 0x9A) : QColor(0x80, 0, 0);
        QCOMPARE(label->property("color").value<QColor>(), checkedExpected);
        auto* tick = root->findChild<QQuickItem*>(QStringLiteral("checkTick"));
        QVERIFY(tick != nullptr);
        QVERIFY(tick->isVisible());

        // Focus shows the tint background.
        root->setProperty("checked", false);
        QVERIFY(!tick->isVisible());
        auto* check = asItem(root);
        QVERIFY(check != nullptr);
        check->forceActiveFocus();
        QTRY_VERIFY(check->hasActiveFocus());
        auto* background = root->findChild<QQuickItem*>(QStringLiteral("checkBackground"));
        QVERIFY(background != nullptr);
        QVERIFY(background->isVisible());
        const QColor tintExpected =
            mode == QStringLiteral("oscuro") ? QColor(0x1E, 0x3A, 0x55) : QColor(0xD9, 0xEC, 0xFA);
        QCOMPARE(background->property("color").value<QColor>(), tintExpected);

        // Read-only ignores user toggles but keeps programmatic control.
        root->setProperty("readOnly", true);
        QTest::mouseClick(harness.window, Qt::LeftButton, Qt::NoModifier, QPoint(10, 10));
        QCOMPARE(root->property("checked").toBool(), false);
        QTest::keyClick(harness.window, Qt::Key_Space);
        QCOMPARE(root->property("checked").toBool(), false);
        root->setProperty("checked", true);
        QCOMPARE(root->property("checked").toBool(), true);
        delete harness.window;
    }
}


// The history forms give the entry fields 21 px: the text line must fit in
// the field without the style's vertical padding eating it.
void TestQmlComponents::fieldTextFitsTheBox() {
    for (const char* file : {"GxTextInput.qml", "GxNumberInput.qml", "GxDateInput.qml"}) {
        QQmlApplicationEngine engine;
        Harness harness;
        QVERIFY(openHarness(&engine, file, QStringLiteral("claro"), harness));
        QObject* root = harness.component;
        root->setProperty("labelText", QStringLiteral("Label:"));
        root->setProperty("width", 200);
        root->setProperty("height", 21);
        harness.window->show();
        QVERIFY(QTest::qWaitForWindowExposed(harness.window));
        auto* field = root->findChild<QQuickItem*>(QStringLiteral("field"));
        QVERIFY(field != nullptr);
        QTRY_COMPARE(field->height(), 21.0);
        const qreal available = field->height() - field->property("topPadding").toReal()
                                - field->property("bottomPadding").toReal();
        const int line = QFontMetrics(field->property("font").value<QFont>()).height();
        QVERIFY2(available >= line, qPrintable(QStringLiteral("%1: %2 px for a %3 px line")
                                                   .arg(QLatin1String(file))
                                                   .arg(available)
                                                   .arg(line)));
        delete harness.window;
    }
}


// Sections: a rounded card with its own background below the title; a nested
// group keeps only the border, and `body` is the padded area for a layout.
void TestQmlComponents::group() {
    for (const QString& mode : {QStringLiteral("claro"), QStringLiteral("oscuro")}) {
        QQmlApplicationEngine engine;
        Harness harness;
        QVERIFY(openHarness(&engine, "GxGroup.qml", mode, harness));
        QObject* root = harness.component;
        root->setProperty("title", QStringLiteral("Section"));
        root->setProperty("width", 300);
        root->setProperty("height", 120);
        harness.window->show();
        QVERIFY(QTest::qWaitForWindowExposed(harness.window));
        auto* frame = root->findChild<QQuickItem*>(QStringLiteral("groupFrame"));
        auto* body = root->findChild<QQuickItem*>(QStringLiteral("groupBody"));
        QVERIFY(frame != nullptr);
        QVERIFY(body != nullptr);

        // Card from halfway down the 18 px title strip, rounded, with the
        // section colour; the title badge sits centred on its top border.
        QCOMPARE(frame->y(), 8.0);
        auto* badge = root->findChild<QQuickItem*>(QStringLiteral("groupTitleBadge"));
        QVERIFY(badge != nullptr);
        QCOMPARE(badge->y() + badge->height() / 2, frame->y());
        QCOMPARE(badge->height(), 16.0);
        QCOMPARE(badge->property("radius").toReal(), 8.0);
        QVERIFY(badge->property("color").value<QColor>() != frame->property("color").value<QColor>());
        QCOMPARE(frame->property("radius").toReal(), 6.0);
        const QColor card = frame->property("color").value<QColor>();
        QCOMPARE(card.alpha(), 255);
        const QImage shot = harness.window->grabWindow();
        const QPointF inside = frame->mapToScene(QPointF(frame->width() / 2, frame->height() / 2));
        QCOMPARE(shot.pixelColor(inside.toPoint()), card);
        QVERIFY(card != shot.pixelColor(0, harness.window->height() - 1)); // not the window grey

        // Body: below the badge, with 8 px of padding on every side.
        QCOMPARE(QRectF(body->x(), body->y(), body->width(), body->height()),
                 QRectF(8, 16 + 8, 300 - 16, 120 - 16 - 16));

        // Nested: border only, a smaller radius.
        root->setProperty("nested", true);
        QCOMPARE(frame->property("color").value<QColor>().alpha(), 0);
        QCOMPARE(frame->property("radius").toReal(), 4.0);
        delete harness.window;
    }
}

void TestQmlComponents::tabView() {
    for (const QString& mode : {QStringLiteral("claro"), QStringLiteral("oscuro")}) {
        QQmlApplicationEngine engine;
        Harness harness;
        QVERIFY(openHarness(&engine, "GxTabView.qml", mode, harness));
        QObject* root = harness.component;
        root->setProperty("width", 400);
        root->setProperty("height", 200);
        root->setProperty("titles", QStringList{QStringLiteral("First"), QStringLiteral("Second"),
                                                QStringLiteral("Third")});
        harness.window->show();
        QVERIFY(QTest::qWaitForWindowExposed(harness.window));

        // One page per tab, stacked: only the current one is visible.
        auto* stack = root->findChild<QQuickItem*>(QStringLiteral("tabStack"));
        QVERIFY(stack != nullptr);
        QQmlComponent pageComponent(&engine);
        pageComponent.setData("import QtQuick\nItem {}", QUrl());
        QList<QQuickItem*> pages;
        for (int i = 0; i < 3; ++i) {
            auto* page = qobject_cast<QQuickItem*>(pageComponent.create());
            QVERIFY(page != nullptr);
            page->setParent(stack);
            page->setParentItem(stack);
            pages.append(page);
        }

        QList<QQuickItem*> tabs;
        for (int i = 0; i < 3; ++i) {
            QList<QQuickItem*> found;
            collectVisualItems(asItem(root), QStringLiteral("tab%1").arg(i), found);
            QCOMPARE(found.size(), 1);
            tabs.append(found.first());
        }
        QCOMPARE(tabs[0]->property("text").toString(), QStringLiteral("First"));
        QCOMPARE(tabs[0]->property("checked").toBool(), true);
        QTRY_VERIFY(pages[0]->isVisible());
        QVERIFY(!pages[1]->isVisible());

        // Clicking a tab selects it and shows its page; neighbours share
        // their border line and each tab is as wide as its caption.
        const QPointF centre = tabs[1]->mapToScene(QPointF(tabs[1]->width() / 2, 12));
        QTest::mouseClick(harness.window, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
        QTRY_COMPARE(root->property("currentIndex").toInt(), 1);
        QCOMPARE(tabs[1]->property("checked").toBool(), true);
        QCOMPARE(tabs[0]->property("checked").toBool(), false);
        QTRY_VERIFY(pages[1]->isVisible());
        QVERIFY(!pages[0]->isVisible());
        QCOMPARE(tabs[1]->x(), tabs[0]->x() + tabs[0]->width() - 1);
        QCOMPARE(tabs[0]->height(), 25.0);

        // A tighter padding narrows every tab by the same amount.
        const qreal wide = tabs[2]->width();
        root->setProperty("tabPadding", 5);
        QTRY_COMPARE(tabs[2]->width(), wide - 16);
        delete harness.window;
    }
}


// In a zoomed child window, a combo list and a calendar are scaled like the
// window and stay against their field: below it when they fit, above it
// otherwise (Qt's own flip misplaces a scaled popup).
void TestQmlComponents::popupsOfAZoomedWindowPickTheirSide() {
    QQmlApplicationEngine engine;
    auto* settings = engine.singletonInstance<InterfaceSettings*>(
        QStringLiteral("Gambasse"), QStringLiteral("InterfaceSettings"));
    QVERIFY(settings != nullptr);
    const auto restore = qScopeGuard([settings] { settings->setLetterSize(13); });
    QQmlComponent component(
        &engine, QUrl::fromLocalFile(QStringLiteral(QML_FIXTURE_DIR "/ZoomedPopups.qml")));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root != nullptr, qPrintable(component.errorString()));
    auto* window = qobject_cast<QQuickWindow*>(root.get());
    QVERIFY(window != nullptr);
    zoomtest::zoomOf(window)->setAvailableArea(zoomtest::wideArea);
    settings->setLetterSize(17);
    const qreal factor = 17.0 / 13.0;
    QVERIFY(QMetaObject::invokeMethod(window, "showPlaced"));
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QTRY_COMPARE(window->contentItem()->size(), QSizeF(zoomtest::zoomOf(window)->windowSize()));

    for (const auto& [name, below] : {std::pair{"topCombo", true}, std::pair{"lowCombo", false}}) {
        auto* input = window->findChild<QQuickItem*>(QLatin1String(name));
        QVERIFY(input != nullptr);
        auto* combo = input->findChild<QQuickItem*>(QStringLiteral("comboBox"));
        auto* popup = combo->property("popup").value<QObject*>();
        QVERIFY(QMetaObject::invokeMethod(popup, "open"));
        QTRY_VERIFY(popup->property("opened").toBool());
        const QString problems = zoomtest::popupProblems(popup, combo, combo->width(), factor);
        QVERIFY2(problems.isEmpty(), qPrintable(QStringLiteral("%1: %2").arg(name, problems)));
        QCOMPARE(zoomtest::isBelow(popup, combo), below);
        QVERIFY(QMetaObject::invokeMethod(popup, "close"));
        QTRY_VERIFY(!popup->property("visible").toBool());
    }
    for (const auto& [name, below] : {std::pair{"topDate", true}, std::pair{"lowDate", false}}) {
        auto* date = window->findChild<QQuickItem*>(QLatin1String(name));
        QVERIFY(date != nullptr);
        auto* calendar = date->findChild<QObject*>(QStringLiteral("calendarPopup"));
        QVERIFY(QMetaObject::invokeMethod(date, "openCalendar"));
        QTRY_VERIFY(calendar->property("opened").toBool());
        const QString problems = zoomtest::popupProblems(
            calendar, date, calendar->property("width").toReal(), factor);
        QVERIFY2(problems.isEmpty(), qPrintable(QStringLiteral("%1: %2").arg(name, problems)));
        QCOMPARE(zoomtest::isBelow(calendar, date), below);
        QVERIFY(QMetaObject::invokeMethod(calendar, "close"));
        QTRY_VERIFY(!calendar->property("visible").toBool());
    }
}


// The combo arrow and the calendar icon are shapes, not Canvas bitmaps, so a
// zoomed window draws them sharp. At the base size they keep the measures of
// the Widgets glyphs and the colour of the theme, and follow a theme switch.
void TestQmlComponents::symbolsAreDrawnWithShapes() {
    QQmlApplicationEngine engine;
    Harness combo;
    QVERIFY(openHarness(&engine, "GxComboInput.qml", QStringLiteral("claro"), combo));
    combo.component->setProperty("width", 200);
    combo.component->setProperty("height", 22);
    combo.window->show();
    QVERIFY(QTest::qWaitForWindowExposed(combo.window));
    auto* arrow = visualItem(combo.window, QStringLiteral("comboArrow"));
    QVERIFY(arrow != nullptr);
    QCOMPARE(QSizeF(arrow->width(), arrow->height()), QSizeF(7, 4));
    QList<qreal> rows;
    for (QQuickItem* row : arrow->childItems())
        rows.append(row->width());
    QCOMPARE(rows, (QList<qreal>{7, 5, 3, 1}));

    Harness date;
    QVERIFY(openHarness(&engine, "GxDateInput.qml", QStringLiteral("claro"), date));
    date.component->setProperty("width", 200);
    date.component->setProperty("height", 22);
    date.window->show();
    QVERIFY(QTest::qWaitForWindowExposed(date.window));
    auto* icon = visualItem(date.window, QStringLiteral("calendarIcon"));
    QVERIFY(icon != nullptr);
    QCOMPARE(QSizeF(icon->width(), icon->height()), QSizeF(16, 16));
    auto* page = visualItem(date.window, QStringLiteral("calendarPage"));
    QVERIFY(page != nullptr);

    const QList<std::tuple<QString, QColor, QColor>> themes = {
        {QStringLiteral("claro"), QColor(QStringLiteral("#6B6B6B")), QColor(QStringLiteral("#1A1A1A"))},
        {QStringLiteral("oscuro"), QColor(QStringLiteral("#BABABA")), QColor(Qt::white)},
    };
    for (const auto& [mode, arrowColour, iconColour] : themes) {
        combo.shell->setProperty("themeMode", mode);
        date.shell->setProperty("themeMode", mode);
        for (QQuickItem* row : arrow->childItems())
            QCOMPARE(row->property("color").value<QColor>(), arrowColour);
        QCOMPARE(page->property("border").value<QObject*>()->property("color").value<QColor>(),
                 iconColour);
        // Painted where it is laid out: the widest row of the arrow, whole
        // pixels on the base size.
        const QPoint spot = arrow->mapToScene(QPointF(3.5, 0.5)).toPoint();
        QTRY_COMPARE(combo.window->grabWindow().pixelColor(spot), arrowColour);
    }
}

} // namespace gambasse

int main(int argc, char** argv) {
    QGuiApplication::setQuitOnLastWindowClosed(false);
    QGuiApplication app(argc, argv);
    // The application's style (main.cpp): Qt's default on Linux is Fusion,
    // whose paddings would measure the controls differently.
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    gambasse::TestQmlComponents test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_qmlcomponents.moc"
