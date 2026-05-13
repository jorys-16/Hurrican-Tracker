#include "hurricanedata.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

QString HurricanePoint::categoryColor(int cat) {
    switch (cat) {
        case -1: return "#808080"; // Tropical Depression - grey
        case  0: return "#00BFFF"; // Tropical Storm     - deep sky blue
        case  1: return "#00FF00"; // Category 1         - green
        case  2: return "#FFFF00"; // Category 2         - yellow
        case  3: return "#FFA500"; // Category 3         - orange
        case  4: return "#FF4500"; // Category 4         - red-orange
        case  5: return "#FF00FF"; // Category 5         - magenta
        default: return "#FFFFFF";
    }
}

// Determine category from max sustained wind speed (knots)
static int windToCategory(int knots) {
    if (knots < 34)  return -1; // Tropical Depression
    if (knots < 64)  return  0; // Tropical Storm
    if (knots < 83)  return  1; // Cat 1
    if (knots < 96)  return  2; // Cat 2
    if (knots < 113) return  3; // Cat 3
    if (knots < 137) return  4; // Cat 4
    return 5;                   // Cat 5
}

QList<HurricanePoint> parseHurricaneCSV(const QString &filePath) {
    QList<HurricanePoint> points;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file:" << filePath;
        return points;
    }

    QTextStream in(&file);
    bool firstLine = true;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        // Skip header row
        if (firstLine) {
            firstLine = false;
            // If the first field isn't a number, it's a header — skip it
            QStringList fields = line.split(',');
            bool ok;
            fields[0].trimmed().toDouble(&ok);
            if (!ok) continue;
        }

        QStringList fields = line.split(',');
        // Expect at least 5 columns: Date, Time, Lat, Lon, Wind
        if (fields.size() < 5) continue;

        HurricanePoint p;
        bool ok1, ok2, ok3;
        p.date      = fields[0].trimmed();
        p.time      = fields[1].trimmed();
        p.lat       = fields[2].trimmed().toDouble(&ok1);
        p.lon       = fields[3].trimmed().toDouble(&ok2);
        p.windSpeed = fields[4].trimmed().toInt(&ok3);

        if (!ok1 || !ok2 || !ok3) continue;

        // Use column 5 if it provides category directly, otherwise derive it
        if (fields.size() >= 6) {
            bool catOk;
            int cat = fields[5].trimmed().toInt(&catOk);
            p.category = catOk ? cat : windToCategory(p.windSpeed);
        } else {
            p.category = windToCategory(p.windSpeed);
        }

        points.append(p);
    }
    return points;
}
