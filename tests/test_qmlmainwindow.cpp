#include <QApplication>
#include <QQmlApplicationEngine>
#include <QImage>
#include <QSet>
#include <functional>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include <Paths.h>
#include <data/common/Database.h>
#include <data/model/Patient.h>
#include <logic/PatientService.h>
#include <ui/InterfaceSettings.h>
#include <ui/PatientController.h>

namespace gambasse {
namespace {

// Direct QML child, addressed by the objectName declared in the .qml file.
// The QML object tree hangs from the window object, not from its content item.
QQuickItem* child(QObject* parent, const QString& name) {
    return parent->findChild<QQuickItem*>(name);
}

// Visual descendants (Repeater instantiates them without QObject parentage),
// so the item tree is the reliable place to look for them.
int countVisualChildren(QQuickItem* parent, const QString& objectName) {
    int found = 0;
    const auto visit = [&](QQuickItem* item, auto&& self) -> void {
        for (QQuickItem* childItem : item->childItems()) {
            if (childItem->objectName() == objectName)
                ++found;
            self(childItem, self);
        }
    };
    if (parent != nullptr)
        visit(parent, visit);
    return found;
}

QPoint centerOf(QQuickItem* item) {
    return item->mapToScene(QPointF(item->width() / 2.0, item->height() / 2.0)).toPoint();
}

// Colours read from the Theme singleton, so the assertions follow the theme.
QColor themeColor(const char* name) {
    QVariant value;
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nimport Gambasse\nQtObject { property color c: Theme."
                          + QByteArray(name) + " }",
                      QUrl());
    QObject* probe = component.create();
    value = probe != nullptr ? probe->property("c") : QVariant();
    delete probe;
    return value.value<QColor>();
}

QColor ThemeHighlight() { return themeColor("highlight"); }
QColor fieldColor(int row) {
    const QColor base = themeColor("fieldBackground");
    return row % 2 ? themeColor("alternateRowBackground") : base;
}

// A tinted (or failed) icon shows up as a flat block: the real flags always
// carry several saturated colours, so the crop is checked for that.
QSet<QRgb> distinctColors(const QImage& shot, QPoint topLeft, QSize size) {
    QSet<QRgb> colors;
    const QImage crop = shot.copy(QRect(topLeft, size));
    for (int y = 0; y < crop.height(); y += 2) {
        for (int x = 0; x < crop.width(); x += 2)
            colors.insert(crop.pixel(x, y));
        if (colors.size() > 32)
            break;
    }
    return colors;
}

QColor pixelAt(QQuickWindow* window, QPoint topLeft, int row) {
    const QImage shot = window->grabWindow();
    return shot.pixelColor(topLeft + QPoint(20, row * 24 + 12));
}

void clickIn(QQuickWindow* window, QQuickItem* item, const QPointF& offset) {
    const QPoint pos = item->mapToScene(offset).toPoint();
    QTest::mouseMove(window, pos);
    QTest::qWait(20);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, pos);
}

void clickIn(QQuickWindow* window, QQuickItem* item) {
    const QPoint pos = centerOf(item);
    QTest::mouseMove(window, pos); // the item under the cursor must be hovered
    QTest::qWait(20);              // and the hover must be delivered first
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, pos);
}

} // namespace

// Verifies the QML patient screen wired to a real PatientController: the
// module loads, the grid mirrors the model, the detail follows the selection
// and the toolbar buttons drive the edit session.
class TestQmlMainWindow : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    PatientService m_service;
    QQmlApplicationEngine* m_engine = nullptr;
    PatientController* m_controller = nullptr;
    InterfaceSettings* m_settings = nullptr;
    QQuickWindow* m_window = nullptr;
    QQuickItem* m_screen = nullptr;

    QQuickItem* item(const QString& name) const { return child(m_screen, name); }

    // Fresh shell wired exactly like main.cpp: module load plus injection of
    // the two C++ bridges.
    void openScreen();
    void cleanup();
    void seedPatients();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void gridFollowsTheModel();
    void detailFollowsTheSelection();
    void toolbarDrivesTheEditSession();
    void savingFromTheFormUpdatesTheGrid();
    void filterBoxesNarrowTheGrid();
    void savedPatientSurvivesReopening();
    void toolbarFollowsTheLanguage();
    void selectedRowIsPaintedHighlighted();
    void detailGroupsShareTheWidth();
    void languageFlagKeepsItsColours();
    void detailPanelFitsTheWindow();
    void detailFieldsFitTheirLine();
    void calendarPopupIsVisible();
    void bothGroupsAreAligned();
    void gridTakesFocusAndKeyboardMoves();
};

