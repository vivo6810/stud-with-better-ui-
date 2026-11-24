#pragma once

#include <QApplication>
#include <QSettings>

namespace stud::ui {

inline bool reduced_motion() {
    if (qApp && qApp->property("studReducedMotion").isValid())
        return qApp->property("studReducedMotion").toBool();
    const bool reduced = QSettings(QStringLiteral("Stud"), QStringLiteral("Interface"))
        .value(QStringLiteral("reduceMotion"), false).toBool();
    if (qApp) qApp->setProperty("studReducedMotion", reduced);
    return reduced;
}

inline void apply_launcher_theme(QApplication& app) {
    // Palette roles keep the user's light/dark colors, including contrast settings.
    app.setStyleSheet(QStringLiteral(R"(
        QWidget { selection-background-color: palette(highlight); selection-color: palette(highlighted-text); }
        QPushButton, QToolButton { padding: 9px 14px; border: 1px solid palette(mid); border-radius: 8px; background: palette(button); color: palette(button-text); }
        QPushButton:hover, QToolButton:hover { border-color: palette(highlight); }
        QPushButton:pressed, QToolButton:pressed { background: palette(midlight); }
        QPushButton:focus, QToolButton:focus { border: 2px solid palette(highlight); }
        QListWidget { background: palette(window); border: none; outline: none; }
        QListWidget::item { padding: 11px 16px; margin: 3px 0; border-radius: 7px; }
        QListWidget::item:selected { background: palette(highlight); color: palette(highlighted-text); }
        QLineEdit, QPlainTextEdit, QComboBox, QSpinBox, QDoubleSpinBox, QTextBrowser { padding: 7px; border: 1px solid palette(mid); border-radius: 6px; background: palette(base); color: palette(text); }
        QCheckBox { spacing: 10px; padding: 6px 0; }
        QTabWidget::pane { border: none; }
        QTabBar::tab { padding: 10px 16px; border-bottom: 2px solid transparent; }
        QTabBar::tab:selected { border-bottom-color: palette(highlight); }
        QScrollArea { border: none; }
        QProgressBar { border: none; border-radius: 4px; background: palette(midlight); min-height: 6px; max-height: 6px; }
        QProgressBar::chunk { background: palette(highlight); border-radius: 4px; }
    )"));
}

} // namespace stud::ui
