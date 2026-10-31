#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
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

void writeFile(const QString& path, const QByteArray& content = "x") {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write(content);
}

QByteArray readFile(const QString& path) {
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    return file.readAll();
}

// A solid picture of the given size, saved in the format of its suffix.
QString writeImage(const QString& path, QSize size, QColor color = Qt::red) {
    QImage image(size, QImage::Format_RGB32);
    image.fill(color);
    QDir().mkpath(QFileInfo(path).absolutePath());
    image.save(path);
    return path;
}

// A JPEG whose EXIF orientation is 6 (rotate 90 degrees clockwise), the way a
// phone stores a portrait picture. QImageWriter cannot write the tag (it turns
// the pixels instead), so a minimal APP1 segment goes right after SOI.
QString writeExifRotatedJpeg(const QString& path, QSize stored) {
    QImage image(stored, QImage::Format_RGB32);
    image.fill(Qt::green);
    QByteArray jpeg;
    {
        QBuffer buffer(&jpeg);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "JPEG");
    }
    // Little-endian TIFF header, IFD0 with one SHORT entry: 0x0112 = 6.
    const QByteArray tiff =
        QByteArray::fromHex("49492A0008000000" "0100" "120103000100000006000000" "00000000");
    const QByteArray payload = QByteArray("Exif\0\0", 6) + tiff;
    const int length = payload.size() + 2;
    QByteArray app1 = QByteArray::fromHex("FFE1");
    app1.append(char(length >> 8)).append(char(length & 0xFF)).append(payload);
    jpeg.insert(2, app1);
    writeFile(path, jpeg);
    return path;
}

QSize storedSize(const QString& path) {
    return QImage(path).size();
}

} // namespace

// Verifies photo path resolution and the import/rename/remove file operations.
class TestPhotoManager : public QObject {
    Q_OBJECT

private:
    QTemporaryDir* m_dir = nullptr;
    PhotoManager m_photos;
    Patient m_patient = makePatient();

    QString sourcePath(const QString& name) const { return m_dir->filePath(QStringLiteral("in/") + name); }

private slots:
    void init();
    void cleanup();

    void defaultResource();
    void pathUsesBasePath();
    void existingPhotoPrefersTheCurrentFormat();
    void renameMovesFile();
    void removeDeletesCurrentAndLegacyFiles();
    void importReducesLargeImages();
    void importKeepsSmallImages();
    void importAppliesTheExifOrientation();
    void importFlattensTransparency();
    void importRejectsNonImages();
    void importReportsWriteErrors();
    void importCreatesThePhotoFolder();
    void importReplacesTheLegacyPhoto();
    void importAcceptsTheOwnPhoto();
};

void TestPhotoManager::init() {
    m_dir = new QTemporaryDir();
    QVERIFY(m_dir->isValid());
    setBasePath(m_dir->path());
}

void TestPhotoManager::cleanup() {
    delete m_dir;
    m_dir = nullptr;
}

void TestPhotoManager::defaultResource() {
    QCOMPARE(m_photos.defaultPhotoResource(), QStringLiteral(":/img/foto0.png"));
}

void TestPhotoManager::pathUsesBasePath() {
    QCOMPARE(m_photos.photoPath(m_patient), QDir(m_dir->path()).filePath(m_patient.photoFilename()));
}

void TestPhotoManager::existingPhotoPrefersTheCurrentFormat() {
    const QString current = m_photos.photoPath(m_patient);
    const QString legacy = m_photos.legacyPhotoPath(m_patient);

    QVERIFY(m_photos.existingPhotoPath(m_patient).isEmpty());
    QVERIFY(!m_photos.hasPhoto(m_patient));

    writeFile(legacy);
    QCOMPARE(m_photos.existingPhotoPath(m_patient), legacy);
    QVERIFY(m_photos.hasPhoto(m_patient));

    writeFile(current);
    QCOMPARE(m_photos.existingPhotoPath(m_patient), current);

    QFile::remove(legacy);
    QCOMPARE(m_photos.existingPhotoPath(m_patient), current);
}

void TestPhotoManager::renameMovesFile() {
    const Patient previous = m_patient;
    Patient current = previous;
    current.name = QStringLiteral("Ana Maria");

    const QString oldPath = m_photos.photoPath(previous);
    writeFile(oldPath);
    QVERIFY(QFileInfo::exists(oldPath));

    m_photos.renameFor(previous, current);

    QVERIFY(!QFileInfo::exists(oldPath));
    QVERIFY(QFileInfo::exists(m_photos.photoPath(current)));
}

void TestPhotoManager::removeDeletesCurrentAndLegacyFiles() {
    const QString current = m_photos.photoPath(m_patient);
    const QString legacy = m_photos.legacyPhotoPath(m_patient);
    writeFile(current);
    writeFile(legacy);

    QVERIFY(m_photos.removeFor(m_patient));
    QVERIFY(!QFileInfo::exists(current));
    QVERIFY(!QFileInfo::exists(legacy));
    // Nothing to remove is not a failure.
    QVERIFY(m_photos.removeFor(m_patient));
}

