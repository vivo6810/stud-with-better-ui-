#pragma once

#include "theme.h"

#include <QAbstractButton>
#include <QFontMetrics>
#include <QHideEvent>
#include <QEnterEvent>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QShowEvent>
#include <QTimer>
#include <QVariantAnimation>
#include <cmath>

namespace stud::ui {

inline void reveal(QWidget* widget) {
    if (!widget || reduced_motion()) return;
    auto* effect = new QGraphicsOpacityEffect(widget);
    widget->setGraphicsEffect(effect);
    auto* animation = new QPropertyAnimation(effect, "opacity", widget);
    animation->setDuration(240);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

// Paint the stretch inside the button: its layout and hit target never move.
class PlayButton : public QPushButton {
public:
    explicit PlayButton(QWidget* parent) : QPushButton(QStringLiteral("Play Roblox"), parent) {
        setAccessibleName(text());
        setCursor(Qt::PointingHandCursor);
        setMinimumHeight(fontMetrics().height() * 4);
        motion_.setDuration(420);
        motion_.setEasingCurve(QEasingCurve::OutBack);
        connect(&motion_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
            stretch_ = value.toReal();
            update();
        });
        connect(this, &QPushButton::pressed, this, [this] { animate(0.94); });
        connect(this, &QPushButton::released, this, [this] { animate(1.0); });
    }
protected:
    void enterEvent(QEnterEvent* event) override {
        animate(1.025);
        QPushButton::enterEvent(event);
    }
    void leaveEvent(QEvent* event) override {
        animate(1.0);
        QPushButton::leaveEvent(event);
    }
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(rect().center());
        painter.scale(stretch_, 1.0 / stretch_);
        const QRectF face(-width() * 0.46, -height() * 0.40, width() * 0.92, height() * 0.80);
        painter.setPen(Qt::NoPen);
        painter.setBrush(isEnabled() ? QColor(QStringLiteral("#335fff")) : palette().color(QPalette::Mid));
        painter.drawRoundedRect(face, 10, 10);
        if (hasFocus()) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(palette().color(QPalette::WindowText), 2, Qt::DashLine));
            painter.drawRoundedRect(face.adjusted(-4, -4, 4, 4), 12, 12);
        }
        painter.setPen(Qt::white);
        QFont label = font();
        label.setBold(true);
        painter.setFont(label);
        painter.drawText(face, Qt::AlignCenter, text());
    }
private:
    void animate(qreal end) {
        if (reduced_motion()) { stretch_ = 1.0; update(); return; }
        motion_.stop();
        motion_.setStartValue(stretch_);
        motion_.setEndValue(end);
        motion_.start();
    }
    QVariantAnimation motion_;
    qreal stretch_ = 1.0;
};

// Solid pulsing placeholders, never a gradient or a glow.
class Skeleton : public QWidget {
public:
    explicit Skeleton(QWidget* parent = nullptr) : QWidget(parent) {
        setAccessibleName(QStringLiteral("Loading content"));
        setMinimumHeight(fontMetrics().height() * 6);
        pulse_.setStartValue(0.0);
        pulse_.setEndValue(6.283185307);
        pulse_.setDuration(1600);
        pulse_.setLoopCount(-1);
        connect(&pulse_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
            phase_ = value.toReal();
            update();
        });
    }
protected:
    void showEvent(QShowEvent* event) override { if (!reduced_motion()) pulse_.start(); QWidget::showEvent(event); }
    void hideEvent(QHideEvent* event) override { pulse_.stop(); QWidget::hideEvent(event); }
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QColor color = palette().color(QPalette::Text);
        color.setAlphaF(0.06 + 0.05 * (1.0 + std::sin(phase_)));
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        const qreal unit = fontMetrics().height();
        painter.drawRoundedRect(QRectF(0, unit / 2, width(), unit * 2), 8, 8);
        painter.drawRoundedRect(QRectF(0, unit * 3, width() * 0.72, unit * 0.65), 4, 4);
        painter.drawRoundedRect(QRectF(0, unit * 4.2, width() * 0.46, unit * 0.65), 4, 4);
    }
private:
    QVariantAnimation pulse_;
    qreal phase_ = 0;
};

} // namespace stud::ui
