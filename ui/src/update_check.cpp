#include "update_check.h"
#include "github_mirror.h"

#include "open_location.h"
#include <QUrl>

#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

namespace stud::ui {
namespace {

constexpr const char* kLatestApi = "https://api.github.com/repos/vivo6810/stud-with-better-ui-/releases/latest";

bool g_started = false;
bool g_update_available = false;
QString g_latest;

// Numeric (1.10.0 > 1.9.0). Unparseable counts as not newer, so a bad tag never nags.
bool is_newer(const QString& candidate, const QString& current) {
    const QStringList a = candidate.split('.');
    const QStringList b = current.split('.');
    for (int i = 0; i < qMax(a.size(), b.size()); ++i) {
        bool a_ok = false;
        bool b_ok = false;
        const int lhs = i < a.size() ? a.at(i).toInt(&a_ok) : 0;
        const int rhs = i < b.size() ? b.at(i).toInt(&b_ok) : 0;
        if (i < a.size() && !a_ok) return false;
        if (i < b.size() && !b_ok) return false;
        if (lhs != rhs) return lhs > rhs;
    }
    return false;
}

}  // namespace

UpdateCheck::UpdateCheck(QObject* parent) : QObject(parent) {}

void UpdateCheck::open() {
    open_location(QUrl(releasesUrl()));
}

UpdateCheck* UpdateCheck::instance() {
    static UpdateCheck check;
    return &check;
}

QString UpdateCheck::releasesUrl() {
    return QStringLiteral("https://github.com/vivo6810/stud-with-better-ui-/releases/latest");
}

bool UpdateCheck::updateAvailable() { return g_update_available; }

QString UpdateCheck::latestVersion() { return g_latest; }

void UpdateCheck::start() {
    if (g_started) return;
    g_started = true;

    github::get_either(QUrl(QString::fromLatin1(kLatestApi)),
        QUrl(QStringLiteral("https://data.jsdelivr.com/v1/packages/gh/vivo6810/stud-with-better-ui-/resolved?specifier=latest")),
        [](const QByteArray& body, bool from_jsdelivr, const QString& error) {
            if (!error.isEmpty()) {
                g_started = false;
                return;
            }
            QString tag = QJsonDocument::fromJson(body).object()
                .value(from_jsdelivr ? QStringLiteral("version") : QStringLiteral("tag_name")).toString();
            if (tag.startsWith('v', Qt::CaseInsensitive)) tag.remove(0, 1);
            if (!is_newer(tag, QStringLiteral(STUD_VERSION))) return;
            g_update_available = true;
            g_latest = tag;
            Q_EMIT instance()->updateFound(tag);
        });
}

} // namespace stud::ui
