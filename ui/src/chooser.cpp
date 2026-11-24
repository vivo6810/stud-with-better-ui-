#include "motion.h"
#include "project_links.h"
#include "mono_icon.h"
#include "chooser.h"
#include "command_warning.h"

#include "cube.h"
#include "cube_view.h"
#include "gpu_enum.h"
#include "settings_window.h"
#include "update_check.h"
#include "stud/settings.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QEventLoop>
#include <QFrame>
#include <QProcess>
#include <QRandomGenerator>
#include <QFile>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPushButton>
#include <QHash>
#include <QScrollBar>
#include <QSet>
#include <QTextBrowser>
#include <QStyle>
#include <QTimer>
#include <QTransform>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <numbers>

namespace stud::ui {

namespace {

// QTextBrowser can't show remote images, so each one is downloaded and the notes relaid out.
class NotesView : public QTextBrowser {
public:
    using QTextBrowser::QTextBrowser;

    void show_html(const QString& html) {
        html_ = html;
        const int scroll = verticalScrollBar()->value();
        setHtml(html_);
        verticalScrollBar()->setValue(scroll);
    }

protected:
    QVariant loadResource(int type, const QUrl& url) override {
        if (type != QTextDocument::ImageResource || !url.scheme().startsWith(QStringLiteral("http")))
            return QTextBrowser::loadResource(type, url);
        if (images_.contains(url)) return images_.value(url);
        if (!requested_.contains(url)) {
            requested_.insert(url);
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Stud/" STUD_VERSION));
            QNetworkReply* reply = net_.get(request);
            QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, url] {
                reply->deleteLater();
                QImage image;
                if (reply->error() != QNetworkReply::NoError || !image.loadFromData(reply->readAll())) return;
                const int room = viewport()->width() - 2 * int(document()->documentMargin());
                if (room > 0 && image.width() > room) image = image.scaledToWidth(room, Qt::SmoothTransformation);
                images_.insert(url, image);
                show_html(html_);
            });
        }
        return {};
    }

private:
    QNetworkAccessManager net_;
    QString html_;
    QHash<QUrl, QImage> images_;
    QSet<QUrl> requested_;
};

void show_release_notes(QWidget* parent) {
    auto* dialog = new QDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(QStringLiteral("Stud %1").arg(QStringLiteral(STUD_VERSION)));
    auto* layout = new QVBoxLayout(dialog);
    auto* notes = new NotesView(dialog);
    notes->setOpenExternalLinks(true);
    auto* skeleton = new Skeleton(dialog);
    layout->addWidget(skeleton);
    auto* status = new QLabel(QStringLiteral("Loading release notes…"), dialog);
    layout->addWidget(status);
    notes->hide();
    QTimer::singleShot(4000, status, [status] {
        status->setText(QStringLiteral("Still loading. The connection may be slow; you can keep playing."));
    });
    layout->addWidget(notes);
    auto* close = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    QObject::connect(close, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    layout->addWidget(close);
    dialog->resize(dialog->sizeHint().width() * 2, dialog->sizeHint().height() * 2);
    dialog->show();

    auto* net = new QNetworkAccessManager(dialog);
    QNetworkRequest request(QUrl(QStringLiteral("https://api.github.com/repos/vivo6810/stud-with-better-ui-/releases/tags/%1")
                                     .arg(QStringLiteral(STUD_VERSION))));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Stud/" STUD_VERSION));
    request.setRawHeader("Accept", "application/vnd.github.html+json");
    request.setTransferTimeout(10000);
    QNetworkReply* reply = net->get(request);
    QObject::connect(reply, &QNetworkReply::finished, notes, [reply, notes, skeleton, status, dialog, layout] {
        skeleton->hide();
        status->hide();
        notes->show();
        reveal(notes);
        reply->deleteLater();
        const QJsonObject release = QJsonDocument::fromJson(reply->readAll()).object();
        const QString body = release.value(QStringLiteral("body_html")).toString();
        if (reply->error() != QNetworkReply::NoError || body.isEmpty()) {
            notes->setText(QStringLiteral("Release notes could not be loaded. You can play without them."));
            auto* retry = new QPushButton(QStringLiteral("Retry"), dialog);
            layout->addWidget(retry);
            QObject::connect(retry, &QPushButton::clicked, dialog, [dialog] {
                show_release_notes(dialog->parentWidget());
                dialog->close();
            });
            return;
        }
        const QString name = release.value(QStringLiteral("name")).toString();
        notes->show_html((name.isEmpty() ? QString() : QStringLiteral("<h1>%1</h1>").arg(name.toHtmlEscaped())) +
                         body);
    });
}

// Stud installed from an rpm that went without WPE WebKit: the rpm only
// recommends it, since it comes from Terra, which plain Fedora lacks.
bool rpm_without_wpe() {
    const auto succeeds = [](const QStringList& arguments) {
        QProcess rpm;
        rpm.start(QStringLiteral("rpm"), arguments);
        return rpm.waitForFinished() && rpm.exitStatus() == QProcess::NormalExit && rpm.exitCode() == 0;
    };
    return succeeds({QStringLiteral("-qf"), QCoreApplication::applicationFilePath()}) &&
           !succeeds({QStringLiteral("-q"), QStringLiteral("--whatprovides"),
                      QStringLiteral("libWPEWebKit-2.0.so.1()(64bit)")});
}

}  // namespace

