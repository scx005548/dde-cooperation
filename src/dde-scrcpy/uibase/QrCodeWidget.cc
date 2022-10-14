#include "QrCodeWidget.h"

#include "QPainter"

#include "QrCode.hpp"

using namespace qrcodegen;

QrCodeWidget::QrCodeWidget(QWidget *parent)
    : QWidget(parent) {
}

void QrCodeWidget::setText(const QString &text) {
    m_text = text;
    update();
}

void QrCodeWidget::paintEvent([[maybe_unused]] QPaintEvent *event) {
    QrCode qr0 = QrCode::encodeText(m_text.toStdString().c_str(), QrCode::Ecc::MEDIUM);

    QPainter painter(this);
    QColor fg("black");

    painter.setPen(Qt::NoPen);

    const int s = qr0.getSize();
    const double w = width();
    const double h = height();
    const double aspect = w / h;
    const double scale = ((aspect > 1.0) ? h : w) / s;

    painter.setBrush(fg);
    for (int y = 0; y < s; y++) {
        for (int x = 0; x < s; x++) {
            if (qr0.getModule(x, y)) {
                const double rx1 = x * scale, ry1 = y * scale;
                QRectF r(rx1, ry1, scale, scale);
                painter.drawRects(&r, 1);
            }
        }
    }
}
