#include "GraphicPipeSegment.h"
#include "FixturePoint.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <cmath>

GraphicPipeSegment::GraphicPipeSegment(HydraulicCalc::NetworkSegment* segment, QGraphicsItem* parent)
    : QGraphicsItemGroup(parent)
    , segmentData(segment)
    , startPoint(0, 0)
    , endPoint(0, 100)
    , pipeLineAller(nullptr)
    , pipeLineRetour(nullptr)
    , startCircle(nullptr)
    , endCircle(nullptr)
    , labelBackground(nullptr)
    , nameLabel(nullptr)
    , resultsLabel(nullptr)
    , dimensionsLabel(nullptr)
    , isHighlighted(false)
    , hasReturn(false)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    createVisuals();
    updateDisplay();
}

GraphicPipeSegment::~GraphicPipeSegment()
{
    // Les items graphiques sont automatiquement supprimés par Qt
}

void GraphicPipeSegment::setStartPoint(const QPointF& point)
{
    startPoint = point;
    updateGeometry();
}

void GraphicPipeSegment::setEndPoint(const QPointF& point)
{
    endPoint = point;
    updateGeometry();
}

void GraphicPipeSegment::updateGeometry()
{
    updatePipeVisual();
    updateLabels();
}

void GraphicPipeSegment::createVisuals()
{
    // Ligne principale (aller)
    pipeLineAller = new QGraphicsLineItem(this);
    QPen allerPen(QColor("#4472C4"), 6);
    allerPen.setCapStyle(Qt::RoundCap);
    pipeLineAller->setPen(allerPen);
    addToGroup(pipeLineAller);

    // Ligne retour (initialement invisible)
    pipeLineRetour = new QGraphicsLineItem(this);
    QPen retourPen(QColor("#70AD47"), 4);
    retourPen.setCapStyle(Qt::RoundCap);
    retourPen.setStyle(Qt::DashLine);
    pipeLineRetour->setPen(retourPen);
    pipeLineRetour->setVisible(false);
    addToGroup(pipeLineRetour);

    // Cercles de début et fin
    startCircle = new QGraphicsEllipseItem(-6, -6, 12, 12, this);
    startCircle->setBrush(QBrush(QColor("#4472C4")));
    startCircle->setPen(QPen(Qt::white, 2));
    addToGroup(startCircle);

    endCircle = new QGraphicsEllipseItem(-6, -6, 12, 12, this);
    endCircle->setBrush(QBrush(QColor("#4472C4")));
    endCircle->setPen(QPen(Qt::white, 2));
    addToGroup(endCircle);

    // Fond du label
    labelBackground = new QGraphicsRectItem(this);
    labelBackground->setBrush(QBrush(QColor(255, 255, 255, 230)));
    labelBackground->setPen(QPen(QColor("#bdc3c7"), 1));
    addToGroup(labelBackground);

    // Label du nom
    nameLabel = new QGraphicsTextItem(this);
    QFont nameFont("Arial", 9, QFont::Bold);
    nameLabel->setFont(nameFont);
    nameLabel->setDefaultTextColor(QColor("#2c3e50"));
    addToGroup(nameLabel);

    // Label des résultats
    resultsLabel = new QGraphicsTextItem(this);
    QFont resultsFont("Courier New", 8);
    resultsLabel->setFont(resultsFont);
    resultsLabel->setDefaultTextColor(QColor("#2c3e50"));
    addToGroup(resultsLabel);

    // Label des dimensions
    dimensionsLabel = new QGraphicsTextItem(this);
    QFont dimFont("Arial", 7);
    dimensionsLabel->setFont(dimFont);
    dimensionsLabel->setDefaultTextColor(QColor("#6c757d"));
    addToGroup(dimensionsLabel);
}

void GraphicPipeSegment::updatePipeVisual()
{
    if (!pipeLineAller) return;

    // Mise à jour de la ligne principale
    pipeLineAller->setLine(startPoint.x(), startPoint.y(), endPoint.x(), endPoint.y());

    // Position des cercles
    startCircle->setPos(startPoint);
    endCircle->setPos(endPoint);

    // Si retour de bouclage, afficher la ligne retour décalée
    if (hasReturn && pipeLineRetour) {
        // Calculer un décalage perpendiculaire à la ligne principale
        QPointF direction = endPoint - startPoint;
        double length = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());
        if (length > 0) {
            QPointF unitDir = direction / length;
            QPointF perpendicular(-unitDir.y(), unitDir.x());
            double offset = 8.0; // Décalage en pixels

            QPointF startRetour = startPoint + perpendicular * offset;
            QPointF endRetour = endPoint + perpendicular * offset;

            pipeLineRetour->setLine(startRetour.x(), startRetour.y(), endRetour.x(), endRetour.y());
            pipeLineRetour->setVisible(true);
        }
    } else {
        pipeLineRetour->setVisible(false);
    }
}

