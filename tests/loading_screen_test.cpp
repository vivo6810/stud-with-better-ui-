#include "loading_screen.h"
#include "check.h"

#include <QApplication>
#include <QEventLoop>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setProperty("studReducedMotion", true);
    using namespace stud::ui;
    int cancellations = 0;
    const auto open = [&] {
        show_loading_lines([&] { ++cancellations; });
        for (QWidget* window : QApplication::topLevelWidgets()) {
            if (window->windowTitle() == QStringLiteral("Stud")) return window;
        }
        return static_cast<QWidget*>(nullptr);
    };
    QWidget* window = open();
    check(window != nullptr, "loading screen opens");
    app.processEvents();
    const QSize size = window->size();
    set_loading_progress(20);
    app.processEvents();
    expect(window->size() == size, "known progress preserves geometry");
    set_loading_progress(-1);
    app.processEvents();
    expect(window->size() == size, "unknown progress preserves geometry");
    QTimer* slow = nullptr;
    for (QTimer* timer : window->findChildren<QTimer*>()) {
        if (timer->isSingleShot() && timer->interval() == 8000) slow = timer;
    }
    check(slow != nullptr, "slow-operation timer exists");
    slow->start(1);
    QEventLoop wait;
    QTimer::singleShot(20, &wait, &QEventLoop::quit);
    wait.exec();
    set_loading_progress(21);
    bool hint = false;
    for (QLabel* label : window->findChildren<QLabel*>())
        hint = hint || label->text().startsWith(QStringLiteral("Taking longer"));
    expect(hint, "progress does not overwrite a slow-operation hint");
    window->close();
    app.processEvents();
    expect(cancellations == 1 && loading_cancelled(), "title-bar close cancels exactly once");
    window = open();
    check(window != nullptr, "screen can reopen after cancellation");
    expect(!loading_cancelled(), "new operation clears cancellation");
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(window, &escape);
    app.processEvents();
    expect(cancellations == 2, "Escape uses cancellation path");
    window = open();
    check(window != nullptr, "screen reopens for button cancellation");
    for (QPushButton* button : window->findChildren<QPushButton*>())
        if (button->text() == QStringLiteral("Cancel")) button->click();
    app.processEvents();
    expect(cancellations == 3, "Cancel button uses cancellation path");
    close_loading_screen();
    return finish("loading lifecycle");
}
