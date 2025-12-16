#include "HydraulicCalculationsWindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QPrinter>
#include <QPainter>
#include <QTextDocument>
#include <QDateTime>

HydraulicCalculationsWindow::HydraulicCalculationsWindow(QWidget *parent)
    : QDialog(parent)
    , multiSegmentMode(false)
    , selectedSegmentIndex(-1)
{
    setupUi();
    applyStyle();

    // Connexions
    connect(networkTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HydraulicCalculationsWindow::onNetworkTypeChanged);
    connect(multiSegmentCheckbox, &QCheckBox::checkStateChanged,
            this, &HydraulicCalculationsWindow::onMultiSegmentModeChanged);
    connect(addSegmentButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onAddSegment);
    connect(editSegmentButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onEditSegment);
    connect(removeSegmentButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onRemoveSegment);
    connect(segmentsTable, &QTableWidget::itemSelectionChanged,
            this, &HydraulicCalculationsWindow::onSegmentSelectionChanged);
    connect(addFixtureButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onAddFixture);
    connect(removeFixtureButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onRemoveFixture);
    connect(calculateButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onCalculate);
    connect(exportButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onExportPDF);
    connect(clearButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onClearResults);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    // Initialisation
    onNetworkTypeChanged(0);
    updateSegmentParametersVisibility();
}

HydraulicCalculationsWindow::~HydraulicCalculationsWindow()
{
}

