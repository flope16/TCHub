#include "HydraulicSchemaView.h"
#include <QGraphicsLineItem>
#include <QKeyEvent>
#include <QScrollBar>
#include <cmath>

HydraulicSchemaView::HydraulicSchemaView(QWidget* parent)
    : QGraphicsView(parent)
    , scene(nullptr)
    , currentMode(InteractionMode::Select)
    , isDrawingSegment(false)
    , temporaryLine(nullptr)
    , fixtureTypeToPlace(HydraulicCalc::FixtureType::WashBasin)
    , fixtureQuantityToPlace(1)
    , isPanning(false)
{
    setupScene();

    // Configuration de la vue
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // Style de fond
    setBackgroundBrush(QBrush(QColor("#f8f9fa")));
}

HydraulicSchemaView::~HydraulicSchemaView()
{
    clearAllSegments();
    delete scene;
}

void HydraulicSchemaView::setupScene()
{
    scene = new QGraphicsScene(this);
    scene->setSceneRect(-2000, -2000, 4000, 4000);
    setScene(scene);

    // Grille de fond (optionnelle)
    scene->setBackgroundBrush(QBrush(QColor("#ffffff")));
}

void HydraulicSchemaView::setInteractionMode(InteractionMode mode)
{
    currentMode = mode;

    // Nettoyage selon le mode précédent
    if (isDrawingSegment) {
        clearTemporaryLine();
        isDrawingSegment = false;
    }

    // Configuration du curseur
    switch (currentMode) {
        case InteractionMode::Select:
            setCursor(Qt::ArrowCursor);
            setDragMode(QGraphicsView::RubberBandDrag);
            break;
        case InteractionMode::AddSegment:
            setCursor(Qt::CrossCursor);
            setDragMode(QGraphicsView::NoDrag);
            break;
        case InteractionMode::AddFixture:
            setCursor(Qt::PointingHandCursor);
            setDragMode(QGraphicsView::NoDrag);
            break;
        case InteractionMode::Pan:
            setCursor(Qt::OpenHandCursor);
            setDragMode(QGraphicsView::NoDrag);
            break;
    }

    emit modeChanged(currentMode);
}

GraphicPipeSegment* HydraulicSchemaView::addSegment(HydraulicCalc::NetworkSegment* segmentData,
                                                     const QPointF& start, const QPointF& end)
{
    if (!segmentData) return nullptr;

    GraphicPipeSegment* graphicSegment = new GraphicPipeSegment(segmentData);
    graphicSegment->setStartPoint(start);
    graphicSegment->setEndPoint(end);
    scene->addItem(graphicSegment);

    segments.push_back(graphicSegment);

    emit segmentAdded(graphicSegment);
    return graphicSegment;
}

void HydraulicSchemaView::removeSegment(GraphicPipeSegment* segment)
{
    if (!segment) return;

    // Retirer les fixtures du segment
    for (auto* fixture : segment->getFixturePoints()) {
        scene->removeItem(fixture);
        delete fixture;
    }

    // Retirer le segment
    auto it = std::find(segments.begin(), segments.end(), segment);
    if (it != segments.end()) {
        segments.erase(it);
    }

    scene->removeItem(segment);
    emit segmentRemoved(segment);
    delete segment;
}

void HydraulicSchemaView::clearAllSegments()
{
    for (auto* segment : segments) {
        // Retirer les fixtures
        for (auto* fixture : segment->getFixturePoints()) {
            scene->removeItem(fixture);
            delete fixture;
        }
        scene->removeItem(segment);
        delete segment;
    }
    segments.clear();
}

void HydraulicSchemaView::setFixtureToPlace(HydraulicCalc::FixtureType type, int quantity)
{
    fixtureTypeToPlace = type;
    fixtureQuantityToPlace = quantity;
}

GraphicPipeSegment* HydraulicSchemaView::getSelectedSegment() const
{
    QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    for (auto* item : selectedItems) {
        GraphicPipeSegment* segment = dynamic_cast<GraphicPipeSegment*>(item);
        if (segment) {
            return segment;
        }
    }
    return nullptr;
}

FixturePoint* HydraulicSchemaView::getSelectedFixture() const
{
    QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    for (auto* item : selectedItems) {
        FixturePoint* fixture = dynamic_cast<FixturePoint*>(item);
        if (fixture) {
            return fixture;
        }
    }
    return nullptr;
}

void HydraulicSchemaView::updateAllSegments()
{
    for (auto* segment : segments) {
        segment->updateDisplay();
    }
}

void HydraulicSchemaView::updateSegmentResults()
{
    for (auto* segment : segments) {
        segment->updateResultsDisplay();
    }
}

void HydraulicSchemaView::resetView()
{
    resetTransform();
    centerOn(0, 0);
}

void HydraulicSchemaView::mousePressEvent(QMouseEvent* event)
{
    switch (currentMode) {
        case InteractionMode::Select:
            handleSelectMode(event);
            break;
        case InteractionMode::AddSegment:
            handleAddSegmentMode(event);
            break;
        case InteractionMode::AddFixture:
            handleAddFixtureMode(event);
            break;
        case InteractionMode::Pan:
            handlePanMode(event);
            break;
    }

    QGraphicsView::mousePressEvent(event);
}

