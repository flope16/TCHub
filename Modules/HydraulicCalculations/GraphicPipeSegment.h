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
    std::string getSegmentId() const { return segmentId; }
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

    // Mise à jour de l'affichage (nécessite les données du segment pour éviter les pointeurs invalides)
    void updateDisplay(const HydraulicCalc::NetworkSegment* segmentData = nullptr);
    void updateResultsDisplay(const HydraulicCalc::NetworkSegment* segmentData = nullptr);
    void setHighlighted(bool highlighted);

    // Gestion du statut de tronçon principal
    bool isMainSegment(const HydraulicCalc::NetworkSegment* segmentData = nullptr) const;

    // Vérification si un point est sur le segment
    bool containsPoint(const QPointF& point, double tolerance = 10.0) const;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void createVisuals();
    void updatePipeVisual(const HydraulicCalc::NetworkSegment* segmentData = nullptr);
    void updateLabels(const HydraulicCalc::NetworkSegment* segmentData = nullptr);
    void updateJunctionPoints();
    void updateMainSegmentDisplay(const HydraulicCalc::NetworkSegment* segmentData = nullptr);
    QColor getSegmentColor() const;

    // Données du segment (stockage par ID pour éviter les pointeurs invalides lors de réallocation du vector)
    std::string segmentId;

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
    QGraphicsTextItem* mainBadge;           // Badge "PRINCIPAL" pour les segments racines
    QGraphicsRectItem* mainBadgeBg;         // Fond du badge principal
    std::vector<QGraphicsEllipseItem*> junctionCircles;  // Cercles de jonction pour les fixtures

    // Points de puisage sur ce segment
    std::vector<FixturePoint*> fixturePoints;

    // État
    bool isHighlighted;
    bool hasReturn;  // Indique si le segment a un retour de bouclage
};