void HydraulicCalculationsWindow::setupUi()
{
    setWindowTitle("Calculs Hydrauliques - Dimensionnement de réseaux");
    setMinimumSize(900, 700);
    resize(1000, 750);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // Titre
    QLabel *titleLabel = new QLabel("💧 Dimensionnement de réseaux d'eau", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(titleLabel);

    // Onglets
    tabWidget = new QTabWidget(this);

    // ========== ONGLET PARAMÈTRES ==========
    parametersTab = new QWidget();
    QVBoxLayout *paramsLayout = new QVBoxLayout(parametersTab);
    paramsLayout->setSpacing(15);

    // Type de réseau
    QGroupBox *networkGroup = new QGroupBox("Type de réseau", this);
    QVBoxLayout *networkLayout = new QVBoxLayout(networkGroup);

    networkTypeCombo = new QComboBox(this);
    networkTypeCombo->addItem("Eau froide");
    networkTypeCombo->addItem("Eau chaude sanitaire");
    networkTypeCombo->addItem("ECS avec bouclage");
    networkLayout->addWidget(networkTypeCombo);

    paramsLayout->addWidget(networkGroup);

    // Matériau
    QGroupBox *materialGroup = new QGroupBox("Matériau des canalisations", this);
    QVBoxLayout *materialLayout = new QVBoxLayout(materialGroup);

    materialCombo = new QComboBox(this);
    materialCombo->addItem("Cuivre");
    materialCombo->addItem("PER (Polyéthylène réticulé)");
    materialCombo->addItem("Multicouche");
    materialCombo->addItem("Acier galvanisé");
    materialLayout->addWidget(materialCombo);

    paramsLayout->addWidget(materialGroup);

    // Mode multi-segments
    multiSegmentCheckbox = new QCheckBox("Mode multi-segments (réseau avec plusieurs tronçons)", this);
    multiSegmentCheckbox->setStyleSheet("font-weight: bold; margin: 10px 0;");
    paramsLayout->addWidget(multiSegmentCheckbox);

    // ===== MODE SEGMENT UNIQUE =====
    singleSegmentGroup = new QGroupBox("Dimensions du tronçon", this);
    QFormLayout *dimensionsLayout = new QFormLayout(singleSegmentGroup);

    lengthSpin = new QDoubleSpinBox(this);
    lengthSpin->setRange(0.1, 1000.0);
    lengthSpin->setValue(10.0);
    lengthSpin->setSuffix(" m");
    lengthSpin->setDecimals(1);
    dimensionsLayout->addRow("Longueur:", lengthSpin);

    heightDiffSpin = new QDoubleSpinBox(this);
    heightDiffSpin->setRange(-100.0, 100.0);
    heightDiffSpin->setValue(3.0);
    heightDiffSpin->setSuffix(" m");
    heightDiffSpin->setDecimals(1);
    dimensionsLayout->addRow("Différence de hauteur:", heightDiffSpin);

    paramsLayout->addWidget(singleSegmentGroup);

    // ===== MODE MULTI-SEGMENTS =====
    multiSegmentGroup = new QGroupBox("Gestion des segments du réseau", this);
    QVBoxLayout *segmentsLayout = new QVBoxLayout(multiSegmentGroup);

    segmentsTable = new QTableWidget(this);
    segmentsTable->setColumnCount(5);
    segmentsTable->setHorizontalHeaderLabels({"Nom", "Parent", "Longueur (m)", "Hauteur (m)", "Appareils"});
    segmentsTable->horizontalHeader()->setStretchLastSection(true);
    segmentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    segmentsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    segmentsTable->setAlternatingRowColors(true);
    segmentsTable->setMinimumHeight(150);
    segmentsLayout->addWidget(segmentsTable);

    QHBoxLayout *segmentButtonsLayout = new QHBoxLayout();
    addSegmentButton = new QPushButton("➕ Ajouter un segment", this);
    editSegmentButton = new QPushButton("✏️ Modifier", this);
    removeSegmentButton = new QPushButton("❌ Supprimer", this);
    segmentButtonsLayout->addWidget(addSegmentButton);
    segmentButtonsLayout->addWidget(editSegmentButton);
    segmentButtonsLayout->addWidget(removeSegmentButton);
    segmentButtonsLayout->addStretch();
    segmentsLayout->addLayout(segmentButtonsLayout);

    paramsLayout->addWidget(multiSegmentGroup);
    multiSegmentGroup->setVisible(false);

    // Pressions
    QGroupBox *pressuresGroup = new QGroupBox("Pressions", this);
    QFormLayout *pressuresLayout = new QFormLayout(pressuresGroup);

    supplyPressureSpin = new QDoubleSpinBox(this);
    supplyPressureSpin->setRange(0.5, 10.0);
    supplyPressureSpin->setValue(3.0);
    supplyPressureSpin->setSuffix(" bar");
    supplyPressureSpin->setDecimals(1);
    pressuresLayout->addRow("Pression d'alimentation:", supplyPressureSpin);

    requiredPressureSpin = new QDoubleSpinBox(this);
    requiredPressureSpin->setRange(0.5, 5.0);
    requiredPressureSpin->setValue(1.0);
    requiredPressureSpin->setSuffix(" bar");
    requiredPressureSpin->setDecimals(1);
    pressuresLayout->addRow("Pression minimale requise:", requiredPressureSpin);

    paramsLayout->addWidget(pressuresGroup);

    // Paramètres spécifiques bouclage
    loopParametersGroup = new QGroupBox("Paramètres du bouclage ECS", this);
    QFormLayout *loopLayout = new QFormLayout(loopParametersGroup);

    loopLengthSpin = new QDoubleSpinBox(this);
    loopLengthSpin->setRange(1.0, 500.0);
    loopLengthSpin->setValue(50.0);
    loopLengthSpin->setSuffix(" m");
    loopLengthSpin->setDecimals(1);
    loopLayout->addRow("Longueur de la boucle:", loopLengthSpin);

    waterTempSpin = new QDoubleSpinBox(this);
    waterTempSpin->setRange(40.0, 80.0);
    waterTempSpin->setValue(60.0);
    waterTempSpin->setSuffix(" °C");
    waterTempSpin->setDecimals(1);
    loopLayout->addRow("Température de l'eau:", waterTempSpin);

    ambientTempSpin = new QDoubleSpinBox(this);
    ambientTempSpin->setRange(5.0, 35.0);
    ambientTempSpin->setValue(20.0);
    ambientTempSpin->setSuffix(" °C");
    ambientTempSpin->setDecimals(1);
    loopLayout->addRow("Température ambiante:", ambientTempSpin);

    insulationSpin = new QDoubleSpinBox(this);
    insulationSpin->setRange(6.0, 50.0);
    insulationSpin->setValue(13.0);
    insulationSpin->setSuffix(" mm");
    insulationSpin->setDecimals(0);
    loopLayout->addRow("Épaisseur d'isolation:", insulationSpin);

    paramsLayout->addWidget(loopParametersGroup);

    paramsLayout->addStretch();
    tabWidget->addTab(parametersTab, "Paramètres");

    // ========== ONGLET APPAREILS ==========
    fixturesTab = new QWidget();
    QVBoxLayout *fixturesLayout = new QVBoxLayout(fixturesTab);

    QLabel *fixturesInfo = new QLabel(
        "Ajoutez les appareils sanitaires desservis par ce tronçon.\n"
        "Le débit de dimensionnement sera calculé avec coefficient de simultanéité.",
        this
    );
    fixturesInfo->setWordWrap(true);
    fixturesInfo->setStyleSheet("color: #6c757d; font-size: 11px; padding: 10px; background: #f8f9fa; border-radius: 4px;");
    fixturesLayout->addWidget(fixturesInfo);

    // Sélection d'appareil
    QHBoxLayout *addFixtureLayout = new QHBoxLayout();

    fixtureTypeCombo = new QComboBox(this);
    fixtureTypeCombo->addItem("Lavabo");
    fixtureTypeCombo->addItem("Évier");
    fixtureTypeCombo->addItem("Douche");
    fixtureTypeCombo->addItem("Baignoire");
    fixtureTypeCombo->addItem("WC");
    fixtureTypeCombo->addItem("Bidet");
    fixtureTypeCombo->addItem("Lave-linge");
    fixtureTypeCombo->addItem("Lave-vaisselle");
    fixtureTypeCombo->addItem("Urinoir à chasse");
    fixtureTypeCombo->addItem("Urinoir à écoulement");
    addFixtureLayout->addWidget(new QLabel("Type:", this));
    addFixtureLayout->addWidget(fixtureTypeCombo, 1);

    fixtureQuantitySpin = new QSpinBox(this);
    fixtureQuantitySpin->setRange(1, 100);
    fixtureQuantitySpin->setValue(1);
    addFixtureLayout->addWidget(new QLabel("Quantité:", this));
    addFixtureLayout->addWidget(fixtureQuantitySpin);

    addFixtureButton = new QPushButton("Ajouter", this);
    addFixtureButton->setFixedWidth(100);
    addFixtureLayout->addWidget(addFixtureButton);

    removeFixtureButton = new QPushButton("Supprimer", this);
    removeFixtureButton->setFixedWidth(100);
    addFixtureLayout->addWidget(removeFixtureButton);

    fixturesLayout->addLayout(addFixtureLayout);

    // Table des appareils
    fixturesTable = new QTableWidget(0, 3, this);
    fixturesTable->setHorizontalHeaderLabels({"Type d'appareil", "Quantité", "Débit unitaire (L/min)"});
    fixturesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    fixturesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    fixturesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    fixturesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    fixturesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fixturesLayout->addWidget(fixturesTable);

    tabWidget->addTab(fixturesTab, "Appareils sanitaires");

    // ========== ONGLET RÉSULTATS ==========
    resultsTab = new QWidget();
    QVBoxLayout *resultsLayout = new QVBoxLayout(resultsTab);

    QGroupBox *resultsGroup = new QGroupBox("Résultats du calcul", this);
    QFormLayout *resultsFormLayout = new QFormLayout(resultsGroup);

    flowRateLabel = new QLabel("-", this);
    flowRateLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    resultsFormLayout->addRow("Débit de calcul:", flowRateLabel);

    diameterLabel = new QLabel("-", this);
    diameterLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    resultsFormLayout->addRow("Diamètre nominal:", diameterLabel);

    velocityLabel = new QLabel("-", this);
    velocityLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    resultsFormLayout->addRow("Vitesse de l'eau:", velocityLabel);

    pressureDropLabel = new QLabel("-", this);
    pressureDropLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    resultsFormLayout->addRow("Perte de charge:", pressureDropLabel);

    availablePressureLabel = new QLabel("-", this);
    availablePressureLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    resultsFormLayout->addRow("Pression disponible:", availablePressureLabel);

    resultsLayout->addWidget(resultsGroup);

    // Groupe résultats bouclage (masqué par défaut)
    returnResultsGroup = new QGroupBox("Résultats du retour de bouclage", this);
    returnResultsGroup->setVisible(false);
    QFormLayout *returnFormLayout = new QFormLayout(returnResultsGroup);

    returnDiameterLabel = new QLabel("-", this);
    returnDiameterLabel->setStyleSheet("font-weight: bold; color: #28a745;");
    returnFormLayout->addRow("Diamètre retour:", returnDiameterLabel);

    returnFlowRateLabel = new QLabel("-", this);
    returnFlowRateLabel->setStyleSheet("font-weight: bold; color: #28a745;");
    returnFormLayout->addRow("Débit retour:", returnFlowRateLabel);

    returnVelocityLabel = new QLabel("-", this);
    returnVelocityLabel->setStyleSheet("font-weight: bold; color: #28a745;");
    returnFormLayout->addRow("Vitesse retour:", returnVelocityLabel);

    heatLossLabel = new QLabel("-", this);
    heatLossLabel->setStyleSheet("font-weight: bold; color: #28a745;");
    returnFormLayout->addRow("Pertes thermiques:", heatLossLabel);

    returnTemperatureLabel = new QLabel("-", this);
    returnTemperatureLabel->setStyleSheet("font-weight: bold; color: #28a745;");
    returnFormLayout->addRow("Température retour:", returnTemperatureLabel);

    resultsLayout->addWidget(returnResultsGroup);

    QGroupBox *recommendationsGroup = new QGroupBox("Recommandations", this);
    QVBoxLayout *recommendationsLayout = new QVBoxLayout(recommendationsGroup);

    recommendationsText = new QTextEdit(this);
    recommendationsText->setReadOnly(true);
    recommendationsText->setMaximumHeight(150);
    recommendationsLayout->addWidget(recommendationsText);

    resultsLayout->addWidget(recommendationsGroup);
    resultsLayout->addStretch();

    tabWidget->addTab(resultsTab, "Résultats");

    mainLayout->addWidget(tabWidget);

    // Boutons d'action
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addStretch();

    clearButton = new QPushButton("Effacer", this);
    clearButton->setFixedSize(120, 35);
    buttonsLayout->addWidget(clearButton);

    calculateButton = new QPushButton("Calculer", this);
    calculateButton->setFixedSize(120, 35);
    buttonsLayout->addWidget(calculateButton);

    exportButton = new QPushButton("Exporter PDF", this);
    exportButton->setFixedSize(120, 35);
    exportButton->setEnabled(false);
    buttonsLayout->addWidget(exportButton);

    closeButton = new QPushButton("Fermer", this);
    closeButton->setFixedSize(120, 35);
    buttonsLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonsLayout);
}