std::optional<client::Product> choose_product() {
    QDialog dialog;
    dialog.setWindowTitle(QStringLiteral("Stud · Roblox for Linux"));
    const int unit = dialog.fontMetrics().height();
    dialog.resize(unit * 48, unit * 30);
    dialog.setMinimumSize(unit * 34, unit * 24);
    auto* layout = new QHBoxLayout(&dialog);
    layout->setContentsMargins(unit * 2, unit * 2, unit * 2, unit * 2);
    layout->setSpacing(unit * 3);

    auto* navigation = new QVBoxLayout;
    auto* brand = new QLabel(QStringLiteral("STUD"), &dialog);
    QFont brandFont = brand->font();
    brandFont.setPointSizeF(brandFont.pointSizeF() * 1.7);
    brandFont.setBold(true);
    brand->setFont(brandFont);
    navigation->addWidget(brand);
    navigation->addWidget(new QLabel(QStringLiteral("FOR LINUX"), &dialog));
    navigation->addSpacing(unit * 3);
    auto* home = new QPushButton(QStringLiteral("Home"), &dialog);
    home->setCheckable(true);
    home->setChecked(true);
    navigation->addWidget(home);
    auto* settings = new QPushButton(QStringLiteral("Settings"), &dialog);
    QObject::connect(settings, &QPushButton::clicked, &dialog, [] { SettingsWindow::showSingleton(); });
    navigation->addWidget(settings);
    auto* notes = new QPushButton(QStringLiteral("What's new"), &dialog);
    QObject::connect(notes, &QPushButton::clicked, &dialog, [&dialog] { show_release_notes(&dialog); });
    navigation->addWidget(notes);
    navigation->addStretch();
    navigation->addWidget(discord_button(&dialog, true));
    auto* version = new QPushButton(QStringLiteral("Stud %1").arg(QStringLiteral(STUD_VERSION)), &dialog);
    version->setFlat(true);
    QObject::connect(version, &QPushButton::clicked, &dialog, [] { UpdateCheck::open(); });
    QObject::connect(UpdateCheck::instance(), &UpdateCheck::updateFound, version, [version](const QString& value) {
        version->setText(QStringLiteral("Update available · %1").arg(value));
    });
    navigation->addWidget(version);
    layout->addLayout(navigation, 1);

    auto* content = new QWidget(&dialog);
    auto* body = new QVBoxLayout(content);
    body->setSpacing(unit);
    body->addWidget(new QLabel(QStringLiteral("YOUR NEXT SESSION STARTS HERE"), content));
    auto* title = new QLabel(QStringLiteral("Ready when you are."), content);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 2);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setWordWrap(true);
    body->addWidget(title);
    auto* subtitle = new QLabel(QStringLiteral("The full Roblox experience. Right here on Linux."), content);
    subtitle->setWordWrap(true);
    body->addWidget(subtitle);
    body->addStretch();
    auto* cube = new CubeView(content);
    cube->setFixedSize(unit * 7, unit * 7);
    body->addWidget(cube, 0, Qt::AlignHCenter);
    auto* player = new QLabel(QStringLiteral("Roblox Player"), content);
    player->setAlignment(Qt::AlignCenter);
    body->addWidget(player);
    auto* caption = new QLabel(QStringLiteral("Client updates are checked when you play."), content);
    caption->setAlignment(Qt::AlignCenter);
    caption->setWordWrap(true);
    body->addWidget(caption);
    std::optional<client::Product> chosen;
    auto* play = new PlayButton(content);
    QObject::connect(play, &QPushButton::clicked, &dialog, [&] {
        chosen = client::Product::Player;
        dialog.accept();
    });
    body->addWidget(play);
    body->addStretch();
    if (rpm_without_wpe()) {
        body->addWidget(command_warning(content,
            QStringLiteral("Sign-in requires WPE WebKit. Install it from Terra, then restart Stud:"),
            QStringLiteral("sudo dnf install --nogpgcheck --repofrompath 'terra,https://repos.fyralabs.com/terra$releasever' terra-release && sudo dnf install wpewebkit")));
    }
    layout->addWidget(content, 4);
    QObject::connect(home, &QPushButton::clicked, content, [content] { reveal(content); });
    UpdateCheck::start();
    dialog.setWindowModality(Qt::NonModal);
    QEventLoop loop;
    QObject::connect(&dialog, &QDialog::finished, &loop, &QEventLoop::quit);
    dialog.show();
    reveal(content);
    loop.exec();
    return chosen;
}

} // namespace stud::ui
