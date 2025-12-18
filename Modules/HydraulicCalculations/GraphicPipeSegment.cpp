#include "GraphicPipeSegment.h"
#include "FixturePoint.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <cmath>

GraphicPipeSegment::GraphicPipeSegment(HydraulicCalc::NetworkSegment* segment, QGraphicsItem* parent)
    : QGraphicsItemGroup(parent)
    , segmentId(segment ? segment->id : "")
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
    , mainBadge(nullptr)
    , mainBadgeBg(nullptr)
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
    QPen allerPen(QColor("#3498db"), 6);  // Bleu moderne
    allerPen.setCapStyle(Qt::RoundCap);
    pipeLineAller->setPen(allerPen);
    addToGroup(pipeLineAller);

    // Ligne retour (initialement invisible)
    pipeLineRetour = new QGraphicsLineItem(this);
    QPen retourPen(QColor("#27ae60"), 4);  // Vert moderne
    retourPen.setCapStyle(Qt::RoundCap);
    retourPen.setStyle(Qt::DashLine);
    pipeLineRetour->setPen(retourPen);
    pipeLineRetour->setVisible(false);
    addToGroup(pipeLineRetour);

    // Cercles de début et fin
    startCircle = new QGraphicsEllipseItem(-6, -6, 12, 12, this);
    startCircle->setBrush(QBrush(QColor("#3498db")));
    startCircle->setPen(QPen(Qt::white, 2));
    addToGroup(startCircle);

    endCircle = new QGraphicsEllipseItem(-6, -6, 12, 12, this);
    endCircle->setBrush(QBrush(QColor("#3498db")));
    endCircle->setPen(QPen(Qt::white, 2));
    addToGroup(endCircle);

    // Fond du label
    labelBackground = new QGraphicsRectItem(this);
    labelBackground->setBrush(QBrush(QColor(255, 255, 255, 230)));
    labelBackground->setPen(QPen(QColor("#bdc3c7"), 1));
    addToGroup(labelBackground);

    // Label du nom
    nameLabel = new QGraphicsTextItem(this);
    QFont nameFont("Arial", 10, QFont::Bold);
    nameLabel->setFont(nameFont);
    nameLabel->setDefaultTextColor(QColor("#000000"));  // Noir
    addToGroup(nameLabel);

    // Label des résultats
    resultsLabel = new QGraphicsTextItem(this);
    QFont resultsFont("Courier New", 9);
    resultsLabel->setFont(resultsFont);
    resultsLabel->setDefaultTextColor(QColor("#000000"));  // Noir
    addToGroup(resultsLabel);

    // Label des dimensions
    dimensionsLabel = new QGraphicsTextItem(this);
    QFont dimFont("Arial", 8);
    dimensionsLabel->setFont(dimFont);
    dimensionsLabel->setDefaultTextColor(QColor("#555555"));  // Gris foncé
    addToGroup(dimensionsLabel);

    // Badge "PRINCIPAL" pour les segments racines
    mainBadge = new QGraphicsTextItem(this);
    QFont badgeFont("Arial", 9, QFont::Bold);
    mainBadge->setFont(badgeFont);
    mainBadge->setDefaultTextColor(QColor("#ffffff"));  // Blanc
    mainBadge->setVisible(false);
    addToGroup(mainBadge);
}

void GraphicPipeSegment::updatePipeVisual(const HydraulicCalc::NetworkSegment* segmentData)
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

void GraphicPipeSegment::updateLabels(const HydraulicCalc::NetworkSegment* segmentData)
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

        // Ajuster le fond du label : calculer la largeur et hauteur réelles sans utiliser united()
        // qui peut causer des problèmes avec des positions absolues
        double maxWidth = std::max({nameLabel->boundingRect().width(),
                                    dimensionsLabel->boundingRect().width(),
                                    resultsLabel->boundingRect().width()});

        // Hauteur totale : somme des hauteurs des 3 labels
        double totalHeight = nameLabel->boundingRect().height()
                           + dimensionsLabel->boundingRect().height()
                           + resultsLabel->boundingRect().height();

        // Définir la zone de fond avec padding constant
        labelBackground->setRect(labelPos.x() - 5, labelPos.y() - 2,
                                maxWidth + 10, totalHeight + 4);
    }
}

