#include "FixturePoint.h"
#include <QGraphicsSceneMouseEvent>
#include <QFont>

FixturePoint::FixturePoint(HydraulicCalc::FixtureType type, int qty, QGraphicsItem* parent)
    : QGraphicsItemGroup(parent)
    , fixtureType(type)
    , quantity(qty)
    , position(0, 0)
    , backgroundCircle(nullptr)
    , iconLabel(nullptr)
    , quantityLabel(nullptr)
    , tooltipLabel(nullptr)
    , isHighlighted(false)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);  // Pas déplaçable pour l'instant
    setAcceptHoverEvents(true);

    createVisuals();
    updateDisplay();
}

FixturePoint::~FixturePoint()
{
    // Les items graphiques sont automatiquement supprimés par Qt
}

void FixturePoint::setQuantity(int qty)
{
    if (qty > 0) {
        quantity = qty;
        updateDisplay();
    }
}

void FixturePoint::setPositionOnSegment(const QPointF& pos)
{
    position = pos;
    setPos(pos);
}

void FixturePoint::createVisuals()
{
    // Cercle de fond
    backgroundCircle = new QGraphicsEllipseItem(-15, -15, 30, 30, this);
    backgroundCircle->setBrush(QBrush(getFixtureColor()));
    backgroundCircle->setPen(QPen(Qt::white, 2));
    addToGroup(backgroundCircle);

    // Icône (emoji)
    iconLabel = new QGraphicsTextItem(this);
    QFont iconFont("Segoe UI Emoji", 14);
    iconLabel->setFont(iconFont);
    iconLabel->setPlainText(getFixtureIcon());
    iconLabel->setPos(-8, -12);  // Centrage approximatif
    addToGroup(iconLabel);

    // Badge de quantité (si > 1)
    quantityLabel = new QGraphicsTextItem(this);
    QFont qtyFont("Arial", 8, QFont::Bold);
    quantityLabel->setFont(qtyFont);
    quantityLabel->setDefaultTextColor(Qt::white);
    addToGroup(quantityLabel);

    // Tooltip (info-bulle, initialement invisible)
    tooltipLabel = new QGraphicsTextItem(this);
    QFont tooltipFont("Arial", 8);
    tooltipLabel->setFont(tooltipFont);
    tooltipLabel->setDefaultTextColor(QColor("#2c3e50"));
    tooltipLabel->setVisible(false);
    addToGroup(tooltipLabel);
}

void FixturePoint::updateDisplay()
{
    // Mise à jour du cercle de fond
    backgroundCircle->setBrush(QBrush(getFixtureColor()));

    // Mise à jour de l'icône
    iconLabel->setPlainText(getFixtureIcon());

    // Affichage de la quantité si > 1
    if (quantity > 1) {
        quantityLabel->setPlainText(QString::number(quantity));
        quantityLabel->setVisible(true);
        // Position du badge (coin supérieur droit)
        quantityLabel->setPos(10, -18);
    } else {
        quantityLabel->setVisible(false);
    }

    // Mise à jour du tooltip
    HydraulicCalc::Fixture fixture = toFixture();
    QString tooltipText = QString("%1\nQté: %2\nDébit: %3 L/min")
        .arg(QString::fromStdString(HydraulicCalc::PipeCalculator::getFixtureName(fixtureType)))
        .arg(quantity)
        .arg(fixture.flowRate * quantity, 0, 'f', 1);
    tooltipLabel->setPlainText(tooltipText);
    tooltipLabel->setPos(20, -20);
}

QString FixturePoint::getFixtureIcon() const
{
    switch (fixtureType) {
        case HydraulicCalc::FixtureType::WashBasin:
            return "🚰";
        case HydraulicCalc::FixtureType::Sink:
            return "🧽";
        case HydraulicCalc::FixtureType::Shower:
            return "🚿";
        case HydraulicCalc::FixtureType::Bathtub:
            return "🛁";
        case HydraulicCalc::FixtureType::WC:
            return "🚽";
        case HydraulicCalc::FixtureType::Bidet:
            return "🚰";
        case HydraulicCalc::FixtureType::WashingMachine:
            return "🧺";
        case HydraulicCalc::FixtureType::Dishwasher:
            return "🍽️";
        case HydraulicCalc::FixtureType::UrinalFlush:
            return "🚹";
        case HydraulicCalc::FixtureType::UrinalContinuous:
            return "🚹";
        default:
            return "💧";
    }
}

QColor FixturePoint::getFixtureColor() const
{
    // Couleurs selon le type d'appareil
    switch (fixtureType) {
        case HydraulicCalc::FixtureType::Shower:
        case HydraulicCalc::FixtureType::Bathtub:
            return QColor("#3498db");  // Bleu pour salle de bain
        case HydraulicCalc::FixtureType::Sink:
        case HydraulicCalc::FixtureType::Dishwasher:
            return QColor("#e67e22");  // Orange pour cuisine
        case HydraulicCalc::FixtureType::WC:
        case HydraulicCalc::FixtureType::UrinalFlush:
        case HydraulicCalc::FixtureType::UrinalContinuous:
            return QColor("#9b59b6");  // Violet pour WC
        case HydraulicCalc::FixtureType::WashingMachine:
            return QColor("#1abc9c");  // Turquoise pour électroménager
        default:
            return QColor("#95a5a6");  // Gris par défaut
    }
}

void FixturePoint::setHighlighted(bool highlighted)
{
    isHighlighted = highlighted;

    if (isHighlighted) {
        backgroundCircle->setPen(QPen(QColor("#f39c12"), 3));
    } else {
        backgroundCircle->setPen(QPen(Qt::white, 2));
    }
}

HydraulicCalc::Fixture FixturePoint::toFixture() const
{
    return HydraulicCalc::Fixture(fixtureType, quantity);
}

void FixturePoint::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItemGroup::mousePressEvent(event);
}

void FixturePoint::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItemGroup::mouseReleaseEvent(event);
}

void FixturePoint::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    // Afficher le tooltip
    tooltipLabel->setVisible(true);
    setHighlighted(true);
    QGraphicsItemGroup::hoverEnterEvent(event);
}

void FixturePoint::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    // Masquer le tooltip
    tooltipLabel->setVisible(false);
    if (!isSelected()) {
        setHighlighted(false);
    }
    QGraphicsItemGroup::hoverLeaveEvent(event);
}