void HydraulicCalculationsWindow::applyStyle()
{
    QString style = R"(
        QDialog {
            background-color: #ffffff;
            color: #2c3e50;
        }
        QGroupBox {
            font-weight: bold;
            border: 2px solid #bdc3c7;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 10px;
            background-color: #ffffff;
            color: #2c3e50;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 5px 10px;
            color: #2c3e50;
        }
        QPushButton {
            background-color: #4472C4;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #365a9e;
        }
        QPushButton:pressed {
            background-color: #2d4a82;
        }
        QPushButton:disabled {
            background-color: #bdc3c7;
            color: #7f8c8d;
        }
        QComboBox, QSpinBox, QDoubleSpinBox, QLineEdit {
            padding: 5px;
            border: 1px solid #bdc3c7;
            border-radius: 4px;
            background-color: #ffffff;
            color: #2c3e50;
        }
        QComboBox QAbstractItemView {
            background-color: #ffffff;
            color: #2c3e50;
            selection-background-color: #4472C4;
            selection-color: white;
        }
        QCheckBox {
            color: #2c3e50;
            spacing: 5px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 2px solid #bdc3c7;
            border-radius: 3px;
            background-color: #ffffff;
        }
        QCheckBox::indicator:checked {
            background-color: #4472C4;
            border-color: #4472C4;
        }
        QTabWidget::pane {
            border: 1px solid #bdc3c7;
            border-radius: 4px;
            background-color: #ffffff;
        }
        QTabBar::tab {
            background-color: #ecf0f1;
            color: #2c3e50;
            padding: 8px 16px;
            margin-right: 2px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        QTabBar::tab:selected {
            background-color: #4472C4;
            color: white;
        }
        QTabBar::tab:hover:!selected {
            background-color: #d5dbdb;
        }
        QTableWidget {
            background-color: #ffffff;
            color: #2c3e50;
            alternate-background-color: #f8f9fa;
            gridline-color: #dee2e6;
            border: 1px solid #bdc3c7;
            border-radius: 4px;
        }
        QTableWidget::item {
            color: #2c3e50;
            padding: 5px;
        }
        QTableWidget::item:selected {
            background-color: #4472C4;
            color: white;
        }
        QHeaderView::section {
            background-color: #ecf0f1;
            color: #2c3e50;
            padding: 5px;
            border: 1px solid #bdc3c7;
            font-weight: bold;
        }
        QTextEdit {
            background-color: #ffffff;
            color: #2c3e50;
            border: 1px solid #bdc3c7;
            border-radius: 4px;
            padding: 5px;
        }
        QLabel {
            color: #2c3e50;
        }
    )";
    setStyleSheet(style);
}

void HydraulicCalculationsWindow::onNetworkTypeChanged(int index)
{
    // Afficher/masquer les paramètres de bouclage
    loopParametersGroup->setVisible(index == 2); // ECS avec bouclage
}

void HydraulicCalculationsWindow::onAddFixture()
{
    int typeIndex = fixtureTypeCombo->currentIndex();
    int quantity = fixtureQuantitySpin->value();

    HydraulicCalc::FixtureType type = static_cast<HydraulicCalc::FixtureType>(typeIndex);
    fixtures.push_back(HydraulicCalc::Fixture(type, quantity));

    updateFixtureTable();

    // Sauvegarder les appareils dans le segment sélectionné (mode multi-segments)
    if (multiSegmentMode && selectedSegmentIndex >= 0 &&
        selectedSegmentIndex < static_cast<int>(networkSegments.size())) {
        networkSegments[selectedSegmentIndex].fixtures = fixtures;
        updateSegmentTable();
    }
}

void HydraulicCalculationsWindow::onRemoveFixture()
{
    int currentRow = fixturesTable->currentRow();
    if (currentRow >= 0 && currentRow < static_cast<int>(fixtures.size())) {
        fixtures.erase(fixtures.begin() + currentRow);
        updateFixtureTable();

        // Sauvegarder les appareils dans le segment sélectionné (mode multi-segments)
        if (multiSegmentMode && selectedSegmentIndex >= 0 &&
            selectedSegmentIndex < static_cast<int>(networkSegments.size())) {
            networkSegments[selectedSegmentIndex].fixtures = fixtures;
            updateSegmentTable();
        }
    }
}

void HydraulicCalculationsWindow::updateFixtureTable()
{
    fixturesTable->setRowCount(0);

    for (size_t i = 0; i < fixtures.size(); ++i) {
        int row = fixturesTable->rowCount();
        fixturesTable->insertRow(row);

        QString typeName = QString::fromStdString(
            HydraulicCalc::PipeCalculator::getFixtureName(fixtures[i].type)
        );
        fixturesTable->setItem(row, 0, new QTableWidgetItem(typeName));
        fixturesTable->setItem(row, 1, new QTableWidgetItem(QString::number(fixtures[i].quantity)));
        fixturesTable->setItem(row, 2, new QTableWidgetItem(QString::number(fixtures[i].flowRate, 'f', 1)));
    }
}