void GraphicPipeSegment::updateJunctionPoints()
{
    // Supprimer les anciens cercles de jonction
    for (auto* circle : junctionCircles) {
        removeFromGroup(circle);
        delete circle;
    }
    junctionCircles.clear();

    // Créer un cercle de jonction pour chaque fixture
    for (auto* fixture : fixturePoints) {
        if (fixture) {
            QPointF fixturePos = fixture->getPositionOnSegment();

            // Créer un petit cercle de jonction sur le segment
            QGraphicsEllipseItem* junctionCircle = new QGraphicsEllipseItem(-4, -4, 8, 8, this);
            junctionCircle->setPos(fixturePos);
            junctionCircle->setBrush(QBrush(QColor("#e74c3c")));  // Rouge vif pour visibilité
            junctionCircle->setPen(QPen(Qt::white, 2));
            junctionCircle->setZValue(10);  // Au-dessus du segment

            addToGroup(junctionCircle);
            junctionCircles.push_back(junctionCircle);
        }
    }
}

void GraphicPipeSegment::updateDisplay(const HydraulicCalc::NetworkSegment* segmentData)
{
    updatePipeVisual(segmentData);
    updateLabels(segmentData);
    updateJunctionPoints();
    updateMainSegmentDisplay(segmentData);
}

void GraphicPipeSegment::updateResultsDisplay(const HydraulicCalc::NetworkSegment* segmentData)
{
    if (!segmentData || !resultsLabel) return;

    const auto& result = segmentData->result;

    // Vérifier si le segment a été calculé
    if (result.nominalDiameter == 0) {
        resultsLabel->setPlainText("(Non calculé)");
        resultsLabel->setDefaultTextColor(QColor("#666666"));  // Gris foncé visible
        hasReturn = false;
        updatePipeVisual(segmentData);
        return;
    }

    // Construire le texte des résultats
    QString resultsText;
    resultsText += QString("DN %1 (D=%2mm)\n")
        .arg(result.nominalDiameter)
        .arg(result.actualDiameter, 0, 'f', 1);

    resultsText += QString("Q=%1 L/min\n")
        .arg(result.flowRate, 0, 'f', 1);

    resultsText += QString("V=%1 m/s\n")
        .arg(result.velocity, 0, 'f', 2);

    resultsText += QString("DP=%1 mCE")
        .arg(result.pressureDrop, 0, 'f', 2);

    // Afficher les températures pour ECS (avec ou sans bouclage)
    if (result.inletTemperature > 0.0 || result.outletTemperature > 0.0) {
        resultsText += QString("\nT in=%1°C / out=%2°C")
            .arg(result.inletTemperature, 0, 'f', 1)
            .arg(result.outletTemperature, 0, 'f', 1);
    }

    // Afficher les pertes thermiques si ECS (avec ou sans bouclage)
    if (result.heatLoss > 0.0) {
        resultsText += QString("\nPertes: %1 W")
            .arg(result.heatLoss, 0, 'f', 1);
    }

    // Si retour de bouclage
    if (result.hasReturn) {
        resultsText += QString("\n\nRetour: DN %1\n")
            .arg(result.returnNominalDiameter);
        resultsText += QString("Qr=%1 L/min\n")
            .arg(result.returnFlowRate, 0, 'f', 1);
        resultsText += QString("Vr=%1 m/s\n")
            .arg(result.returnVelocity, 0, 'f', 2);

        // Afficher températures retour (inlet/outlet)
        // RETOUR : flux inversé (enfant → parent)
        if (result.returnInletTemperature > 0.0 || result.returnOutletTemperature > 0.0) {
            resultsText += QString("T ret in=%1°C / out=%2°C")
                .arg(result.returnInletTemperature, 0, 'f', 1)
                .arg(result.returnOutletTemperature, 0, 'f', 1);
        } else {
            // Compatibilité ancien champ
            resultsText += QString("Tr=%1°C")
                .arg(result.returnTemperature, 0, 'f', 1);
        }

        hasReturn = true;
    } else {
        hasReturn = false;
    }

    resultsLabel->setPlainText(resultsText);

    // Couleur selon les alertes
    if (result.velocity > 2.0 || result.velocity < 0.3) {
        resultsLabel->setDefaultTextColor(QColor("#cc0000"));  // Rouge vif si problème
    } else {
        resultsLabel->setDefaultTextColor(QColor("#006600"));  // Vert foncé si OK
    }

    updatePipeVisual(segmentData);
    updateLabels(segmentData);
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
    // Couleur par défaut (sera mise à jour via updateResultsDisplay qui a accès aux données)
    // Cette méthode est conservée pour la compatibilité mais ne peut plus accéder aux données du segment
    return QColor("#3498db");  // Bleu moderne par défaut
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

QRectF GraphicPipeSegment::boundingRect() const
{
    // Retourner un bounding rect qui contient SEULEMENT le segment (ligne + cercles endpoints)
    // PAS les labels qui peuvent être loin
    const double margin = 15.0;  // Marge pour les cercles et la largeur de ligne

    double minX = std::min(startPoint.x(), endPoint.x()) - margin;
    double maxX = std::max(startPoint.x(), endPoint.x()) + margin;
    double minY = std::min(startPoint.y(), endPoint.y()) - margin;
    double maxY = std::max(startPoint.y(), endPoint.y()) + margin;

    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY));
}

