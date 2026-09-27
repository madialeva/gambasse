#include <ui/AppIcon.h>

#include <QImage>
#include <QImageReader>
#include <QPixmap>

namespace gambasse {

QIcon applicationIcon() {
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256}) {
        QImageReader reader(QStringLiteral(":/img/logo.svg"));
        reader.setScaledSize(QSize(size, size));
        const QImage image = reader.read();
        if (!image.isNull())
            icon.addPixmap(QPixmap::fromImage(image));
    }
    return icon;
}

} // namespace gambasse