void HydraulicSchemaView::mouseMoveEvent(QMouseEvent* event)
{
    if (currentMode == InteractionMode::AddSegment && isDrawingSegment) {
        // Mise à jour de la ligne temporaire
        QPointF scenePos = mapToScene(event->pos());
        drawTemporaryLine(segmentStartPoint, scenePos);
    } else if (currentMode == InteractionMode::Pan && isPanning) {
        // Déplacement de la vue
        QPointF delta = mapToScene(event->pos()) - mapToScene(lastPanPoint);
        lastPanPoint = event->pos();

        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    }

    QGraphicsView::mouseMoveEvent(event);
}

void HydraulicSchemaView::mouseReleaseEvent(QMouseEvent* event)
{
    if (currentMode == InteractionMode::Pan && isPanning) {
        isPanning = false;
        setCursor(Qt::OpenHandCursor);
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void HydraulicSchemaView::wheelEvent(QWheelEvent* event)
{
    // Zoom avec la molette
    double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0) {
        scale(scaleFactor, scaleFactor);
    } else {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
}

void HydraulicSchemaView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        // Supprimer l'élément sélectionné
        GraphicPipeSegment* selectedSegment = getSelectedSegment();
        if (selectedSegment) {
            removeSegment(selectedSegment);
            return;
        }

        FixturePoint* selectedFixture = getSelectedFixture();
        if (selectedFixture) {
            // Retirer la fixture du segment parent
            for (auto* segment : segments) {
                segment->removeFixturePoint(selectedFixture);
            }
            scene->removeItem(selectedFixture);
            emit fixtureRemoved(selectedFixture);
            delete selectedFixture;
            return;
        }
    }

    QGraphicsView::keyPressEvent(event);
}

void HydraulicSchemaView::handleSelectMode(QMouseEvent* event)
{
    QPointF scenePos = mapToScene(event->pos());

    // Vérifier si on clique sur un segment
    GraphicPipeSegment* clickedSegment = findSegmentAt(scenePos);
    if (clickedSegment) {
        emit segmentSelected(clickedSegment);
    }

    // Vérifier si on clique sur une fixture
    QGraphicsItem* item = itemAt(event->pos());
    if (item) {
        FixturePoint* fixture = dynamic_cast<FixturePoint*>(item->parentItem());
        if (!fixture) {
            fixture = dynamic_cast<FixturePoint*>(item);
        }
        if (fixture) {
            emit fixtureSelected(fixture);
        }
    }
}

void HydraulicSchemaView::handleAddSegmentMode(QMouseEvent* event)
{
    QPointF scenePos = mapToScene(event->pos());

    if (!isDrawingSegment) {
        // Premier clic : début du segment
        segmentStartPoint = scenePos;
        isDrawingSegment = true;
        drawTemporaryLine(segmentStartPoint, scenePos);
    } else {
        // Deuxième clic : fin du segment
        clearTemporaryLine();
        isDrawingSegment = false;

        // Émettre le signal pour que la fenêtre parente crée le segment
        emit segmentDrawingComplete(segmentStartPoint, scenePos);
    }
}

void HydraulicSchemaView::handleAddFixtureMode(QMouseEvent* event)
{
    QPointF scenePos = mapToScene(event->pos());

    // Trouver le segment le plus proche
    GraphicPipeSegment* targetSegment = findSegmentAt(scenePos);
    if (targetSegment) {
        // Créer la fixture
        FixturePoint* fixture = new FixturePoint(fixtureTypeToPlace, fixtureQuantityToPlace);
        fixture->setPositionOnSegment(scenePos);
        scene->addItem(fixture);

        // Ajouter au segment
        targetSegment->addFixturePoint(fixture);

        // Ajouter aussi aux données du segment
        targetSegment->getSegmentData()->fixtures.push_back(fixture->toFixture());

        emit fixtureAdded(fixture, targetSegment);

        // Retourner en mode sélection
        setInteractionMode(InteractionMode::Select);
    }
}

void HydraulicSchemaView::handlePanMode(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        isPanning = true;
        lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

GraphicPipeSegment* HydraulicSchemaView::findSegmentAt(const QPointF& scenePos)
{
    // Chercher le segment le plus proche du point cliqué
    GraphicPipeSegment* closestSegment = nullptr;
    double closestDistance = 20.0;  // Tolérance en pixels

    for (auto* segment : segments) {
        if (segment->containsPoint(scenePos, closestDistance)) {
            closestSegment = segment;
        }
    }

    return closestSegment;
}

void HydraulicSchemaView::drawTemporaryLine(const QPointF& start, const QPointF& end)
{
    if (!temporaryLine) {
        temporaryLine = new QGraphicsLineItem();
        QPen pen(QColor("#95a5a6"), 2, Qt::DashLine);
        temporaryLine->setPen(pen);
        scene->addItem(temporaryLine);
    }
    temporaryLine->setLine(start.x(), start.y(), end.x(), end.y());
}

void HydraulicSchemaView::clearTemporaryLine()
{
    if (temporaryLine) {
        scene->removeItem(temporaryLine);
        delete temporaryLine;
        temporaryLine = nullptr;
    }
}