void TestQmlMainWindow::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));
}

void TestQmlMainWindow::init() {
    QSqlQuery erase(Database::instance().connection());
    QVERIFY2(erase.exec(QStringLiteral("DELETE FROM b01_patient")), qPrintable(erase.lastError().text()));
}

void TestQmlMainWindow::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestQmlMainWindow::cleanup() {
    if (m_window != nullptr)
        m_window->close();
    delete m_controller;
    m_controller = nullptr;
    delete m_settings;
    m_settings = nullptr;
    delete m_engine;
    m_engine = nullptr;
    m_window = nullptr;
    m_screen = nullptr;
}

void TestQmlMainWindow::openScreen() {
    cleanup();
    m_controller = new PatientController();
    QVERIFY(m_controller->load());
    m_engine = new QQmlApplicationEngine();
    m_engine->loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("Root"));
    QVERIFY(!m_engine->rootObjects().isEmpty());
    m_window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    QVERIFY(m_window != nullptr);
    m_window->setProperty("patientController", QVariant::fromValue<QObject*>(m_controller));
    m_settings = new InterfaceSettings();
    m_window->setProperty("uiSettings", QVariant::fromValue<QObject*>(m_settings));
    m_screen = child(m_window, QStringLiteral("mainView"));
    QVERIFY(m_screen != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
}

void TestQmlMainWindow::seedPatients() {
    const QStringList names = {QStringLiteral("QMLW_ALFA"), QStringLiteral("QMLW_BETA"),
                               QStringLiteral("QMLW_GAMA")};
    for (const QString& name : names) {
        Patient p;
        p.name = name;
        p.sex = Patient::Sex::Home;
        p.birthDate = QDate(2001, 2, 3);
        p.ageRange = 20;
        QVERIFY(m_service.create(p) == PatientService::SaveResult::Saved);
    }
}

void TestQmlMainWindow::gridFollowsTheModel() {
    seedPatients();
    openScreen();

    // One filter box and one sortable header per column, in model order.
    QCOMPARE(countVisualChildren(m_screen, QStringLiteral("filterBox")),
             PatientsModel::ColumnCount);
    QCOMPARE(countVisualChildren(m_screen, QStringLiteral("headerCell")),
             PatientsModel::ColumnCount);

    // Three patients, and the delegate cells of the first row are on screen.
    QCOMPARE(m_controller->gridModel()->rowCount(), 3);
    QCOMPARE(countVisualChildren(item(QStringLiteral("gridTable")), QStringLiteral("gridCell")),
             3 * PatientsModel::ColumnCount);
    QVERIFY(!item(QStringLiteral("photoImage"))->property("source").toUrl().isEmpty());
}

void TestQmlMainWindow::detailFollowsTheSelection() {
    seedPatients();
    openScreen();

    auto* nameInput = item(QStringLiteral("nameInput"));
    QVERIFY(nameInput != nullptr);
    // View mode: the form is read-only and the grid drives the selection.
    QCOMPARE(nameInput->property("enabled").toBool(), false);
    QCOMPARE(m_controller->editing(), false);
    QCOMPARE(nameInput->property("text").toString(), m_controller->patientName());

    m_controller->selectRow(1); // driven from the controller side
    QCOMPARE(nameInput->property("text").toString(), QStringLiteral("QMLW_BETA"));
    QCOMPARE(item(QStringLiteral("addressInput"))->property("text").toString(),
             m_controller->address());
    QCOMPARE(item(QStringLiteral("ageInput"))->property("text").toString(),
             QString::number(m_controller->ageRange()));
    QCOMPARE(item(QStringLiteral("codeLabel"))->property("text").toString(),
             QStringLiteral("Code: %1").arg(m_controller->patientId()));
}