void HydraulicCalculationsWindow::onCalculate()
{
    HydraulicCalc::PipeCalculator calculator;
    int networkIndex = networkTypeCombo->currentIndex();
    int materialIndex = materialCombo->currentIndex();

    if (multiSegmentMode) {
        // ===== MODE MULTI-SEGMENTS =====
        if (networkSegments.empty()) {
            QMessageBox::warning(this, "Attention",
                "Veuillez ajouter au moins un segment.");
            return;
        }

        // Vérifier qu'au moins un segment a des appareils
        bool hasFixtures = false;
        for (const auto& segment : networkSegments) {
            if (!segment.fixtures.empty()) {
                hasFixtures = true;
                break;
            }
        }

        if (!hasFixtures) {
            QMessageBox::warning(this, "Attention",
                "Veuillez ajouter des appareils à au moins un segment.");
            return;
        }

        // Préparation des paramètres réseau
        HydraulicCalc::NetworkCalculationParameters networkParams;
        networkParams.networkType = static_cast<HydraulicCalc::NetworkType>(networkIndex);
        networkParams.material = static_cast<HydraulicCalc::PipeMaterial>(materialIndex);
        networkParams.supplyPressure = supplyPressureSpin->value();
        networkParams.requiredPressure = requiredPressureSpin->value();
        networkParams.segments = networkSegments;

        // Paramètres bouclage si nécessaire
        if (networkIndex == 2) {
            networkParams.loopLength = loopLengthSpin->value();
            networkParams.waterTemperature = waterTempSpin->value();
            networkParams.ambientTemperature = ambientTempSpin->value();
            networkParams.insulationThickness = insulationSpin->value();
        }

        // Calcul du réseau complet
        calculator.calculateNetwork(networkParams);

        // Sauvegarde pour export
        lastNetworkParams = networkParams;

        // Affichage des résultats
        displayMultiSegmentResults(networkParams);

    } else {
        // ===== MODE SEGMENT UNIQUE =====
        if (fixtures.empty()) {
            QMessageBox::warning(this, "Attention",
                "Veuillez ajouter au moins un appareil sanitaire.");
            return;
        }

        // Préparation des paramètres
        HydraulicCalc::CalculationParameters params;
        params.networkType = static_cast<HydraulicCalc::NetworkType>(networkIndex);
        params.material = static_cast<HydraulicCalc::PipeMaterial>(materialIndex);
        params.length = lengthSpin->value();
        params.heightDifference = heightDiffSpin->value();
        params.supplyPressure = supplyPressureSpin->value();
        params.requiredPressure = requiredPressureSpin->value();
        params.fixtures = fixtures;

        // Paramètres bouclage si nécessaire
        if (networkIndex == 2) {
            params.loopLength = loopLengthSpin->value();
            params.waterTemperature = waterTempSpin->value();
            params.ambientTemperature = ambientTempSpin->value();
            params.insulationThickness = insulationSpin->value();
        }

        // Calcul
        HydraulicCalc::PipeSegmentResult result = calculator.calculate(params);

        // Sauvegarde pour export
        lastResult = result;
        lastParams = params;

        // Affichage des résultats
        displayResults(result);
    }

    // Activer le bouton d'export
    exportButton->setEnabled(true);

    // Basculer sur l'onglet résultats
    tabWidget->setCurrentWidget(resultsTab);
}

void HydraulicCalculationsWindow::displayResults(const HydraulicCalc::PipeSegmentResult& result)
{
    // Résultats aller (ou réseau simple)
    flowRateLabel->setText(QString::number(result.flowRate, 'f', 2) + " L/min");
    diameterLabel->setText("DN " + QString::number(result.nominalDiameter) +
                          " (Ø int. " + QString::number(result.actualDiameter, 'f', 1) + " mm)");
    velocityLabel->setText(QString::number(result.velocity, 'f', 2) + " m/s");
    pressureDropLabel->setText(QString::number(result.pressureDrop, 'f', 2) + " mCE");

    double availablePressure = lastParams.supplyPressure - (result.pressureDrop / 10.0);
    availablePressureLabel->setText(QString::number(availablePressure, 'f', 2) + " bar");

    // Résultats retour bouclage (si applicable)
    if (result.hasReturn) {
        returnResultsGroup->setVisible(true);
        returnDiameterLabel->setText("DN " + QString::number(result.returnNominalDiameter) +
                                    " (Ø int. " + QString::number(result.returnActualDiameter, 'f', 1) + " mm)");
        returnFlowRateLabel->setText(QString::number(result.returnFlowRate, 'f', 2) + " L/min");
        returnVelocityLabel->setText(QString::number(result.returnVelocity, 'f', 2) + " m/s");
        heatLossLabel->setText(QString::number(result.heatLoss, 'f', 0) + " W");
        returnTemperatureLabel->setText(QString::number(result.returnTemperature, 'f', 1) + " °C");
    } else {
        returnResultsGroup->setVisible(false);
    }

    recommendationsText->setPlainText(QString::fromStdString(result.recommendation));
}

