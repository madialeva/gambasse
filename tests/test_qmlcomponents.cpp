#include <QFontMetrics>
#include <QGuiApplication>
#include <QImage>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <Paths.h>
#include <ui/InterfaceSettings.h>

namespace gambasse {
namespace {

// Loads one library component inside the test harness (production Theme
// path) parented to a fresh exposed window.
struct Harness {
    QQuickWindow* window = nullptr;
    QObject* component = nullptr;
};

QQuickItem* asItem(QObject* object) {
    return qobject_cast<QQuickItem*>(object);
}

bool openHarness(QQmlApplicationEngine* engine, const char* file, const QString& theme,
                 Harness& out) {
    QQmlComponent harness(
        engine, QUrl::fromLocalFile(QStringLiteral(QML_FIXTURE_DIR "/ComponentHarness.qml")));
    QObject* root = harness.create();
    if (root == nullptr)
        return false;
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
    void textInput();
    void numberInput();
    void dateInput();
    void comboInput();
    void label();
    void checkBox();
    void fieldTextFitsTheBox();
    void group();
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

        // Focus shows the tint background.
        root->setProperty("checked", false);
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
// the field without the style's vertical padding eating it, whatever the
// interface font size of the desktop (checked from 9 to 12 points).
void TestQmlComponents::fieldTextFitsTheBox() {
    const QFont original = QGuiApplication::font();
    for (int points : {9, 10, 11, 12}) {
        QFont font = original;
        font.setPointSize(points);
        QGuiApplication::setFont(font);
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
            QVERIFY2(available >= line,
                     qPrintable(QStringLiteral("%1 at %2 pt: %3 px for a %4 px line")
                                    .arg(QLatin1String(file)).arg(points).arg(available).arg(line)));
            delete harness.window;
        }
    }
    QGuiApplication::setFont(original);
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

} // namespace gambasse

int main(int argc, char** argv) {
    QGuiApplication::setQuitOnLastWindowClosed(false);
    QGuiApplication app(argc, argv);
    gambasse::TestQmlComponents test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_qmlcomponents.moc"
