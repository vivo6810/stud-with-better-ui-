#include "loading_screen.h"

#include "cube_view.h"
#include "motion.h"

#include <QCoreApplication>
#include <QCloseEvent>
#include <QKeyEvent>
#include <algorithm>
#include <QFile>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QRandomGenerator>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>
#include <memory>

namespace stud::ui {
namespace {

struct Screen {
    std::unique_ptr<QWidget> window;
    QLabel* label = nullptr;
    QPushButton* cancel = nullptr;
    QProgressBar* bar = nullptr;
    QTimer* timer = nullptr;
    Skeleton* skeleton = nullptr;
    QLabel* hint = nullptr;
    QTimer* slow = nullptr;
    QStringList lines;
    qsizetype last = -1;
    bool lines_on = false;
    bool text_on = false;
    bool cancelled = false;
    bool slow_shown = false;
    std::function<void()> on_cancel;
};

Screen& screen() {
    static Screen s;
    return s;
}

void request_cancel() {
    Screen& s = screen();
    if (s.cancelled || !s.on_cancel) return;
    s.cancelled = true;
    auto on_cancel = std::move(s.on_cancel);
    QTimer::singleShot(0, qApp, [on_cancel] {
        close_loading_screen();
        if (on_cancel) on_cancel();
    });
}

class LoadingWindow : public QWidget {
public:
    LoadingWindow() : QWidget(nullptr, Qt::Dialog) {}
protected:
    void closeEvent(QCloseEvent* event) override {
        if (screen().on_cancel || screen().cancelled) {
            event->ignore();
            request_cancel();
        } else {
            QWidget::closeEvent(event);
        }
    }
    void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_Escape && screen().on_cancel) {
            request_cancel();
            event->accept();
        } else {
            QWidget::keyPressEvent(event);
        }
    }
};

constexpr int kWidth = 460;

void fit() {
    Screen& s = screen();
    if (s.window == nullptr) return;
    s.window->setFixedSize(kWidth, std::max(s.window->minimumHeight(), s.window->layout()->totalHeightForWidth(kWidth)));
}

void next_line() {
    Screen& s = screen();
    if (s.lines.isEmpty() || s.text_on) return;
    // Never the same line twice in a row.
    qsizetype i = QRandomGenerator::global()->bounded(s.lines.size());
    if (s.lines.size() > 1 && i == s.last) i = (i + 1) % s.lines.size();
    s.last = i;
    s.label->setText(s.lines.at(i));
    fit();
}