void HydraulicCalculationsWindow::onExportPDF()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Exporter la fiche de calcul", "",
        "Fichier PDF (*.pdf)");

    if (fileName.isEmpty()) {
        return;
    }

    if (!fileName.endsWith(".pdf", Qt::CaseInsensitive)) {
        fileName += ".pdf";
    }

    // Création du document HTML
    QString html = "<html><head><style>"
        "body { font-family: Arial, sans-serif; margin: 40px; }"
        "h1 { color: #2c3e50; border-bottom: 3px solid #4472C4; padding-bottom: 10px; page-break-after: avoid; }"
        "h2 { color: #4472C4; margin-top: 30px; page-break-after: avoid; page-break-before: auto; }"
        "h3 { color: #5a8fd1; margin-top: 20px; border-left: 4px solid #4472C4; padding-left: 10px; page-break-after: avoid; }"
        "table { border-collapse: collapse; width: 100%; margin: 20px 0; page-break-inside: avoid; }"
        "th, td { border: 1px solid #bdc3c7; padding: 10px; text-align: left; }"
        "th { background-color: #ecf0f1; font-weight: bold; }"
        ".result { background-color: #e8f5e9; font-weight: bold; }"
        ".segment-header { background-color: #d1e7fd; font-weight: bold; }"
        ".segment-section { page-break-inside: avoid; margin-bottom: 30px; }"
        ".info { color: #6c757d; font-size: 0.9em; margin-top: 30px; }"
        ".page-break { page-break-before: always; margin-top: 0; }"
        "p { page-break-inside: avoid; }"
        "</style></head><body>";

    html += "<h1>Fiche de calcul - Dimensionnement hydraulique</h1>";
    html += "<p><strong>Date:</strong> " + QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm") + "</p>";

    if (multiSegmentMode) {
        // ===== EXPORT MULTI-SEGMENTS =====
        html += "<h2>Paramètres généraux</h2>";
        html += "<table>";
        html += "<tr><th>Paramètre</th><th>Valeur</th></tr>";
        html += "<tr><td>Mode de calcul</td><td>Multi-segments</td></tr>";
        html += "<tr><td>Type de réseau</td><td>" + networkTypeCombo->currentText() + "</td></tr>";
        html += "<tr><td>Matériau</td><td>" + materialCombo->currentText() + "</td></tr>";
        html += "<tr><td>Pression d'alimentation</td><td>" + QString::number(lastNetworkParams.supplyPressure, 'f', 1) + " bar</td></tr>";
        html += "<tr><td>Pression minimale requise</td><td>" + QString::number(lastNetworkParams.requiredPressure, 'f', 1) + " bar</td></tr>";
        html += "<tr><td>Nombre de segments</td><td>" + QString::number(lastNetworkParams.segments.size()) + "</td></tr>";
        html += "</table>";

        if (lastNetworkParams.networkType == HydraulicCalc::NetworkType::HotWaterWithLoop) {
            html += "<h2>Paramètres du bouclage ECS</h2>";
            html += "<table>";
            html += "<tr><th>Paramètre</th><th>Valeur</th></tr>";
            html += "<tr><td>Longueur de la boucle</td><td>" + QString::number(lastNetworkParams.loopLength, 'f', 1) + " m</td></tr>";
            html += "<tr><td>Température de l'eau</td><td>" + QString::number(lastNetworkParams.waterTemperature, 'f', 1) + " °C</td></tr>";
            html += "<tr><td>Température ambiante</td><td>" + QString::number(lastNetworkParams.ambientTemperature, 'f', 1) + " °C</td></tr>";
            html += "<tr><td>Épaisseur d'isolation</td><td>" + QString::number(lastNetworkParams.insulationThickness, 'f', 0) + " mm</td></tr>";
            html += "</table>";
        }

        // Résultats pour chaque segment
        bool firstSegment = true;
        for (const auto& segment : lastNetworkParams.segments) {
            if (!firstSegment) {
                html += "<div class='page-break'></div>";
            }
            firstSegment = false;

            html += "<div class='segment-section'>";
            html += "<h2>Segment: " + QString::fromStdString(segment.name) + "</h2>";

            // Info segment
            html += "<table>";
            html += "<tr><th>Paramètre</th><th>Valeur</th></tr>";
            QString parentName = "(Segment racine)";
            if (!segment.parentId.empty()) {
                for (const auto& ps : lastNetworkParams.segments) {
                    if (ps.id == segment.parentId) {
                        parentName = QString::fromStdString(ps.name);
                        break;
                    }
                }
            }
            html += "<tr><td>Segment parent</td><td>" + parentName + "</td></tr>";
            html += "<tr><td>Longueur</td><td>" + QString::number(segment.length, 'f', 1) + " m</td></tr>";
            html += "<tr><td>Différence de hauteur</td><td>" + QString::number(segment.heightDifference, 'f', 1) + " m</td></tr>";
            html += "<tr><td>Pression en entrée</td><td>" + QString::number(segment.inletPressure, 'f', 2) + " bar</td></tr>";
            html += "<tr><td>Pression en sortie</td><td>" + QString::number(segment.outletPressure, 'f', 2) + " bar</td></tr>";
            html += "</table>";

            // Appareils du segment
            if (!segment.fixtures.empty()) {
                html += "<h3>Appareils sanitaires</h3>";
                html += "<table>";
                html += "<tr><th>Type d'appareil</th><th>Quantité</th><th>Débit unitaire (L/min)</th></tr>";
                for (const auto& fixture : segment.fixtures) {
                    html += "<tr>";
                    html += "<td>" + QString::fromStdString(HydraulicCalc::PipeCalculator::getFixtureName(fixture.type)) + "</td>";
                    html += "<td>" + QString::number(fixture.quantity) + "</td>";
                    html += "<td>" + QString::number(fixture.flowRate, 'f', 1) + "</td>";
                    html += "</tr>";
                }
                html += "</table>";
            }

            // Résultats du segment
            html += "<h3>Résultats du dimensionnement</h3>";
            html += "<table>";
            html += "<tr><th>Résultat</th><th>Valeur</th></tr>";
            html += "<tr class='result'><td>Débit de calcul</td><td>" + QString::number(segment.result.flowRate, 'f', 2) + " L/min</td></tr>";
            html += "<tr class='result'><td>Diamètre nominal</td><td>DN " + QString::number(segment.result.nominalDiameter) +
                    " (Ø int. " + QString::number(segment.result.actualDiameter, 'f', 1) + " mm)</td></tr>";
            html += "<tr><td>Vitesse de l'eau</td><td>" + QString::number(segment.result.velocity, 'f', 2) + " m/s</td></tr>";
            html += "<tr><td>Perte de charge</td><td>" + QString::number(segment.result.pressureDrop, 'f', 2) + " mCE</td></tr>";
            html += "</table>";

            // Retour bouclage si applicable
            if (segment.result.hasReturn) {
                html += "<h3>Retour de bouclage</h3>";
                html += "<table>";
                html += "<tr><th>Résultat</th><th>Valeur</th></tr>";
                html += "<tr class='result'><td>Diamètre retour</td><td>DN " + QString::number(segment.result.returnNominalDiameter) +
                        " (Ø int. " + QString::number(segment.result.returnActualDiameter, 'f', 1) + " mm)</td></tr>";
                html += "<tr><td>Débit de retour</td><td>" + QString::number(segment.result.returnFlowRate, 'f', 2) + " L/min</td></tr>";
                html += "<tr><td>Vitesse de retour</td><td>" + QString::number(segment.result.returnVelocity, 'f', 2) + " m/s</td></tr>";
                html += "<tr><td>Pertes thermiques</td><td>" + QString::number(segment.result.heatLoss, 'f', 0) + " W</td></tr>";
                html += "<tr><td>Température retour</td><td>" + QString::number(segment.result.returnTemperature, 'f', 1) + " °C</td></tr>";
                html += "</table>";
            }

            html += "<h3>Recommandations</h3>";
            html += "<p>" + QString::fromStdString(segment.result.recommendation) + "</p>";
            html += "</div>"; // Fin de segment-section
        }

    } else {
        // ===== EXPORT SEGMENT UNIQUE =====
        html += "<h2>Paramètres du projet</h2>";
        html += "<table>";
        html += "<tr><th>Paramètre</th><th>Valeur</th></tr>";
        html += "<tr><td>Type de réseau</td><td>" + networkTypeCombo->currentText() + "</td></tr>";
        html += "<tr><td>Matériau</td><td>" + materialCombo->currentText() + "</td></tr>";
        html += "<tr><td>Longueur du tronçon</td><td>" + QString::number(lastParams.length, 'f', 1) + " m</td></tr>";
        html += "<tr><td>Différence de hauteur</td><td>" + QString::number(lastParams.heightDifference, 'f', 1) + " m</td></tr>";
        html += "<tr><td>Pression d'alimentation</td><td>" + QString::number(lastParams.supplyPressure, 'f', 1) + " bar</td></tr>";
        html += "<tr><td>Pression minimale requise</td><td>" + QString::number(lastParams.requiredPressure, 'f', 1) + " bar</td></tr>";
        html += "</table>";

        if (lastParams.networkType == HydraulicCalc::NetworkType::HotWaterWithLoop) {
            html += "<h2>Paramètres du bouclage ECS</h2>";
            html += "<table>";
            html += "<tr><th>Paramètre</th><th>Valeur</th></tr>";
            html += "<tr><td>Longueur de la boucle</td><td>" + QString::number(lastParams.loopLength, 'f', 1) + " m</td></tr>";
            html += "<tr><td>Température de l'eau</td><td>" + QString::number(lastParams.waterTemperature, 'f', 1) + " °C</td></tr>";
            html += "<tr><td>Température ambiante</td><td>" + QString::number(lastParams.ambientTemperature, 'f', 1) + " °C</td></tr>";
            html += "<tr><td>Épaisseur d'isolation</td><td>" + QString::number(lastParams.insulationThickness, 'f', 0) + " mm</td></tr>";
            html += "</table>";
        }

        html += "<h2>Appareils sanitaires</h2>";
        html += "<table>";
        html += "<tr><th>Type d'appareil</th><th>Quantité</th><th>Débit unitaire (L/min)</th></tr>";
        for (const auto& fixture : fixtures) {
            html += "<tr>";
            html += "<td>" + QString::fromStdString(HydraulicCalc::PipeCalculator::getFixtureName(fixture.type)) + "</td>";
            html += "<td>" + QString::number(fixture.quantity) + "</td>";
            html += "<td>" + QString::number(fixture.flowRate, 'f', 1) + "</td>";
            html += "</tr>";
        }
        html += "</table>";

        int totalFixtures = 0;
        for (const auto& f : fixtures) totalFixtures += f.quantity;
        double simCoeff = HydraulicCalc::PipeCalculator::getSimultaneityCoefficient(totalFixtures);
        html += "<p><strong>Nombre total d'appareils:</strong> " + QString::number(totalFixtures) + "</p>";
        html += "<p><strong>Coefficient de simultanéité:</strong> " + QString::number(simCoeff, 'f', 3) + "</p>";

        html += "<h2>Résultats du dimensionnement</h2>";
        html += "<table>";
        html += "<tr><th>Résultat</th><th>Valeur</th></tr>";
        html += "<tr class='result'><td>Débit de calcul</td><td>" + QString::number(lastResult.flowRate, 'f', 2) + " L/min</td></tr>";
        html += "<tr class='result'><td>Diamètre nominal</td><td>DN " + QString::number(lastResult.nominalDiameter) +
                " (Ø int. " + QString::number(lastResult.actualDiameter, 'f', 1) + " mm)</td></tr>";
        html += "<tr><td>Vitesse de l'eau</td><td>" + QString::number(lastResult.velocity, 'f', 2) + " m/s</td></tr>";
        html += "<tr><td>Perte de charge</td><td>" + QString::number(lastResult.pressureDrop, 'f', 2) + " mCE</td></tr>";

        double availablePressure = lastParams.supplyPressure - (lastResult.pressureDrop / 10.0);
        html += "<tr><td>Pression disponible</td><td>" + QString::number(availablePressure, 'f', 2) + " bar</td></tr>";
        html += "</table>";

        // Résultats du retour de bouclage si applicable
        if (lastResult.hasReturn) {
            html += "<h2>Résultats du retour de bouclage</h2>";
            html += "<table>";
            html += "<tr><th>Résultat</th><th>Valeur</th></tr>";
            html += "<tr class='result'><td>Diamètre retour</td><td>DN " + QString::number(lastResult.returnNominalDiameter) +
                    " (Ø int. " + QString::number(lastResult.returnActualDiameter, 'f', 1) + " mm)</td></tr>";
            html += "<tr><td>Débit de retour</td><td>" + QString::number(lastResult.returnFlowRate, 'f', 2) + " L/min</td></tr>";
            html += "<tr><td>Vitesse de retour</td><td>" + QString::number(lastResult.returnVelocity, 'f', 2) + " m/s</td></tr>";
            html += "<tr><td>Pertes thermiques</td><td>" + QString::number(lastResult.heatLoss, 'f', 0) + " W</td></tr>";
            html += "<tr><td>Température retour</td><td>" + QString::number(lastResult.returnTemperature, 'f', 1) + " °C</td></tr>";
            html += "</table>";
        }

        html += "<h2>Recommandations</h2>";
        html += "<p>" + QString::fromStdString(lastResult.recommendation) + "</p>";
    }

    html += "<div class='info'>";
    html += "<p><em>Calculs réalisés selon les normes DTU 60.11 et formules de Darcy-Weisbach.</em></p>";
    html += "<p><em>Document généré par TC Hub - Module de calculs hydrauliques</em></p>";
    html += "</div>";

    html += "</body></html>";

    // Création du PDF
    QTextDocument document;
    document.setHtml(html);

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageSize(QPageSize::A4);
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

    document.print(&printer);

    QMessageBox::information(this, "Export réussi",
        "La fiche de calcul a été exportée avec succès:\n" + fileName);
}

