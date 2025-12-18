#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QToolButton>
#include <QButtonGroup>
#include <QScrollArea>
#include <QEvent>
#include <vector>
#include "PipeCalculator.h"
#include "HydraulicSchemaView.h"
#include "GraphicPipeSegment.h"
#include "FixturePoint.h"

// Filtre pour désactiver la molette sur les SpinBox
class SpinBoxWheelFilter : public QObject
{
    Q_OBJECT
public:
    explicit SpinBoxWheelFilter(QObject* parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::Wheel) {
            return true;  // Bloquer l'événement de molette
        }
        return QObject::eventFilter(obj, event);
    }
};

class HydraulicCalculationsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit HydraulicCalculationsWindow(QWidget *parent = nullptr);
    ~HydraulicCalculationsWindow();

protected:
    // Surcharge pour désactiver la fermeture par Échap
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    // Changements de paramètres
    void onNetworkTypeChanged(int index);

    // Modes d'interaction
    void onSelectModeActivated();
    void onAddSegmentModeActivated();
    void onAddFixtureModeActivated(HydraulicCalc::FixtureType type);

    // Gestion des segments
    void onSegmentDrawingComplete(const QPointF& start, const QPointF& end);
    void onSegmentAdded(GraphicPipeSegment* segment);
    void onSegmentSelected(GraphicPipeSegment* segment);
    void onSegmentRemoved(GraphicPipeSegment* segment);
    void onEditSelectedSegment();
    void onDeleteSelectedSegment();

    // Gestion des fixtures
    void onFixtureAdded(FixturePoint* fixture, GraphicPipeSegment* segment);
    void onFixtureSelected(FixturePoint* fixture);
    void onFixtureRemoved(FixturePoint* fixture);
    void onEditSelectedFixture();
    void onDeleteSelectedFixture();

    // Actions principales
    void onCalculate();
    void onExportPDF();
    void onClear();
    void onResetView();

private:
    void setupUi();
    void applyStyle();
    void createToolsPanel();
    void createParametersPanel();
    void createFixturePalette();
    void createSchemaView();

    // Dialogues
    bool showSegmentDialog(HydraulicCalc::NetworkSegment& segment, bool isEdit = false);
    bool showFixtureDialog(FixturePoint* fixture);

    // Calculs
    void performCalculations();
    void updateNetworkSegmentsData();

    // Helpers pour rechercher les segments par ID (évite les pointeurs invalides)
    HydraulicCalc::NetworkSegment* findSegmentById(const std::string& id);
    const HydraulicCalc::NetworkSegment* findSegmentById(const std::string& id) const;

    // Export
    QString generatePDFHtml();

    // Layout principal
    QHBoxLayout *mainLayout;

    // Panneau latéral gauche
    QWidget *leftPanel;
    QVBoxLayout *leftPanelLayout;
    QScrollArea *leftScrollArea;

    // Vue schéma (droite)
    HydraulicSchemaView *schemaView;

    // Paramètres généraux
    QGroupBox *parametersGroup;
    QComboBox *networkTypeCombo;
    QComboBox *materialCombo;
    QDoubleSpinBox *supplyPressureSpin;
    QDoubleSpinBox *requiredPressureSpin;

    // Paramètres bouclage ECS
    QGroupBox *loopParametersGroup;
    QDoubleSpinBox *waterTempSpin;
    QDoubleSpinBox *ambientTempSpin;
    QDoubleSpinBox *insulationSpin;

    // Outils
    QGroupBox *toolsGroup;
    QButtonGroup *toolsButtonGroup;
    QToolButton *selectToolButton;
    QToolButton *addSegmentToolButton;
    QToolButton *panToolButton;

    // Palette d'appareils
    QGroupBox *fixturesGroup;
    std::vector<QPushButton*> fixtureButtons;

    // Actions d'édition
    QGroupBox *editGroup;
    QPushButton *editButton;
    QPushButton *deleteButton;

    // Boutons d'action
    QPushButton *calculateButton;
    QPushButton *exportButton;
    QPushButton *resetViewButton;
    QPushButton *clearButton;
    QPushButton *closeButton;

    // Données
    std::vector<HydraulicCalc::NetworkSegment> networkSegments;
    HydraulicCalc::PipeCalculator calculator;

    // État
    GraphicPipeSegment* currentSelectedSegment;
    FixturePoint* currentSelectedFixture;
    bool hasCalculated;

    // Filtre pour bloquer la molette sur les spinbox
    SpinBoxWheelFilter* wheelFilter;
};
