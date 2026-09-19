#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

#include <Paths.h>
#include <data/model/Patient.h>
#include <logic/PhotoManager.h>

namespace gambasse {
namespace {

Patient makePatient() {
    Patient p;
    p.id = 1;
    p.name = QStringLiteral("Ana");
    p.birthDate = QDate(2000, 1, 2);
    p.sex = Patient::Sex::Home;
    p.ageRange = 24;
    return p;
}

void writeFile(const QString& path) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write("x");
}

} // namespace

// Verifies photo path resolution and the rename/remove file operations.
class TestPhotoManager : public QObject {
    Q_OBJECT

private slots:
    void defaultResource();
    void pathUsesBasePath();
    void renameMovesFile();
    void removeDeletesFile();
};

void TestPhotoManager::defaultResource() {
    QCOMPARE(PhotoManager().defaultPhotoResource(), QStringLiteral(":/img/foto0.png"));
}

void TestPhotoManager::pathUsesBasePath() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    setBasePath(dir.path());

    const Patient p = makePatient();
    QCOMPARE(PhotoManager().photoPath(p), QDir(dir.path()).filePath(p.photoFilename()));
}

void TestPhotoManager::renameMovesFile() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    setBasePath(dir.path());

    const Patient previous = makePatient();
    Patient current = previous;
    current.name = QStringLiteral("Ana Maria");

    const QString oldPath = PhotoManager().photoPath(previous);
    writeFile(oldPath);
    QVERIFY(QFileInfo::exists(oldPath));

    PhotoManager().renameFor(previous, current);

    QVERIFY(!QFileInfo::exists(oldPath));
    QVERIFY(QFileInfo::exists(PhotoManager().photoPath(current)));
}

void TestPhotoManager::removeDeletesFile() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    setBasePath(dir.path());

    const Patient p = makePatient();
    const QString path = PhotoManager().photoPath(p);
    writeFile(path);
    QVERIFY(QFileInfo::exists(path));

    PhotoManager().removeFor(p);
    QVERIFY(!QFileInfo::exists(path));
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestPhotoManager test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_photomanager.moc"