QPainterPath GraphicPipeSegment::shape() const
{
    // Définir la forme cliquable : une zone autour du segment (ligne épaissie)
    QPainterPath path;

    // Créer un rectangle autour de la ligne du segment
    const double thickness = 10.0;  // Épaisseur de la zone cliquable

    QPointF segmentVector = endPoint - startPoint;
    double length = std::sqrt(segmentVector.x() * segmentVector.x() +
                             segmentVector.y() * segmentVector.y());

    if (length > 0) {
        // Vecteur perpendiculaire normalisé
        QPointF perpVector(-segmentVector.y() / length, segmentVector.x() / length);

        // Les 4 coins du rectangle autour du segment
        QPointF p1 = startPoint + perpVector * thickness;
        QPointF p2 = startPoint - perpVector * thickness;
        QPointF p3 = endPoint - perpVector * thickness;
        QPointF p4 = endPoint + perpVector * thickness;

        // Créer le chemin (rectangle)
        path.moveTo(p1);
        path.lineTo(p4);
        path.lineTo(p3);
        path.lineTo(p2);
        path.closeSubpath();
    }

    // Ajouter des cercles aux extrémités
    path.addEllipse(startPoint, thickness, thickness);
    path.addEllipse(endPoint, thickness, thickness);

    return path;
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

bool GraphicPipeSegment::isMainSegment(const HydraulicCalc::NetworkSegment* segmentData) const
{
    // Un segment est principal s'il n'a pas de parent
    return segmentData && segmentData->parentId.empty();
}

void GraphicPipeSegment::updateMainSegmentDisplay(const HydraulicCalc::NetworkSegment* segmentData)
{
    if (!mainBadge) return;

    if (isMainSegment(segmentData)) {
        // Ne plus afficher le badge "PRINCIPAL"
        mainBadge->setVisible(false);
        if (mainBadgeBg) {
            mainBadgeBg->setVisible(false);
        }

        // Rendre les cercles d'extrémité plus visibles pour le segment principal
        startCircle->setBrush(QBrush(QColor("#e74c3c")));
        startCircle->setPen(QPen(Qt::white, 3));
        startCircle->setRect(-8, -8, 16, 16);  // Plus grand

        endCircle->setBrush(QBrush(QColor("#e74c3c")));
        endCircle->setPen(QPen(Qt::white, 3));
        endCircle->setRect(-8, -8, 16, 16);  // Plus grand
    } else {
        // Masquer le badge pour les segments non-principaux
        mainBadge->setVisible(false);
        if (mainBadgeBg) {
            mainBadgeBg->setVisible(false);
        }

        // Restaurer la taille normale des cercles
        startCircle->setBrush(QBrush(QColor("#3498db")));
        startCircle->setPen(QPen(Qt::white, 2));
        startCircle->setRect(-6, -6, 12, 12);

        endCircle->setBrush(QBrush(QColor("#3498db")));
        endCircle->setPen(QPen(Qt::white, 2));
        endCircle->setRect(-6, -6, 12, 12);
    }
}