void HydraulicCalculationsWindow::onClearResults()
{
    fixtures.clear();
    updateFixtureTable();

    flowRateLabel->setText("-");
    diameterLabel->setText("-");
    velocityLabel->setText("-");
    pressureDropLabel->setText("-");
    availablePressureLabel->setText("-");
    returnResultsGroup->setVisible(false);
    returnDiameterLabel->setText("-");
    returnFlowRateLabel->setText("-");
    returnVelocityLabel->setText("-");
    heatLossLabel->setText("-");
    returnTemperatureLabel->setText("-");
    recommendationsText->clear();

    exportButton->setEnabled(false);

    tabWidget->setCurrentWidget(parametersTab);
}

void HydraulicCalculationsWindow::onMultiSegmentModeChanged(Qt::CheckState state)
{
    multiSegmentMode = (state == Qt::Checked);
    updateSegmentParametersVisibility();

    if (multiSegmentMode && networkSegments.empty()) {
        // Créer un segment par défaut
        HydraulicCalc::NetworkSegment defaultSegment("seg_1", "Branche principale");
        defaultSegment.length = 10.0;
        defaultSegment.heightDifference = 3.0;
        networkSegments.push_back(defaultSegment);
        updateSegmentTable();
    }
}

void HydraulicCalculationsWindow::updateSegmentParametersVisibility()
{
    singleSegmentGroup->setVisible(!multiSegmentMode);
    multiSegmentGroup->setVisible(multiSegmentMode);

    // En mode multi-segments, les appareils sont gérés par segment
    if (multiSegmentMode) {
        fixturesTab->setEnabled(selectedSegmentIndex >= 0);
    } else {
        fixturesTab->setEnabled(true);
    }
}