void TestQmlMainWindow::toolbarDrivesTheEditSession() {
    seedPatients();
    openScreen();

    auto* nameInput = item(QStringLiteral("nameInput"));
    auto* addButton = item(QStringLiteral("addButton"));
    auto* saveButton = item(QStringLiteral("saveButton"));
    auto* cancelButton = item(QStringLiteral("cancelButton"));
    QCOMPARE(addButton->property("enabled").toBool(), true);
    QCOMPARE(saveButton->property("enabled").toBool(), false);

    clickIn(m_window, addButton);
    QVERIFY(m_controller->editing());
    QCOMPARE(saveButton->property("enabled").toBool(), true);
    QCOMPARE(cancelButton->property("enabled").toBool(), true);
    QCOMPARE(addButton->property("enabled").toBool(), false);
    QCOMPARE(nameInput->property("enabled").toBool(), true);
    // The form is empty and offers the identifier that will be assigned.
    QVERIFY(nameInput->property("text").toString().isEmpty());
    QVERIFY(item(QStringLiteral("codeLabel"))->property("text").toString().contains(
        QString::number(m_controller->patientId())));

    // Cancelling returns to the selected patient without writing anything.
    clickIn(m_window, cancelButton);
    QVERIFY(!m_controller->editing());
    QCOMPARE(nameInput->property("enabled").toBool(), false);
    QCOMPARE(m_controller->patientName(), QStringLiteral("QMLW_ALFA"));
}

void TestQmlMainWindow::savingFromTheFormUpdatesTheGrid() {
    seedPatients();
    openScreen();

    clickIn(m_window, item(QStringLiteral("addButton")));
    QVERIFY(m_controller->editing());

    // Type into the form the way the user would, then save.
    auto* nameInput = item(QStringLiteral("nameInput"));
    nameInput->setProperty("text", QStringLiteral("QMLW_DELTA"));
    item(QStringLiteral("ageInput"))->setProperty("text", QStringLiteral("33"));
    item(QStringLiteral("addressInput"))->setProperty("text", QStringLiteral("rua nova"));
    clickIn(m_window, item(QStringLiteral("saveButton")));

    QVERIFY(!m_controller->editing());
    QCOMPARE(m_controller->gridModel()->rowCount(), 4);
    QCOMPARE(m_controller->patientName(), QStringLiteral("QMLW_DELTA"));
    QCOMPARE(m_controller->ageRange(), 33);
    QCOMPARE(m_controller->address(), QStringLiteral("rua nova"));
    // The saved patient becomes the selection and the form shows it again.
    QCOMPARE(item(QStringLiteral("nameInput"))->property("text").toString(),
             QStringLiteral("QMLW_DELTA"));
    QCOMPARE(item(QStringLiteral("nameInput"))->property("enabled").toBool(), false);

    // Deleting asks for confirmation and then removes the patient.
    clickIn(m_window, item(QStringLiteral("deleteButton")));
    auto* confirm = m_screen->findChild<QObject*>(QStringLiteral("deleteConfirm"));
    QVERIFY(confirm != nullptr);
    QCOMPARE(confirm->property("visible").toBool(), true);
    auto* yesButton = confirm->findChild<QQuickItem*>(QStringLiteral("dialogYesButton"));
    QVERIFY(yesButton != nullptr);
    QVERIFY(yesButton->isVisible());
    QTest::qWait(200); // let the modal overlay settle before aiming at the button
    clickIn(m_window, yesButton); // confirm, as the user would
    QTRY_COMPARE(m_controller->gridModel()->rowCount(), 3);
}

void TestQmlMainWindow::savedPatientSurvivesReopening() {
    seedPatients();
    openScreen();
    clickIn(m_window, item(QStringLiteral("addButton")));
    item(QStringLiteral("nameInput"))->setProperty("text", QStringLiteral("QMLW_DELTA"));
    clickIn(m_window, item(QStringLiteral("saveButton")));
    QCOMPARE(m_controller->gridModel()->rowCount(), 4);

    // Reopening: a brand new controller over the same database keeps the
    // patient, and the grid shows it sorted among the others.
    cleanup();
    PatientController reopened;
    QVERIFY(reopened.load());
    QCOMPARE(reopened.gridModel()->rowCount(), 4);
    QCOMPARE(reopened.patientName(), QStringLiteral("QMLW_ALFA"));
    reopened.selectRow(1);
    QCOMPARE(reopened.patientName(), QStringLiteral("QMLW_BETA"));
    reopened.selectRow(2);
    QCOMPARE(reopened.patientName(), QStringLiteral("QMLW_DELTA"));
}