void GraphicPipeSegment::updateLabels()
{
    if (!segmentData) return;

    // Position du label (à côté du segment)
    QPointF midPoint = (startPoint + endPoint) / 2.0;
    QPointF direction = endPoint - startPoint;
    double length = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());

    if (length > 0) {
        QPointF unitDir = direction / length;
        QPointF perpendicular(-unitDir.y(), unitDir.x());
        QPointF labelPos = midPoint + perpendicular * 20.0; // Décalage latéral

        // Nom du segment
        nameLabel->setPlainText(QString::fromStdString(segmentData->name));
        nameLabel->setPos(labelPos);

        // Dimensions (longueur et hauteur)
        QString dimText = QString("L=%1m H=%2m")
            .arg(segmentData->length, 0, 'f', 1)
            .arg(segmentData->heightDifference, 0, 'f', 1);
        dimensionsLabel->setPlainText(dimText);
        dimensionsLabel->setPos(labelPos.x(), labelPos.y() + 15);

        // Résultats (si calculés)
        resultsLabel->setPos(labelPos.x(), labelPos.y() + 30);

        // Ajuster le fond du label
        QRectF boundingRect = nameLabel->boundingRect().united(dimensionsLabel->boundingRect()).united(resultsLabel->boundingRect());
        labelBackground->setRect(labelPos.x() - 5, labelPos.y() - 2,
                                boundingRect.width() + 10, boundingRect.height() + 50);
    }
}

void GraphicPipeSegment::updateDisplay()
{
    updatePipeVisual();
    updateLabels();
}

void GraphicPipeSegment::updateResultsDisplay()
{
    if (!segmentData || !resultsLabel) return;

    const auto& result = segmentData->result;

    // Vérifier si le segment a été calculé
    if (result.nominalDiameter == 0) {
        resultsLabel->setPlainText("(Non calculé)");
        resultsLabel->setDefaultTextColor(QColor("#95a5a6"));
        hasReturn = false;
        updatePipeVisual();
        return;
    }

    // Construire le texte des résultats
    QString resultsText;
    resultsText += QString("DN %1 (\u00D8%2mm)\n")
        .arg(result.nominalDiameter)
        .arg(result.actualDiameter, 0, 'f', 1);

    resultsText += QString("Q=%1 L/min\n")
        .arg(result.flowRate, 0, 'f', 1);

    resultsText += QString("V=%1 m/s\n")
        .arg(result.velocity, 0, 'f', 2);

    resultsText += QString("DP=%1 mCE\n")
        .arg(result.pressureDrop, 0, 'f', 2);

    // Si retour de bouclage
    if (result.hasReturn) {
        resultsText += QString("\nRetour: DN %1\n")
            .arg(result.returnNominalDiameter);
        resultsText += QString("Qr=%1 L/min")
            .arg(result.returnFlowRate, 0, 'f', 1);
        hasReturn = true;
    } else {
        hasReturn = false;
    }

    resultsLabel->setPlainText(resultsText);

    // Couleur selon les alertes
    if (result.velocity > 2.0 || result.velocity < 0.3) {
        resultsLabel->setDefaultTextColor(QColor("#e74c3c"));  // Rouge si problème
    } else {
        resultsLabel->setDefaultTextColor(QColor("#27ae60"));  // Vert si OK
    }

    updatePipeVisual();
    updateLabels();
}

void GraphicPipeSegment::setHighlighted(bool highlighted)
{
    isHighlighted = highlighted;

    if (isHighlighted) {
        QPen highlightPen(QColor("#f39c12"), 8);
        highlightPen.setCapStyle(Qt::RoundCap);
        pipeLineAller->setPen(highlightPen);
    } else {
        QPen normalPen(getSegmentColor(), 6);
        normalPen.setCapStyle(Qt::RoundCap);
        pipeLineAller->setPen(normalPen);
    }
}

QColor GraphicPipeSegment::getSegmentColor() const
{
    // Couleur selon l'état
    if (!segmentData) return QColor("#95a5a6");

    if (segmentData->result.nominalDiameter == 0) {
        return QColor("#95a5a6");  // Gris si non calculé
    }

    if (segmentData->result.velocity > 2.0 || segmentData->result.velocity < 0.3) {
        return QColor("#e74c3c");  // Rouge si problème
    }

    return QColor("#4472C4");  // Bleu si OK
}

void GraphicPipeSegment::addFixturePoint(FixturePoint* fixture)
{
    if (fixture) {
        fixturePoints.push_back(fixture);
    }
}

void GraphicPipeSegment::removeFixturePoint(FixturePoint* fixture)
{
    auto it = std::find(fixturePoints.begin(), fixturePoints.end(), fixture);
    if (it != fixturePoints.end()) {
        fixturePoints.erase(it);
    }
}

bool GraphicPipeSegment::containsPoint(const QPointF& point, double tolerance) const
{
    // Calculer la distance du point à la ligne
    QPointF v = endPoint - startPoint;
    QPointF w = point - startPoint;

    double c1 = w.x() * v.x() + w.y() * v.y();
    if (c1 <= 0) {
        double dx = point.x() - startPoint.x();
        double dy = point.y() - startPoint.y();
        return std::sqrt(dx * dx + dy * dy) <= tolerance;
    }

    double c2 = v.x() * v.x() + v.y() * v.y();
    if (c1 >= c2) {
        double dx = point.x() - endPoint.x();
        double dy = point.y() - endPoint.y();
        return std::sqrt(dx * dx + dy * dy) <= tolerance;
    }

    double b = c1 / c2;
    QPointF pb = startPoint + v * b;
    double dx = point.x() - pb.x();
    double dy = point.y() - pb.y();
    return std::sqrt(dx * dx + dy * dy) <= tolerance;
}

QVariant GraphicPipeSegment::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemSelectedChange) {
        setHighlighted(value.toBool());
    }
    return QGraphicsItemGroup::itemChange(change, value);
}

void GraphicPipeSegment::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItemGroup::mousePressEvent(event);
}

void GraphicPipeSegment::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItemGroup::mouseReleaseEvent(event);
}
