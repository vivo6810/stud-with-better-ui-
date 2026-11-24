#pragma once

#include <QStringList>

namespace stud::ui {

inline QStringList without_studio_associations(const QStringList& lines, const QString& app_id) {
    QStringList result;
    QString section;
    const QStringList owned{app_id + QStringLiteral(".desktop"),
                            app_id + QStringLiteral(".Studio.desktop"), QStringLiteral("stud.desktop")};
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QLatin1Char('['))) section = trimmed;
        const bool associations = section == QStringLiteral("[Default Applications]") ||
                                  section == QStringLiteral("[Added Associations]") ||
                                  section == QStringLiteral("[Removed Associations]");
        const int equal = line.indexOf(QLatin1Char('='));
        const QString key = line.left(equal).trimmed();
        if (!associations || equal < 0 ||
            (key != QStringLiteral("x-scheme-handler/roblox-studio") &&
             key != QStringLiteral("x-scheme-handler/roblox-studio-auth") &&
             key != QStringLiteral("application/x-roblox-rbxl") &&
             key != QStringLiteral("application/x-roblox-rbxlx"))) {
            result << line;
            continue;
        }
        QStringList entries = line.mid(equal + 1).split(QLatin1Char(';'), Qt::SkipEmptyParts);
        bool changed = false;
        for (const QString& entry : owned) changed = entries.removeAll(entry) > 0 || changed;
        if (!changed) result << line;
        else if (!entries.isEmpty()) result << key + QLatin1Char('=') + entries.join(QLatin1Char(';')) + QLatin1Char(';');
    }
    return result;
}

} // namespace stud::ui