void HydraulicCalculationsWindow::onAddSegment()
{
    // Créer un dialogue pour saisir les informations du nouveau segment
    QDialog dialog(this);
    dialog.setWindowTitle("Ajouter un segment");
    dialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit *nameEdit = new QLineEdit(&dialog);
    nameEdit->setText("Segment " + QString::number(networkSegments.size() + 1));
    formLayout->addRow("Nom:", nameEdit);

    QComboBox *parentCombo = new QComboBox(&dialog);
    parentCombo->addItem("(Aucun - Segment racine)", "");
    for (const auto& seg : networkSegments) {
        parentCombo->addItem(QString::fromStdString(seg.name), QString::fromStdString(seg.id));
    }
    formLayout->addRow("Segment parent:", parentCombo);

    QDoubleSpinBox *lengthSpin = new QDoubleSpinBox(&dialog);
    lengthSpin->setRange(0.1, 1000.0);
    lengthSpin->setValue(10.0);
    lengthSpin->setSuffix(" m");
    lengthSpin->setDecimals(1);
    formLayout->addRow("Longueur:", lengthSpin);

    QDoubleSpinBox *heightSpin = new QDoubleSpinBox(&dialog);
    heightSpin->setRange(-100.0, 100.0);
    heightSpin->setValue(0.0);
    heightSpin->setSuffix(" m");
    heightSpin->setDecimals(1);
    formLayout->addRow("Différence de hauteur:", heightSpin);

    layout->addLayout(formLayout);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("OK", &dialog);
    QPushButton *cancelButton = new QPushButton("Annuler", &dialog);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);
    layout->addLayout(buttonsLayout);

    if (dialog.exec() == QDialog::Accepted) {
        HydraulicCalc::NetworkSegment newSegment;
        newSegment.id = "seg_" + std::to_string(networkSegments.size() + 1);
        newSegment.name = nameEdit->text().toStdString();
        newSegment.parentId = parentCombo->currentData().toString().toStdString();
        newSegment.length = lengthSpin->value();
        newSegment.heightDifference = heightSpin->value();

        networkSegments.push_back(newSegment);
        updateSegmentTable();
    }
}

void HydraulicCalculationsWindow::onEditSegment()
{
    int row = segmentsTable->currentRow();
    if (row < 0 || row >= static_cast<int>(networkSegments.size())) {
        QMessageBox::warning(this, "Aucune sélection", "Veuillez sélectionner un segment à modifier.");
        return;
    }

    HydraulicCalc::NetworkSegment& segment = networkSegments[row];

    // Créer un dialogue pour modifier les informations du segment
    QDialog dialog(this);
    dialog.setWindowTitle("Modifier le segment");
    dialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit *nameEdit = new QLineEdit(&dialog);
    nameEdit->setText(QString::fromStdString(segment.name));
    formLayout->addRow("Nom:", nameEdit);

    QComboBox *parentCombo = new QComboBox(&dialog);
    parentCombo->addItem("(Aucun - Segment racine)", "");
    for (size_t i = 0; i < networkSegments.size(); ++i) {
        if (i != static_cast<size_t>(row)) { // Ne pas pouvoir se choisir comme parent
            parentCombo->addItem(QString::fromStdString(networkSegments[i].name),
                               QString::fromStdString(networkSegments[i].id));
        }
    }
    // Sélectionner le parent actuel
    int parentIndex = parentCombo->findData(QString::fromStdString(segment.parentId));
    if (parentIndex >= 0) {
        parentCombo->setCurrentIndex(parentIndex);
    }
    formLayout->addRow("Segment parent:", parentCombo);

    QDoubleSpinBox *lengthSpin = new QDoubleSpinBox(&dialog);
    lengthSpin->setRange(0.1, 1000.0);
    lengthSpin->setValue(segment.length);
    lengthSpin->setSuffix(" m");
    lengthSpin->setDecimals(1);
    formLayout->addRow("Longueur:", lengthSpin);

    QDoubleSpinBox *heightSpin = new QDoubleSpinBox(&dialog);
    heightSpin->setRange(-100.0, 100.0);
    heightSpin->setValue(segment.heightDifference);
    heightSpin->setSuffix(" m");
    heightSpin->setDecimals(1);
    formLayout->addRow("Différence de hauteur:", heightSpin);

    layout->addLayout(formLayout);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("OK", &dialog);
    QPushButton *cancelButton = new QPushButton("Annuler", &dialog);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);
    layout->addLayout(buttonsLayout);

    if (dialog.exec() == QDialog::Accepted) {
        segment.name = nameEdit->text().toStdString();
        segment.parentId = parentCombo->currentData().toString().toStdString();
        segment.length = lengthSpin->value();
        segment.heightDifference = heightSpin->value();

        updateSegmentTable();
    }
}

