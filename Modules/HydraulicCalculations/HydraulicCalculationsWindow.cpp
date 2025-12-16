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
{
    setupUi();
    applyStyle();

    // Connexions
    connect(networkTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HydraulicCalculationsWindow::onNetworkTypeChanged);
    connect(addFixtureButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onAddFixture);
    connect(removeFixtureButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onRemoveFixture);
    connect(calculateButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onCalculate);
    connect(exportButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onExportPDF);
    connect(clearButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onClearResults);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    // Initialisation
    onNetworkTypeChanged(0);
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

    // Dimensions du tronçon
    QGroupBox *dimensionsGroup = new QGroupBox("Dimensions du tronçon", this);
    QFormLayout *dimensionsLayout = new QFormLayout(dimensionsGroup);

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

    paramsLayout->addWidget(dimensionsGroup);

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
        QGroupBox {
            font-weight: bold;
            border: 2px solid #bdc3c7;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 10px;
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
        QComboBox, QSpinBox, QDoubleSpinBox {
            padding: 5px;
            border: 1px solid #bdc3c7;
            border-radius: 4px;
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
}

void HydraulicCalculationsWindow::onRemoveFixture()
{
    int currentRow = fixturesTable->currentRow();
    if (currentRow >= 0 && currentRow < static_cast<int>(fixtures.size())) {
        fixtures.erase(fixtures.begin() + currentRow);
        updateFixtureTable();
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
    if (fixtures.empty()) {
        QMessageBox::warning(this, "Attention",
            "Veuillez ajouter au moins un appareil sanitaire.");
        return;
    }

    // Préparation des paramètres
    HydraulicCalc::CalculationParameters params;

    // Type de réseau
    int networkIndex = networkTypeCombo->currentIndex();
    params.networkType = static_cast<HydraulicCalc::NetworkType>(networkIndex);

    // Matériau
    int materialIndex = materialCombo->currentIndex();
    params.material = static_cast<HydraulicCalc::PipeMaterial>(materialIndex);

    // Dimensions
    params.length = lengthSpin->value();
    params.heightDifference = heightDiffSpin->value();

    // Pressions
    params.supplyPressure = supplyPressureSpin->value();
    params.requiredPressure = requiredPressureSpin->value();

    // Appareils
    params.fixtures = fixtures;

    // Paramètres bouclage si nécessaire
    if (networkIndex == 2) {
        params.loopLength = loopLengthSpin->value();
        params.waterTemperature = waterTempSpin->value();
        params.ambientTemperature = ambientTempSpin->value();
        params.insulationThickness = insulationSpin->value();
    }

    // Calcul
    HydraulicCalc::PipeCalculator calculator;
    HydraulicCalc::PipeSegmentResult result = calculator.calculate(params);

    // Sauvegarde pour export
    lastResult = result;
    lastParams = params;

    // Affichage des résultats
    displayResults(result);

    // Activer le bouton d'export
    exportButton->setEnabled(true);

    // Basculer sur l'onglet résultats
    tabWidget->setCurrentWidget(resultsTab);
}

void HydraulicCalculationsWindow::displayResults(const HydraulicCalc::PipeSegmentResult& result)
{
    flowRateLabel->setText(QString::number(result.flowRate, 'f', 2) + " L/min");
    diameterLabel->setText("DN " + QString::number(result.nominalDiameter) +
                          " (Ø int. " + QString::number(result.actualDiameter, 'f', 1) + " mm)");
    velocityLabel->setText(QString::number(result.velocity, 'f', 2) + " m/s");
    pressureDropLabel->setText(QString::number(result.pressureDrop, 'f', 2) + " mCE");

    double availablePressure = lastParams.supplyPressure - (result.pressureDrop / 10.0);
    availablePressureLabel->setText(QString::number(availablePressure, 'f', 2) + " bar");

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
        "h1 { color: #2c3e50; border-bottom: 3px solid #4472C4; padding-bottom: 10px; }"
        "h2 { color: #4472C4; margin-top: 30px; }"
        "table { border-collapse: collapse; width: 100%; margin: 20px 0; }"
        "th, td { border: 1px solid #bdc3c7; padding: 10px; text-align: left; }"
        "th { background-color: #ecf0f1; font-weight: bold; }"
        ".result { background-color: #e8f5e9; font-weight: bold; }"
        ".info { color: #6c757d; font-size: 0.9em; margin-top: 30px; }"
        "</style></head><body>";

    html += "<h1>Fiche de calcul - Dimensionnement hydraulique</h1>";
    html += "<p><strong>Date:</strong> " + QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm") + "</p>";

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

    html += "<h2>Recommandations</h2>";
    html += "<p>" + QString::fromStdString(lastResult.recommendation) + "</p>";

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
    recommendationsText->clear();

    exportButton->setEnabled(false);

    tabWidget->setCurrentWidget(parametersTab);
}