void TestQmlMainWindow::toolbarFollowsTheLanguage() {
    seedPatients();
    openScreen();

    auto* addButton = item(QStringLiteral("addButton"));
    const QString english = addButton->property("text").toString();
    QCOMPARE(english, QStringLiteral("Add"));

    // Same wiring as main.cpp: the catalog plus engine.retranslate() repaints
    // the QML strings in place.
    InterfaceSettings settings;
    settings.setLanguage(QStringLiteral("es"));
    settings.applyLanguage(m_engine);
    QCOMPARE(addButton->property("text").toString(), QStringLiteral("A\u00f1adir"));

    settings.setLanguage(QStringLiteral("pt"));
    settings.applyLanguage(m_engine);
    QCOMPARE(addButton->property("text").toString(), QStringLiteral("Adicionar"));

    settings.setLanguage(QStringLiteral("en"));
    settings.applyLanguage(m_engine);
    QCOMPARE(addButton->property("text").toString(), english);
}

void TestQmlMainWindow::selectedRowIsPaintedHighlighted() {
    seedPatients();
    openScreen();

    // The grid selection is applied when the controller arrives, so the first
    // patient is highlighted without touching the mouse.
    auto* table = item(QStringLiteral("gridTable"));
    QVERIFY(table != nullptr);
    const QPoint topLeft = table->mapToScene(QPointF(0, 0)).toPoint();
    QCOMPARE(m_controller->currentRow(), 0);
    QCOMPARE(pixelAt(m_window, topLeft, 0), ThemeHighlight());

    m_controller->selectRow(1);
    QTRY_COMPARE(pixelAt(m_window, topLeft, 1), ThemeHighlight());
    QCOMPARE(pixelAt(m_window, topLeft, 0), fieldColor(0)); // the old row is back to normal

    // Clicking a row moves the highlight and the detail together.
    m_controller->cancelEdit();
    clickIn(m_window, table, QPointF(20, 2 * 24 + 12));
    QTRY_COMPARE(m_controller->currentRow(), 2);
    QCOMPARE(m_controller->patientName(), QStringLiteral("QMLW_GAMA"));
    QCOMPARE(pixelAt(m_window, topLeft, 2), ThemeHighlight());
}

void TestQmlMainWindow::detailGroupsShareTheWidth() {
    seedPatients();
    openScreen();

    auto* basic = item(QStringLiteral("basicDataGroup"));
    auto* address = item(QStringLiteral("addressGroup"));
    QVERIFY(basic != nullptr);
    QVERIFY(address != nullptr);
    // Half and half, whatever each group's implicit width is.
    const int total = basic->width() + address->width();
    QVERIFY(total > 0);
    QVERIFY2(qAbs(basic->width() - address->width()) <= 2,
             qPrintable(QStringLiteral("basic=%1 address=%2").arg(basic->width()).arg(address->width())));
}

