#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QWheelEvent>
#include <vector>
#include "GraphicPipeSegment.h"
#include "FixturePoint.h"
#include "PipeCalculator.h"

// Modes d'interaction
enum class InteractionMode {
    Select,         // Mode sélection
    AddSegment,     // Mode ajout de tronçon
    AddFixture,     // Mode ajout de point de puisage
    Pan             // Mode déplacement de la vue
};

// Vue graphique pour dessiner et manipuler le schéma hydraulique
class HydraulicSchemaView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit HydraulicSchemaView(QWidget* parent = nullptr);
    ~HydraulicSchemaView();

    // Gestion du mode d'interaction
    void setInteractionMode(InteractionMode mode);
    InteractionMode getInteractionMode() const { return currentMode; }

    // Gestion des segments
    GraphicPipeSegment* addSegment(HydraulicCalc::NetworkSegment* segmentData, const QPointF& start, const QPointF& end);
    void removeSegment(GraphicPipeSegment* segment);
    void clearAllSegments();
    std::vector<GraphicPipeSegment*>& getSegments() { return segments; }

    // Gestion des fixtures (appareil à placer)
    void setFixtureToPlace(HydraulicCalc::FixtureType type, int quantity = 1);

    // Sélection
    GraphicPipeSegment* getSelectedSegment() const;
    FixturePoint* getSelectedFixture() const;

    // Mise à jour de l'affichage
    void updateAllSegments();
    void updateSegmentResults();

    // Réinitialisation de la vue
    void resetView();

    // Accès à la scène
    QGraphicsScene* getScene() { return scene; }

signals:
    void segmentAdded(GraphicPipeSegment* segment);
    void segmentSelected(GraphicPipeSegment* segment);
    void segmentRemoved(GraphicPipeSegment* segment);
    void fixtureAdded(FixturePoint* fixture, GraphicPipeSegment* segment);
    void fixtureSelected(FixturePoint* fixture);
    void fixtureRemoved(FixturePoint* fixture);
    void modeChanged(InteractionMode mode);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupScene();
    void handleSelectMode(QMouseEvent* event);
    void handleAddSegmentMode(QMouseEvent* event);
    void handleAddFixtureMode(QMouseEvent* event);
    void handlePanMode(QMouseEvent* event);

    GraphicPipeSegment* findSegmentAt(const QPointF& scenePos);
    void drawTemporaryLine(const QPointF& start, const QPointF& end);
    void clearTemporaryLine();

    // Scène graphique
    QGraphicsScene* scene;

    // Segments et fixtures
    std::vector<GraphicPipeSegment*> segments;

    // Mode d'interaction
    InteractionMode currentMode;

    // Pour l'ajout de segments
    bool isDrawingSegment;
    QPointF segmentStartPoint;
    QGraphicsLineItem* temporaryLine;

    // Pour l'ajout de fixtures
    HydraulicCalc::FixtureType fixtureTypeToPlace;
    int fixtureQuantityToPlace;

    // Pour le pan (déplacement de la vue)
    bool isPanning;
    QPoint lastPanPoint;
};