void HydraulicCalculationsWindow::onRemoveSegment()
{
    int row = segmentsTable->currentRow();
    if (row < 0 || row >= static_cast<int>(networkSegments.size())) {
        QMessageBox::warning(this, "Aucune sélection", "Veuillez sélectionner un segment à supprimer.");
        return;
    }

    // Vérifier si d'autres segments dépendent de celui-ci
    std::string segmentId = networkSegments[row].id;
    bool hasChildren = false;
    for (const auto& seg : networkSegments) {
        if (seg.parentId == segmentId) {
            hasChildren = true;
            break;
        }
    }

    if (hasChildren) {
        QMessageBox::warning(this, "Suppression impossible",
            "Ce segment ne peut pas être supprimé car d'autres segments en dépendent.\n"
            "Veuillez d'abord supprimer ou modifier les segments enfants.");
        return;
    }

    auto reply = QMessageBox::question(this, "Confirmation",
        "Êtes-vous sûr de vouloir supprimer ce segment ?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        networkSegments.erase(networkSegments.begin() + row);
        updateSegmentTable();
        selectedSegmentIndex = -1;
    }
}

void HydraulicCalculationsWindow::onSegmentSelectionChanged()
{
    int row = segmentsTable->currentRow();
    selectedSegmentIndex = row;

    if (row >= 0 && row < static_cast<int>(networkSegments.size())) {
        // Charger les appareils du segment sélectionné
        fixtures = networkSegments[row].fixtures;
        updateFixtureTable();
        fixturesTab->setEnabled(true);
    } else {
        fixturesTab->setEnabled(false);
    }
}

void HydraulicCalculationsWindow::updateSegmentTable()
{
    // Sauvegarder la sélection actuelle
    int currentRow = segmentsTable->currentRow();

    segmentsTable->setRowCount(0);

    for (size_t i = 0; i < networkSegments.size(); ++i) {
        const auto& segment = networkSegments[i];
        int row = segmentsTable->rowCount();
        segmentsTable->insertRow(row);

        segmentsTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(segment.name)));

        // Trouver le nom du parent
        QString parentName = "(Racine)";
        if (!segment.parentId.empty()) {
            for (const auto& parentSeg : networkSegments) {
                if (parentSeg.id == segment.parentId) {
                    parentName = QString::fromStdString(parentSeg.name);
                    break;
                }
            }
        }
        segmentsTable->setItem(row, 1, new QTableWidgetItem(parentName));

        segmentsTable->setItem(row, 2, new QTableWidgetItem(QString::number(segment.length, 'f', 1)));
        segmentsTable->setItem(row, 3, new QTableWidgetItem(QString::number(segment.heightDifference, 'f', 1)));

        // Compter les appareils de ce segment ET de tous ses descendants
        int directFixtureCount = 0;
        for (const auto& fixture : segment.fixtures) {
            directFixtureCount += fixture.quantity;
        }

        int totalFixtureCount = countTotalFixtures(segment.id);

        // Afficher: direct (total avec enfants) si différent, sinon juste le total
        QString fixtureText;
        if (totalFixtureCount > directFixtureCount) {
            fixtureText = QString("%1 (%2 total)").arg(directFixtureCount).arg(totalFixtureCount);
        } else {
            fixtureText = QString::number(directFixtureCount);
        }
        segmentsTable->setItem(row, 4, new QTableWidgetItem(fixtureText));
    }

    // Ajuster la taille des colonnes
    segmentsTable->resizeColumnsToContents();

    // Restaurer la sélection si elle était valide
    if (currentRow >= 0 && currentRow < segmentsTable->rowCount()) {
        segmentsTable->setCurrentCell(currentRow, 0);
    }
}

int HydraulicCalculationsWindow::countTotalFixtures(const std::string& segmentId) const
{
    int total = 0;

    // Trouver le segment
    const HydraulicCalc::NetworkSegment* segment = nullptr;
    for (const auto& seg : networkSegments) {
        if (seg.id == segmentId) {
            segment = &seg;
            break;
        }
    }

    if (!segment) return 0;

    // Compter les appareils directs
    for (const auto& fixture : segment->fixtures) {
        total += fixture.quantity;
    }

    // Compter récursivement les appareils des enfants
    for (const auto& child : networkSegments) {
        if (child.parentId == segmentId) {
            total += countTotalFixtures(child.id);
        }
    }

    return total;
}

void HydraulicCalculationsWindow::displayMultiSegmentResults(const HydraulicCalc::NetworkCalculationParameters& networkParams)
{
    // Créer un texte résumé des résultats pour chaque segment
    QString resultsText;

    for (const auto& segment : networkParams.segments) {
        resultsText += "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        resultsText += QString::fromStdString("📍 " + segment.name) + "\n";
        resultsText += "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

        resultsText += QString("Longueur: %1 m | Hauteur: %2 m\n")
            .arg(segment.length, 0, 'f', 1)
            .arg(segment.heightDifference, 0, 'f', 1);

        resultsText += QString("Pression entrée: %1 bar → Pression sortie: %2 bar\n\n")
            .arg(segment.inletPressure, 0, 'f', 2)
            .arg(segment.outletPressure, 0, 'f', 2);

        resultsText += QString("Débit: %1 L/min\n")
            .arg(segment.result.flowRate, 0, 'f', 2);
        resultsText += QString("Diamètre: DN %1 (Ø int. %2 mm)\n")
            .arg(segment.result.nominalDiameter)
            .arg(segment.result.actualDiameter, 0, 'f', 1);
        resultsText += QString("Vitesse: %1 m/s\n")
            .arg(segment.result.velocity, 0, 'f', 2);
        resultsText += QString("Perte de charge: %1 mCE\n")
            .arg(segment.result.pressureDrop, 0, 'f', 2);

        if (segment.result.hasReturn) {
            resultsText += QString("\n🔄 Retour de bouclage:\n");
            resultsText += QString("  Diamètre retour: DN %1 (Ø int. %2 mm)\n")
                .arg(segment.result.returnNominalDiameter)
                .arg(segment.result.returnActualDiameter, 0, 'f', 1);
            resultsText += QString("  Débit retour: %1 L/min\n")
                .arg(segment.result.returnFlowRate, 0, 'f', 2);
            resultsText += QString("  Vitesse retour: %1 m/s\n")
                .arg(segment.result.returnVelocity, 0, 'f', 2);
            resultsText += QString("  Pertes thermiques: %1 W\n")
                .arg(segment.result.heatLoss, 0, 'f', 0);
            resultsText += QString("  Température retour: %1 °C\n")
                .arg(segment.result.returnTemperature, 0, 'f', 1);
        }

        resultsText += QString("\n💡 %1\n\n")
            .arg(QString::fromStdString(segment.result.recommendation));
    }

    recommendationsText->setPlainText(resultsText);

    // Masquer les labels de résultats uniques qui ne sont plus pertinents
    flowRateLabel->setText("-");
    diameterLabel->setText("-");
    velocityLabel->setText("-");
    pressureDropLabel->setText("-");
    availablePressureLabel->setText("-");
    returnResultsGroup->setVisible(false);
}