void TestQmlMainWindow::languageFlagKeepsItsColours() {
    seedPatients();
    openScreen();

    auto* flag = m_window->findChild<QQuickItem*>(QStringLiteral("languageFlag"));
    QVERIFY(flag != nullptr);
    QVERIFY(flag->isVisible());

    for (const QString& code : {QStringLiteral("es"), QStringLiteral("pt"), QStringLiteral("en")}) {
        m_settings->setLanguage(code);
        QTRY_COMPARE(flag->property("source").toUrl().toString(),
                     QStringLiteral("qrc:/img/flag-%1.png").arg(code));
        QTRY_VERIFY(flag->property("status").toInt() == 1); // Image.Ready

        // Painted, not tinted: many colours in both themes.
        for (const QString& theme : {QStringLiteral("claro"), QStringLiteral("oscuro")}) {
            m_settings->setTheme(theme);
            QTest::qWait(50);
            const QSize flagSize = QSize(24, 16);
            const QPoint pos = flag->mapToScene(QPointF(0, 0)).toPoint();
            const QSet<QRgb> colors = distinctColors(m_window->grabWindow(), pos, flagSize);
            QVERIFY2(colors.size() >= 8,
                     qPrintable(QStringLiteral("theme=%1 code=%2 colors=%3")
                                    .arg(theme, code)
                                    .arg(colors.size())));
        }
    }
    m_settings->setTheme(QStringLiteral("claro"));

    // Same inside the language menu: the entry icons keep their colours too.
    auto* languageButton = m_window->findChild<QQuickItem*>(QStringLiteral("languageButton"));
    QVERIFY(languageButton != nullptr);
    clickIn(m_window, languageButton);
    QTest::qWait(200); // the modal overlay needs to settle before the click
    QQuickItem* spanishEntry = nullptr;
    const std::function<void(QQuickItem*)> visit = [&](QQuickItem* it) {
        if (spanishEntry == nullptr && it->property("text").toString() == QStringLiteral("Spanish"))
            spanishEntry = it;
        for (QQuickItem* c : it->childItems())
            visit(c);
    };
    visit(m_window->contentItem());
    QVERIFY(spanishEntry != nullptr);
    // The entry icon keeps its colours as well (no tint from the style).
    const QPoint entryPos = spanishEntry->mapToScene(QPointF(0, 0)).toPoint();
    const QSet<QRgb> menuColors =
        distinctColors(m_window->grabWindow(), entryPos + QPoint(25, 2), QSize(16, 14));
    QVERIFY2(menuColors.size() >= 6,
             qPrintable(QStringLiteral("menu flag colors=%1").arg(menuColors.size())));
}

void TestQmlMainWindow::detailPanelFitsTheWindow() {
    seedPatients();
    openScreen();

    auto* basic = item(QStringLiteral("basicDataGroup"));
    auto* address = item(QStringLiteral("addressGroup"));
    auto* age = item(QStringLiteral("ageInput"));
    auto* date = item(QStringLiteral("dateInput"));
    auto* sex = item(QStringLiteral("sexInput"));
    auto* code = item(QStringLiteral("codeLabel"));
    auto* siblings = item(QStringLiteral("siblingsInput"));

    // "Basic data" is three lines: code, name, and age + birth date + sex.
    QVERIFY(code != nullptr);
    // Scene coordinates: the last line is a nested layout, so the parents differ.
    const auto top = [](QQuickItem* it) { return it->mapToScene(QPointF(0, 0)).toPoint(); };
    const auto left = [](QQuickItem* it) { return it->mapToScene(QPointF(0, 0)).toPoint().x(); };
    QVERIFY(top(code).y() < top(item(QStringLiteral("nameInput"))).y());
    QVERIFY(top(item(QStringLiteral("nameInput"))).y() < top(age).y());
    QCOMPARE(top(age).y(), top(date).y());
    QCOMPARE(top(date).y(), top(sex).y());
    // The three controls of the last line share it instead of stacking.
    QVERIFY(left(age) < left(date));
    QVERIFY(left(date) < left(sex));

    // Nothing of the bottom panel may fall outside the window, not even at the
    // 960x540 minimum (the code label used to push the last field out).
    for (const QSize target : {QSize(1008, 561), QSize(960, 540)}) {
        m_window->resize(target);
        QTRY_COMPARE(m_window->size(), target);
        QTRY_COMPARE(m_screen->height(), target.height() - 32); // layout settled
        const auto bottom = [](QQuickItem* it) {
            return it->mapToScene(QPointF(0, it->height())).toPoint().y();
        };
        QVERIFY2(bottom(basic) <= m_window->height(),
                 qPrintable(QStringLiteral("%1x%2 basic=%3").arg(target.width())
                                .arg(target.height())
                                .arg(bottom(basic))));
        QVERIFY2(bottom(address) <= m_window->height(),
                 qPrintable(QStringLiteral("%1x%2 address=%3").arg(target.width())
                                .arg(target.height())
                                .arg(bottom(address))));
        QVERIFY2(bottom(age) <= bottom(basic), "the age field is not cut");
        QVERIFY2(bottom(siblings) <= bottom(address), "the siblings field is not cut");
        // The grid keeps the remaining space instead of pushing the panel out.
        QVERIFY(item(QStringLiteral("gridTable"))->height() > 100);
        // The code line is visible too (it used to overflow with the age field).
        QVERIFY(code->mapToScene(QPointF(0, 0)).toPoint().y() > 0);
    }
    m_window->resize(1008, 561);
}