void raise() {
    Screen& s = screen();
    if (s.window == nullptr) {
        // A dialog of a fixed size: tiling window managers float both.
        s.window = std::make_unique<LoadingWindow>();
        s.window->setWindowTitle(QStringLiteral("Stud"));
        auto* layout = new QVBoxLayout(s.window.get());
        layout->setContentsMargins(28, 28, 28, 28);
        layout->setSpacing(16);
        auto* heading = new QLabel(QStringLiteral("Getting things ready"), s.window.get());
        QFont font = heading->font();
        font.setPointSizeF(font.pointSizeF() * 1.5);
        font.setBold(true);
        heading->setFont(font);
        layout->addWidget(heading, 0, Qt::AlignHCenter);
        auto* stud = new CubeView(s.window.get());
        stud->setFixedSize(128, 128);
        layout->addWidget(stud, 0, Qt::AlignHCenter);
        s.label = new QLabel(s.window.get());
        s.label->setAlignment(Qt::AlignCenter);
        s.label->setWordWrap(true);
        s.label->setMinimumHeight(s.label->fontMetrics().lineSpacing() * 3);
        layout->addWidget(s.label);
        // A moving bar while nobody knows how far along it is; a progress bar
        // while something says (a download, the unpacking).
        s.skeleton = new Skeleton(s.window.get());
        auto skeletonPolicy = s.skeleton->sizePolicy();
        skeletonPolicy.setRetainSizeWhenHidden(true);
        s.skeleton->setSizePolicy(skeletonPolicy);
        layout->addWidget(s.skeleton);
        s.hint = new QLabel(QStringLiteral("Preparing Roblox Player…"), s.window.get());
        s.hint->setWordWrap(true);
        s.hint->setMinimumHeight(s.hint->fontMetrics().lineSpacing() * 5);
        s.hint->setAlignment(Qt::AlignCenter);
        layout->addWidget(s.hint);
        s.slow = new QTimer(s.window.get());
        s.slow->setSingleShot(true);
        s.slow->setInterval(8000);
        QObject::connect(s.slow, &QTimer::timeout, s.window.get(), [] {
            screen().slow_shown = true;
            screen().hint->setText(QStringLiteral("Taking longer than usual. If a download is in progress, your connection may be slow. You can cancel and try again."));
            fit();
        });
        s.slow->start();
        s.bar = new QProgressBar(s.window.get());
        s.bar->setRange(0, 0);
        s.bar->setTextVisible(false);
        layout->addWidget(s.bar);
        s.cancel = new QPushButton(QStringLiteral("Cancel"), s.window.get());
        s.cancel->hide();
        QObject::connect(s.cancel, &QPushButton::clicked, s.window.get(), request_cancel);
        layout->addWidget(s.cancel, 0, Qt::AlignHCenter);
        fit();
        s.timer = new QTimer(s.window.get());
        s.timer->setInterval(2000);
        QObject::connect(s.timer, &QTimer::timeout, s.window.get(), next_line);
    }
    s.window->show();
    s.window->raise();
    // Painted now: what follows may not return to an event loop for a
    // moment (unpacking, building, starting the other processes).
    QCoreApplication::processEvents();
}

}  // namespace

void show_loading_lines(std::function<void()> on_cancel) {
    Screen& s = screen();
    s.on_cancel = std::move(on_cancel);
    if (s.lines.isEmpty()) {
        QFile file(QStringLiteral(":/loading_lines.txt"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            for (const QString& line : QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'))) {
                if (!line.trimmed().isEmpty()) s.lines.append(line.trimmed());
            }
        }
    }
    s.cancelled = false;
    s.lines_on = true;
    raise();
    if (!s.window || s.cancelled) return;
    s.cancel->setVisible(s.on_cancel != nullptr);
    fit();
    next_line();
    s.timer->start();
}

bool loading_cancelled() { return screen().cancelled; }

void show_loading_text(const QString& text) {
    Screen& s = screen();
    s.text_on = true;
    raise();
    if (!s.window || s.cancelled) return;
    s.label->setText(text);
    fit();
    QCoreApplication::processEvents();
}

void set_loading_progress(int percent) {
    Screen& s = screen();
    if (s.bar == nullptr) return;
    s.skeleton->setVisible(percent < 0);
    fit();
    if (percent >= 0 && !s.slow_shown) {
        s.hint->setText(QStringLiteral("Preparing Roblox Player · %1%").arg(percent));
    }
    if (percent < 0) {
        s.bar->setRange(0, 0);
    } else {
        s.bar->setRange(0, 100);
        s.bar->setValue(percent);
    }
}

void clear_loading_text() {
    Screen& s = screen();
    set_loading_progress(-1);
    if (!s.text_on) return;
    s.text_on = false;
    if (s.lines_on) {
        next_line();
    } else if (s.window != nullptr) {
        s.window->hide();
    }
}

void close_loading_screen() {
    Screen& s = screen();
    s.lines_on = false;
    s.text_on = false;
    s.window.reset();
    s.label = nullptr;
    s.cancel = nullptr;
    s.bar = nullptr;
    s.timer = nullptr;
    s.skeleton = nullptr;
    s.hint = nullptr;
    s.slow = nullptr;
    s.slow_shown = false;
    s.on_cancel = {};
}

}  // namespace stud::ui