void TestPhotoManager::importReducesLargeImages() {
    const QString source = writeImage(sourcePath(QStringLiteral("large.jpg")), QSize(1200, 900));
    QCOMPARE(m_photos.importFor(m_patient, source), PhotoManager::ImportResult::Imported);

    const QString stored = m_photos.photoPath(m_patient);
    QCOMPARE(storedSize(stored), QSize(240, 180));
    QCOMPARE(QImageReader(stored).format(), QByteArray("jpeg"));
    // A tall picture is limited by the frame height instead.
    writeImage(source, QSize(300, 600));
    QCOMPARE(m_photos.importFor(m_patient, source), PhotoManager::ImportResult::Imported);
    QCOMPARE(storedSize(stored), QSize(102, 204));
}

void TestPhotoManager::importKeepsSmallImages() {
    const QString source = writeImage(sourcePath(QStringLiteral("small.png")), QSize(100, 80));
    QCOMPARE(m_photos.importFor(m_patient, source), PhotoManager::ImportResult::Imported);
    QCOMPARE(storedSize(m_photos.photoPath(m_patient)), QSize(100, 80));
}

void TestPhotoManager::importAppliesTheExifOrientation() {
    // Stored 300x100 and turned upright to 100x300, then reduced to the frame
    // height: 68x204.
    const QString source = writeExifRotatedJpeg(sourcePath(QStringLiteral("phone.jpg")), QSize(300, 100));
    QCOMPARE(m_photos.importFor(m_patient, source), PhotoManager::ImportResult::Imported);
    QCOMPARE(storedSize(m_photos.photoPath(m_patient)), QSize(68, 204));
}

void TestPhotoManager::importFlattensTransparency() {
    QImage image(50, 50, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    const QString source = sourcePath(QStringLiteral("clear.png"));
    QDir().mkpath(QFileInfo(source).absolutePath());
    QVERIFY(image.save(source));

    QCOMPARE(m_photos.importFor(m_patient, source), PhotoManager::ImportResult::Imported);
    const QImage stored(m_photos.photoPath(m_patient));
    QCOMPARE(stored.pixelColor(25, 25), QColor(Qt::white));
}

void TestPhotoManager::importRejectsNonImages() {
    const QString previous = writeImage(m_photos.photoPath(m_patient), QSize(10, 10));
    const QByteArray before = readFile(previous);
    const QString source = sourcePath(QStringLiteral("notes.jpg"));
    writeFile(source, "not a picture");

    QCOMPARE(m_photos.importFor(m_patient, source), PhotoManager::ImportResult::UnreadableImage);
    QCOMPARE(readFile(previous), before);
    QCOMPARE(m_photos.importFor(m_patient, sourcePath(QStringLiteral("missing.jpg"))),
             PhotoManager::ImportResult::UnreadableImage);
}

void TestPhotoManager::importReportsWriteErrors() {
    const QString previous = writeImage(m_photos.photoPath(m_patient), QSize(10, 10));
    const QByteArray before = readFile(previous);
    const QString source = writeImage(sourcePath(QStringLiteral("new.png")), QSize(50, 50));
    const QString folder = QFileInfo(previous).absolutePath();
    const QFile::Permissions permissions = QFile::permissions(folder);
    // A read-only fotos/ folder: the new photo cannot be written next to it.
    QFile::setPermissions(folder, QFile::ReadOwner | QFile::ExeOwner);
    if (QFileInfo(folder).isWritable()) {
        QFile::setPermissions(folder, permissions);
        QSKIP("The folder stays writable (running as root?)");
    }

    const PhotoManager::ImportResult result = m_photos.importFor(m_patient, source);
    QFile::setPermissions(folder, permissions);
    QCOMPARE(result, PhotoManager::ImportResult::WriteError);
    QCOMPARE(readFile(previous), before);
}

void TestPhotoManager::importCreatesThePhotoFolder() {
    QVERIFY(!QFileInfo::exists(m_dir->filePath(QStringLiteral("fotos"))));
    const QString source = writeImage(sourcePath(QStringLiteral("first.png")), QSize(50, 50));

    QCOMPARE(m_photos.importFor(m_patient, source), PhotoManager::ImportResult::Imported);
    QVERIFY(QFileInfo(m_dir->filePath(QStringLiteral("fotos"))).isDir());
    QVERIFY(QFileInfo::exists(m_photos.photoPath(m_patient)));
}

void TestPhotoManager::importReplacesTheLegacyPhoto() {
    const QString legacy = writeImage(m_photos.legacyPhotoPath(m_patient), QSize(10, 10));
    const QString source = writeImage(sourcePath(QStringLiteral("new.png")), QSize(50, 50));

    QCOMPARE(m_photos.importFor(m_patient, source), PhotoManager::ImportResult::Imported);
    QVERIFY(!QFileInfo::exists(legacy));
    QCOMPARE(m_photos.existingPhotoPath(m_patient), m_photos.photoPath(m_patient));
}

void TestPhotoManager::importAcceptsTheOwnPhoto() {
    const QString own = writeImage(m_photos.photoPath(m_patient), QSize(600, 300));
    QCOMPARE(m_photos.importFor(m_patient, own), PhotoManager::ImportResult::Imported);
    QCOMPARE(storedSize(own), QSize(240, 120));
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestPhotoManager test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_photomanager.moc"
