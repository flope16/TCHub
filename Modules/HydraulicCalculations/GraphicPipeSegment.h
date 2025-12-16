#pragma once

#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QPen>
#include <QBrush>
#include <QFont>
#include "PipeCalculator.h"

// Déclaration anticipée
class FixturePoint;

// Représentation graphique d'un tronçon de canalisation
class GraphicPipeSegment : public QGraphicsItemGroup
{
public:
    GraphicPipeSegment(HydraulicCalc::NetworkSegment* segment, QGraphicsItem* parent = nullptr);
    ~GraphicPipeSegment();

    // Accesseurs
    HydraulicCalc::NetworkSegment* getSegmentData() { return segmentData; }
    QPointF getStartPoint() const { return startPoint; }
    QPointF getEndPoint() const { return endPoint; }

    // Configuration de la géométrie
    void setStartPoint(const QPointF& point);
    void setEndPoint(const QPointF& point);
    void updateGeometry();

    // Gestion des points de puisage
    void addFixturePoint(FixturePoint* fixture);
    void removeFixturePoint(FixturePoint* fixture);
    std::vector<FixturePoint*>& getFixturePoints() { return fixturePoints; }

    // Mise à jour de l'affichage
    void updateDisplay();
    void updateResultsDisplay();
    void setHighlighted(bool highlighted);

    // Vérification si un point est sur le segment
    bool containsPoint(const QPointF& point, double tolerance = 10.0) const;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void createVisuals();
    void updatePipeVisual();
    void updateLabels();
    void updateJunctionPoints();
    QColor getSegmentColor() const;

    // Données du segment
    HydraulicCalc::NetworkSegment* segmentData;

    // Position
    QPointF startPoint;
    QPointF endPoint;

    // Éléments visuels
    QGraphicsLineItem* pipeLineAller;       // Ligne principale (aller)
    QGraphicsLineItem* pipeLineRetour;      // Ligne retour (si bouclage)
    QGraphicsEllipseItem* startCircle;      // Cercle de début
    QGraphicsEllipseItem* endCircle;        // Cercle de fin
    QGraphicsRectItem* labelBackground;     // Fond du label
    QGraphicsTextItem* nameLabel;           // Nom du segment
    QGraphicsTextItem* resultsLabel;        // Résultats (DN, vitesse, etc.)
    QGraphicsTextItem* dimensionsLabel;     // Longueur et hauteur
    std::vector<QGraphicsEllipseItem*> junctionCircles;  // Cercles de jonction pour les fixtures

    // Points de puisage sur ce segment
    std::vector<FixturePoint*> fixturePoints;

    // État
    bool isHighlighted;
    bool hasReturn;  // Indique si le segment a un retour de bouclage
};