void TestQmlMainWindow::detailFieldsFitTheirLine() {
    seedPatients();
    openScreen();

    const auto innerField = [this](const QString& name) -> qreal {
        auto* f = item(name)->findChild<QQuickItem*>(QStringLiteral("field"));
        return f != nullptr ? f->width() : -1.0;
    };
    auto* age = item(QStringLiteral("ageInput"));
    auto* date = item(QStringLiteral("dateInput"));
    auto* sex = item(QStringLiteral("sexInput"));
    auto* cohabitants = item(QStringLiteral("cohabitantsInput"));
    auto* contact = item(QStringLiteral("contactInput"));
    auto* siblings = item(QStringLiteral("siblingsInput"));

    // Both boxes share the panel height now (three control lines each).
    QCOMPARE(item(QStringLiteral("basicDataGroup"))->height(),
             item(QStringLiteral("addressGroup"))->height());

    // The age only holds three digits, so the date takes the rest of the line
    // and its text field must fit a whole dd/MM/yyyy.
    QVERIFY2(age->width() <= 100, qPrintable(QStringLiteral("age=%1").arg(age->width())));
    QVERIFY(item(QStringLiteral("ageInput"))->findChild<QQuickItem*>(QStringLiteral("field"))
            != nullptr);
    QVERIFY2(innerField(QStringLiteral("ageInput")) >= 36, "three digits fit in the age field");
    QVERIFY2(innerField(QStringLiteral("dateInput")) >= 120,
             qPrintable(QStringLiteral("date field=%1").arg(innerField(QStringLiteral("dateInput")))));
    QVERIFY2(date->width() > age->width() * 2, "the date is the wide one");

    // Address: line 1 full width, line 2 cohabitants + contact, line 3 siblings.
    const auto line = [](QQuickItem* it) { return it->mapToScene(QPointF(0, 0)).toPoint().y(); };
    QVERIFY2(qAbs(line(cohabitants) - line(contact)) <= 1, "cohabitants and contact share the line");
    QVERIFY(contact->x() > cohabitants->x());
    QVERIFY(line(siblings) > line(contact));
    // The siblings field starts at the left edge, like the other lines.
    QVERIFY(qAbs(siblings->x() - item(QStringLiteral("addressInput"))->x()) <= 1);
    QVERIFY2(siblings->width() <= 130, "the siblings field stays narrow");
    QVERIFY(innerField(QStringLiteral("siblingsInput")) >= 30);
}

void TestQmlMainWindow::calendarPopupIsVisible() {
    seedPatients();
    openScreen();

    // The calendar is parented to the window overlay: as a child of the field
    // it was painted inside the panel and never appeared.
    // The form fields are only editable while adding/editing a patient.
    clickIn(m_window, item(QStringLiteral("addButton")));
    QVERIFY(m_controller->editing());
    auto* date = item(QStringLiteral("dateInput"));
    QVERIFY(date != nullptr);
    auto* calendarButton = date->findChild<QQuickItem*>(QStringLiteral("calendarButton"));
    QVERIFY(calendarButton != nullptr);
    QVERIFY(calendarButton->isEnabled());
    clickIn(m_window, calendarButton);
    QTest::qWait(200); // the popup needs a moment to be laid out

    auto* popup = date->findChild<QObject*>(QStringLiteral("calendarPopup"));
    QVERIFY(popup != nullptr);
    QCOMPARE(popup->property("visible").toBool(), true);
    // The month grid is the visible part of the calendar: it must land inside
    // the window, otherwise the popup paints outside its parent item.
    auto* monthGrid =
        qobject_cast<QQuickItem*>(popup->findChild<QObject*>(QStringLiteral("monthGrid")));
    QVERIFY(monthGrid != nullptr);
    const QRectF grid = monthGrid->mapRectToScene(QRectF(0, 0, monthGrid->width(),
                                                          monthGrid->height()));
    const QRectF windowRect(0, 0, m_window->width(), m_window->height());
    QVERIFY2(grid.intersects(windowRect), "the calendar is painted inside the window");
    QVERIFY2(grid.top() >= 0 && grid.left() >= 0 && grid.bottom() <= windowRect.height()
                 && grid.right() <= windowRect.width(),
             qPrintable(QStringLiteral("grid at %1,%2 %3x%4")
                            .arg(grid.x())
                            .arg(grid.y())
                            .arg(grid.width())
                            .arg(grid.height())));
}

