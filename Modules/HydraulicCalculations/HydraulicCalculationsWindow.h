#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QCheckBox>
#include <vector>
#include "PipeCalculator.h"

class HydraulicCalculationsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit HydraulicCalculationsWindow(QWidget *parent = nullptr);
    ~HydraulicCalculationsWindow();

private slots:
    void onNetworkTypeChanged(int index);
    void onMultiSegmentModeChanged(Qt::CheckState state);
    void onAddSegment();
    void onEditSegment();
    void onRemoveSegment();
    void onSegmentSelectionChanged();
    void onAddFixture();
    void onRemoveFixture();
    void onCalculate();
    void onExportPDF();
    void onClearResults();

private:
    void setupUi();
    void applyStyle();
    void updateFixtureTable();
    void updateSegmentTable();
    void updateSegmentParametersVisibility();
    void displayResults(const HydraulicCalc::PipeSegmentResult& result);
    void displayMultiSegmentResults(const HydraulicCalc::NetworkCalculationParameters& networkParams);

    // Widgets principaux
    QTabWidget *tabWidget;

    // Onglet Paramètres
    QWidget *parametersTab;
    QCheckBox *multiSegmentCheckbox;
    QComboBox *networkTypeCombo;
    QComboBox *materialCombo;
    QDoubleSpinBox *supplyPressureSpin;
    QDoubleSpinBox *requiredPressureSpin;

    // Paramètres pour mode segment unique (masqués en mode multi-segment)
    QGroupBox *singleSegmentGroup;
    QDoubleSpinBox *lengthSpin;
    QDoubleSpinBox *heightDiffSpin;

    // Gestion multi-segments (masqués en mode segment unique)
    QGroupBox *multiSegmentGroup;
    QTableWidget *segmentsTable;
    QPushButton *addSegmentButton;
    QPushButton *editSegmentButton;
    QPushButton *removeSegmentButton;

    // Paramètres spécifiques ECS avec bouclage
    QGroupBox *loopParametersGroup;
    QDoubleSpinBox *loopLengthSpin;
    QDoubleSpinBox *waterTempSpin;
    QDoubleSpinBox *ambientTempSpin;
    QDoubleSpinBox *insulationSpin;

    // Onglet Appareils
    QWidget *fixturesTab;
    QComboBox *fixtureTypeCombo;
    QSpinBox *fixtureQuantitySpin;
    QPushButton *addFixtureButton;
    QPushButton *removeFixtureButton;
    QTableWidget *fixturesTable;

    // Onglet Résultats
    QWidget *resultsTab;
    QLabel *flowRateLabel;
    QLabel *velocityLabel;
    QLabel *pressureDropLabel;
    QLabel *diameterLabel;
    QLabel *availablePressureLabel;
    QGroupBox *returnResultsGroup;  // Groupe pour résultats bouclage
    QLabel *returnDiameterLabel;
    QLabel *returnFlowRateLabel;
    QLabel *returnVelocityLabel;
    QLabel *heatLossLabel;
    QTextEdit *recommendationsText;

    // Boutons d'action
    QPushButton *calculateButton;
    QPushButton *exportButton;
    QPushButton *clearButton;
    QPushButton *closeButton;

    // Données
    std::vector<HydraulicCalc::Fixture> fixtures;
    HydraulicCalc::PipeSegmentResult lastResult;
    HydraulicCalc::CalculationParameters lastParams;

    // Données multi-segments
    bool multiSegmentMode;
    std::vector<HydraulicCalc::NetworkSegment> networkSegments;
    HydraulicCalc::NetworkCalculationParameters lastNetworkParams;
    int selectedSegmentIndex;
};
