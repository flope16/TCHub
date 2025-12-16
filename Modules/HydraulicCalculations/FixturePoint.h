#pragma once

#include <QGraphicsItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QPen>
#include <QBrush>
#include "PipeCalculator.h"

// Représentation graphique d'un point de puisage d'appareil sanitaire
class FixturePoint : public QGraphicsItemGroup
{
public:
    FixturePoint(HydraulicCalc::FixtureType type, int quantity = 1, QGraphicsItem* parent = nullptr);
    ~FixturePoint();

    // Accesseurs
    HydraulicCalc::FixtureType getType() const { return fixtureType; }
    int getQuantity() const { return quantity; }
    void setQuantity(int qty);

    // Position sur le tronçon
    void setPositionOnSegment(const QPointF& position);
    QPointF getPositionOnSegment() const { return position; }

    // Mise à jour de l'affichage
    void updateDisplay();
    void setHighlighted(bool highlighted);

    // Conversion en structure Fixture pour calculs
    HydraulicCalc::Fixture toFixture() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    void createVisuals();
    QString getFixtureIcon() const;
    QColor getFixtureColor() const;

    // Données
    HydraulicCalc::FixtureType fixtureType;
    int quantity;
    QPointF position;

    // Éléments visuels
    QGraphicsEllipseItem* backgroundCircle;
    QGraphicsTextItem* iconLabel;       // Icône emoji
    QGraphicsTextItem* quantityLabel;   // Badge de quantité
    QGraphicsTextItem* tooltipLabel;    // Info-bulle

    // État
    bool isHighlighted;
};
