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
    void onAddFixture();
    void onRemoveFixture();
    void onCalculate();
    void onExportPDF();
    void onClearResults();

private:
    void setupUi();
    void applyStyle();
    void updateFixtureTable();
    void displayResults(const HydraulicCalc::PipeSegmentResult& result);

    // Widgets principaux
    QTabWidget *tabWidget;

    // Onglet Paramètres
    QWidget *parametersTab;
    QComboBox *networkTypeCombo;
    QComboBox *materialCombo;
    QDoubleSpinBox *lengthSpin;
    QDoubleSpinBox *heightDiffSpin;
    QDoubleSpinBox *supplyPressureSpin;
    QDoubleSpinBox *requiredPressureSpin;

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
};
