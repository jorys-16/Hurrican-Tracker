#pragma once
#include <QWidget>
#include <QList>
#include <QImage>
#include "hurricanedata.h"

class MapWidget : public QWidget {
    Q_OBJECT
public:
    explicit MapWidget(QWidget *parent = nullptr);
    void setData(const QList<HurricanePoint> &points);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QList<HurricanePoint> m_points;

    // Pan & zoom state
    double  m_scale;
    QPointF m_offset;
    QPoint  m_lastMousePos;
    bool    m_dragging;

    // Map bounds (in lon/lat) for the flat-map projection
    // Atlantic basin view: lon -110..0, lat 5..65
    const double m_lonMin = -110.0;
    const double m_lonMax =    0.0;
    const double m_latMin =    5.0;
    const double m_latMax =   65.0;

    QPointF geoToScreen(double lat, double lon) const;
    void    drawLegend(QPainter &painter);
};