void TestQmlMainWindow::bothGroupsAreAligned() {
    seedPatients();
    openScreen();

    auto* basic = item(QStringLiteral("basicDataGroup"));
    auto* address = item(QStringLiteral("addressGroup"));
    const auto top = [](QQuickItem* it) { return it->mapToScene(QPointF(0, 0)).toPoint().y(); };
    QCOMPARE(top(basic), top(address));
    QCOMPARE(basic->height(), address->height());
    // The action row sits above both groups (same structure as the Widgets).
    QVERIFY(top(item(QStringLiteral("detailActions"))) < top(basic));
    // Controls keep their 30px height inside the boxes.
    QCOMPARE(item(QStringLiteral("nameInput"))->height(), 30);
    QCOMPARE(item(QStringLiteral("addressInput"))->height(), 30);
    QCOMPARE(item(QStringLiteral("ageInput"))->height(), 30);
    QCOMPARE(item(QStringLiteral("siblingsInput"))->height(), 30);
    // The sex combo has room for its longest option.
    auto* combo = item(QStringLiteral("sexInput"))->findChild<QQuickItem*>(QStringLiteral("comboBox"));
    QVERIFY(combo != nullptr);
    QVERIFY2(combo->width() >= 90,
             qPrintable(QStringLiteral("combo width=%1").arg(combo->width())));
}

void TestQmlMainWindow::gridTakesFocusAndKeyboardMoves() {
    seedPatients();
    openScreen();

    auto* grid = item(QStringLiteral("gridTable"));
    QVERIFY(grid != nullptr);
    // The list is focused as soon as the screen opens, with a patient selected.
    QVERIFY(grid->hasActiveFocus());
    QCOMPARE(m_controller->currentRow(), 0);
    // The wheel must stop at the ends instead of leaving a blank band.
    QCOMPARE(grid->property("boundsBehavior").toInt(), 0); // Flickable.StopAtBounds

    // The arrow keys move the selection and the detail follows.
    QTest::keyClick(m_window, Qt::Key_Down);
    QTRY_COMPARE(m_controller->currentRow(), 1);
    QCOMPARE(m_controller->patientName(), QStringLiteral("QMLW_BETA"));
    QCOMPARE(item(QStringLiteral("nameInput"))->property("text").toString(),
             QStringLiteral("QMLW_BETA"));

    QTest::keyClick(m_window, Qt::Key_Down);
    QTRY_COMPARE(m_controller->currentRow(), 2);
    QTest::keyClick(m_window, Qt::Key_Up);
    QTRY_COMPARE(m_controller->currentRow(), 1);

    // And the highlight is on the row the keys moved to.
    const QPoint topLeft = grid->mapToScene(QPointF(0, 0)).toPoint();
    QCOMPARE(pixelAt(m_window, topLeft, 1), ThemeHighlight());

    // Editing takes the focus to the form, and cancelling gives it back.
    clickIn(m_window, item(QStringLiteral("editButton")));
    QVERIFY(m_controller->editing());
    clickIn(m_window, item(QStringLiteral("cancelButton")));
    QVERIFY(!m_controller->editing());
    QTRY_VERIFY(grid->hasActiveFocus());
}

void TestQmlMainWindow::filterBoxesNarrowTheGrid() {
    seedPatients();
    openScreen();

    // Filters are anchored at the start, so '*' widens the match.
    m_controller->setColumnFilter(PatientsModel::ColumnName, QStringLiteral("*BETA"));
    QCOMPARE(m_controller->gridModel()->rowCount(), 1);
    QCOMPARE(m_controller->patientName(), QStringLiteral("QMLW_BETA"));

    // A filter with no match empties the grid and the detail.
    m_controller->setColumnFilter(PatientsModel::ColumnName, QStringLiteral("*ZZZ"));
    QCOMPARE(m_controller->gridModel()->rowCount(), 0);
    QCOMPARE(item(QStringLiteral("nameInput"))->property("text").toString(), QString());
}

} // namespace gambasse

int main(int argc, char** argv) {
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication app(argc, argv);
    gambasse::TestQmlMainWindow test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_qmlmainwindow.moc"
