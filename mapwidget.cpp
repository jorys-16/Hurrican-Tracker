#include "mapwidget.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QFont>
#include <QFontMetrics>
#include <cmath>

MapWidget::MapWidget(QWidget *parent)
    : QWidget(parent)
    , m_scale(1.0)
    , m_offset(0, 0)
    , m_dragging(false)
{
    setMinimumSize(800, 500);
    setMouseTracking(true);
    // Dark ocean background
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor("#0a1628"));
    setAutoFillBackground(true);
    setPalette(pal);
}

void MapWidget::setData(const QList<HurricanePoint> &points) {
    m_points = points;
    update();
}

// Convert geographic coords → screen pixels (equirectangular projection)
QPointF MapWidget::geoToScreen(double lat, double lon) const {
    double w = width();
    double h = height();

    double lonRange = m_lonMax - m_lonMin;
    double latRange = m_latMax - m_latMin;

    double x = ((lon - m_lonMin) / lonRange) * w;
    // Latitude increases upward, screen Y increases downward
    double y = ((m_latMax - lat) / latRange) * h;

    // Apply zoom and pan
    x = x * m_scale + m_offset.x();
    y = y * m_scale + m_offset.y();
    return {x, y};
}

void MapWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width(), h = height();

    // --- Draw simple ocean/land grid lines ---
    painter.setPen(QPen(QColor(255,255,255,25), 1));
    // Longitude lines every 10 degrees
    for (double lon = -110; lon <= 0; lon += 10) {
        QPointF top    = geoToScreen(m_latMax, lon);
        QPointF bottom = geoToScreen(m_latMin, lon);
        painter.drawLine(top, bottom);
    }
    // Latitude lines every 10 degrees
    for (double lat = 10; lat <= 60; lat += 10) {
        QPointF left  = geoToScreen(lat, m_lonMin);
        QPointF right = geoToScreen(lat, m_lonMax);
        painter.drawLine(left, right);
    }

    // Label the grid
    painter.setPen(QColor(255,255,255,60));
    painter.setFont(QFont("Arial", 8));
    for (double lon = -100; lon <= -10; lon += 10) {
        QPointF p = geoToScreen(m_latMin + 2, lon);
        painter.drawText(p, QString::number((int)lon) + "°");
    }
    for (double lat = 10; lat <= 60; lat += 10) {
        QPointF p = geoToScreen(lat, m_lonMin + 1);
        painter.drawText(p, QString::number((int)lat) + "°N");
    }

    if (m_points.isEmpty()) {
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 16));
        painter.drawText(rect(), Qt::AlignCenter, "Load a hurricane CSV to begin");
        return;
    }

    // --- Draw edges (path lines) ---
    for (int i = 1; i < m_points.size(); ++i) {
        const auto &prev = m_points[i-1];
        const auto &curr = m_points[i];

        QPointF p1 = geoToScreen(prev.lat, prev.lon);
        QPointF p2 = geoToScreen(curr.lat, curr.lon);

        // Color the edge by the destination point's category
        QColor edgeColor(HurricanePoint::categoryColor(curr.category));
        edgeColor.setAlpha(180);
        painter.setPen(QPen(edgeColor, 2.0));
        painter.drawLine(p1, p2);
    }

    // --- Draw vertices (data points) ---
    for (const auto &pt : m_points) {
        QPointF pos = geoToScreen(pt.lat, pt.lon);

        QColor fillColor(HurricanePoint::categoryColor(pt.category));
        double radius = 5.0 * m_scale;
        radius = qBound(3.0, radius, 12.0);

        // Glow effect
        QColor glow = fillColor;
        glow.setAlpha(60);
        painter.setBrush(glow);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(pos, radius * 1.8, radius * 1.8);

        // Main dot
        painter.setBrush(fillColor);
        painter.setPen(QPen(Qt::white, 1));
        painter.drawEllipse(pos, radius, radius);
    }

    // --- Draw legend ---
    drawLegend(painter);
}

void MapWidget::drawLegend(QPainter &painter) {
    struct LegendEntry { int cat; QString label; };
    static const QList<LegendEntry> entries = {
        {-1, "Tropical Depression"},
        { 0, "Tropical Storm"},
        { 1, "Category 1"},
        { 2, "Category 2"},
        { 3, "Category 3"},
        { 4, "Category 4"},
        { 5, "Category 5"},
    };

    int boxX = 12, boxY = 12;
    int lineH = 22;
    int boxW  = 190;
    int boxH  = entries.size() * lineH + 24;

    painter.setBrush(QColor(0,0,0,160));
    painter.setPen(QColor(255,255,255,80));
    painter.drawRoundedRect(boxX, boxY, boxW, boxH, 8, 8);

    painter.setFont(QFont("Arial", 9, QFont::Bold));
    painter.setPen(Qt::white);
    painter.drawText(boxX + 10, boxY + 16, "Hurricane Category");

    painter.setFont(QFont("Arial", 9));
    for (int i = 0; i < entries.size(); ++i) {
        int y = boxY + 24 + i * lineH;
        QColor c(HurricanePoint::categoryColor(entries[i].cat));

        // Colored dot
        painter.setBrush(c);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(boxX + 12, y + 4, 10, 10);

        // Label
        painter.setPen(Qt::white);
        painter.drawText(boxX + 30, y + 13, entries[i].label);
    }
}

void MapWidget::resizeEvent(QResizeEvent *) { update(); }

void MapWidget::wheelEvent(QWheelEvent *event) {
    double factor = (event->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
    QPointF cursor = event->position();

    // Zoom toward cursor
    m_offset = cursor + (m_offset - cursor) * factor;
    m_scale  *= factor;
    m_scale   = qBound(0.5, m_scale, 20.0);
    update();
}

void MapWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging    = true;
        m_lastMousePos = event->pos();
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_offset    += delta;
        m_lastMousePos = event->pos();
        update();
    }
}

void MapWidget::mouseReleaseEvent(QMouseEvent *) { m_dragging = false; }
