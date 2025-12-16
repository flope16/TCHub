#include "HydraulicCalculationsWindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QPrinter>
#include <QTextDocument>
#include <QDateTime>
#include <QPainter>
#include <QPixmap>
#include <QSplitter>

HydraulicCalculationsWindow::HydraulicCalculationsWindow(QWidget *parent)
    : QDialog(parent)
    , mainLayout(nullptr)
    , leftPanel(nullptr)
    , leftPanelLayout(nullptr)
    , leftScrollArea(nullptr)
    , schemaView(nullptr)
    , currentSelectedSegment(nullptr)
    , currentSelectedFixture(nullptr)
    , hasCalculated(false)
{
    setupUi();
    applyStyle();

    // Connexions
    connect(networkTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HydraulicCalculationsWindow::onNetworkTypeChanged);

    // Connexions des outils
    connect(selectToolButton, &QToolButton::clicked, this, &HydraulicCalculationsWindow::onSelectModeActivated);
    connect(addSegmentToolButton, &QToolButton::clicked, this, &HydraulicCalculationsWindow::onAddSegmentModeActivated);
    connect(panToolButton, &QToolButton::clicked, [this]() { schemaView->setInteractionMode(InteractionMode::Pan); });

    // Connexions de la vue schéma
    connect(schemaView, &HydraulicSchemaView::segmentAdded, this, &HydraulicCalculationsWindow::onSegmentAdded);
    connect(schemaView, &HydraulicSchemaView::segmentSelected, this, &HydraulicCalculationsWindow::onSegmentSelected);
    connect(schemaView, &HydraulicSchemaView::segmentRemoved, this, &HydraulicCalculationsWindow::onSegmentRemoved);
    connect(schemaView, &HydraulicSchemaView::fixtureAdded, this, &HydraulicCalculationsWindow::onFixtureAdded);
    connect(schemaView, &HydraulicSchemaView::fixtureSelected, this, &HydraulicCalculationsWindow::onFixtureSelected);
    connect(schemaView, &HydraulicSchemaView::fixtureRemoved, this, &HydraulicCalculationsWindow::onFixtureRemoved);

    // Connexions des boutons d'édition
    connect(editButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onEditSelectedSegment);
    connect(deleteButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onDeleteSelectedSegment);

    // Connexions des actions
    connect(calculateButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onCalculate);
    connect(exportButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onExportPDF);
    connect(resetViewButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onResetView);
    connect(clearButton, &QPushButton::clicked, this, &HydraulicCalculationsWindow::onClear);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    // Initialisation
    onNetworkTypeChanged(0);
    onSelectModeActivated();
}

HydraulicCalculationsWindow::~HydraulicCalculationsWindow()
{
}

void HydraulicCalculationsWindow::setupUi()
{
    setWindowTitle("Module Hydraulique - Schéma de Colonne");
    setMinimumSize(1200, 800);
    resize(1400, 900);

    mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Créer les composants
    createParametersPanel();
    createSchemaView();

    // Splitter pour redimensionner
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    // Panneau gauche dans un scroll area
    leftScrollArea = new QScrollArea();
    leftScrollArea->setWidget(leftPanel);
    leftScrollArea->setWidgetResizable(true);
    leftScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftScrollArea->setMinimumWidth(350);
    leftScrollArea->setMaximumWidth(450);

    splitter->addWidget(leftScrollArea);
    splitter->addWidget(schemaView);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
}

void HydraulicCalculationsWindow::createParametersPanel()
{
    leftPanel = new QWidget();
    leftPanelLayout = new QVBoxLayout(leftPanel);
    leftPanelLayout->setContentsMargins(15, 15, 15, 15);
    leftPanelLayout->setSpacing(15);

    // Titre
    QLabel *titleLabel = new QLabel("💧 Dimensionnement Hydraulique");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; padding: 10px;");
    leftPanelLayout->addWidget(titleLabel);

    // Paramètres généraux
    parametersGroup = new QGroupBox("Paramètres généraux");
    QFormLayout *paramsLayout = new QFormLayout(parametersGroup);

    networkTypeCombo = new QComboBox();
    networkTypeCombo->addItem("Eau froide");
    networkTypeCombo->addItem("Eau chaude sanitaire");
    networkTypeCombo->addItem("ECS avec bouclage");
    paramsLayout->addRow("Type de réseau:", networkTypeCombo);

    materialCombo = new QComboBox();
    materialCombo->addItem("Cuivre");
    materialCombo->addItem("PER");
    materialCombo->addItem("Multicouche");
    materialCombo->addItem("Acier galvanisé");
    paramsLayout->addRow("Matériau:", materialCombo);

    supplyPressureSpin = new QDoubleSpinBox();
    supplyPressureSpin->setRange(0.5, 10.0);
    supplyPressureSpin->setValue(3.0);
    supplyPressureSpin->setSuffix(" bar");
    supplyPressureSpin->setDecimals(1);
    paramsLayout->addRow("Pression alimentation:", supplyPressureSpin);

    requiredPressureSpin = new QDoubleSpinBox();
    requiredPressureSpin->setRange(0.5, 5.0);
    requiredPressureSpin->setValue(1.0);
    requiredPressureSpin->setSuffix(" bar");
    requiredPressureSpin->setDecimals(1);
    paramsLayout->addRow("Pression requise:", requiredPressureSpin);

    leftPanelLayout->addWidget(parametersGroup);

    // Paramètres bouclage ECS
    loopParametersGroup = new QGroupBox("Paramètres bouclage ECS");
    loopParametersGroup->setVisible(false);
    QFormLayout *loopLayout = new QFormLayout(loopParametersGroup);

    loopLengthSpin = new QDoubleSpinBox();
    loopLengthSpin->setRange(1.0, 500.0);
    loopLengthSpin->setValue(50.0);
    loopLengthSpin->setSuffix(" m");
    loopLengthSpin->setDecimals(1);
    loopLayout->addRow("Longueur boucle:", loopLengthSpin);

    waterTempSpin = new QDoubleSpinBox();
    waterTempSpin->setRange(40.0, 80.0);
    waterTempSpin->setValue(60.0);
    waterTempSpin->setSuffix(" °C");
    waterTempSpin->setDecimals(1);
    loopLayout->addRow("Température eau:", waterTempSpin);

    ambientTempSpin = new QDoubleSpinBox();
    ambientTempSpin->setRange(5.0, 35.0);
    ambientTempSpin->setValue(20.0);
    ambientTempSpin->setSuffix(" °C");
    ambientTempSpin->setDecimals(1);
    loopLayout->addRow("Température ambiante:", ambientTempSpin);

    insulationSpin = new QDoubleSpinBox();
    insulationSpin->setRange(6.0, 50.0);
    insulationSpin->setValue(13.0);
    insulationSpin->setSuffix(" mm");
    insulationSpin->setDecimals(0);
    loopLayout->addRow("Épaisseur isolation:", insulationSpin);

    leftPanelLayout->addWidget(loopParametersGroup);

    // Outils
    createToolsPanel();

    // Palette d'appareils
    createFixturePalette();

    // Groupe d'édition
    editGroup = new QGroupBox("Édition");
    QVBoxLayout *editLayout = new QVBoxLayout(editGroup);

    editButton = new QPushButton("✏️ Modifier");
    editButton->setEnabled(false);
    editLayout->addWidget(editButton);

    deleteButton = new QPushButton("🗑️ Supprimer");
    deleteButton->setEnabled(false);
    editLayout->addWidget(deleteButton);

    leftPanelLayout->addWidget(editGroup);

    // Boutons d'action
    QGroupBox *actionsGroup = new QGroupBox("Actions");
    QVBoxLayout *actionsLayout = new QVBoxLayout(actionsGroup);

    calculateButton = new QPushButton("🧮 Calculer");
    calculateButton->setMinimumHeight(40);
    actionsLayout->addWidget(calculateButton);

    exportButton = new QPushButton("📄 Export PDF");
    exportButton->setEnabled(false);
    actionsLayout->addWidget(exportButton);

    resetViewButton = new QPushButton("🔄 Réinitialiser vue");
    actionsLayout->addWidget(resetViewButton);

    clearButton = new QPushButton("🗑️ Tout effacer");
    actionsLayout->addWidget(clearButton);

    closeButton = new QPushButton("✖️ Fermer");
    actionsLayout->addWidget(closeButton);

    leftPanelLayout->addWidget(actionsGroup);

    leftPanelLayout->addStretch();
}

void HydraulicCalculationsWindow::createToolsPanel()
{
    toolsGroup = new QGroupBox("🛠️ Outils");
    QVBoxLayout *toolsLayout = new QVBoxLayout(toolsGroup);

    toolsButtonGroup = new QButtonGroup(this);

    selectToolButton = new QToolButton();
    selectToolButton->setText("👆 Sélectionner");
    selectToolButton->setCheckable(true);
    selectToolButton->setChecked(true);
    selectToolButton->setMinimumHeight(35);
    selectToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolsButtonGroup->addButton(selectToolButton);
    toolsLayout->addWidget(selectToolButton);

    addSegmentToolButton = new QToolButton();
    addSegmentToolButton->setText("➕ Ajouter tronçon");
    addSegmentToolButton->setCheckable(true);
    addSegmentToolButton->setMinimumHeight(35);
    addSegmentToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolsButtonGroup->addButton(addSegmentToolButton);
    toolsLayout->addWidget(addSegmentToolButton);

    panToolButton = new QToolButton();
    panToolButton->setText("🖐️ Déplacer");
    panToolButton->setCheckable(true);
    panToolButton->setMinimumHeight(35);
    panToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolsButtonGroup->addButton(panToolButton);
    toolsLayout->addWidget(panToolButton);

    leftPanelLayout->addWidget(toolsGroup);
}

void HydraulicCalculationsWindow::createFixturePalette()
{
    fixturesGroup = new QGroupBox("🚰 Appareils sanitaires");
    QVBoxLayout *fixturesLayout = new QVBoxLayout(fixturesGroup);

    // Liste des appareils avec icônes
    struct FixtureInfo {
        HydraulicCalc::FixtureType type;
        QString icon;
        QString name;
    };

    std::vector<FixtureInfo> fixtures = {
        {HydraulicCalc::FixtureType::WashBasin, "🚰", "Lavabo"},
        {HydraulicCalc::FixtureType::Sink, "🧽", "Évier"},
        {HydraulicCalc::FixtureType::Shower, "🚿", "Douche"},
        {HydraulicCalc::FixtureType::Bathtub, "🛁", "Baignoire"},
        {HydraulicCalc::FixtureType::WC, "🚽", "WC"},
        {HydraulicCalc::FixtureType::Bidet, "🚰", "Bidet"},
        {HydraulicCalc::FixtureType::WashingMachine, "🧺", "Lave-linge"},
        {HydraulicCalc::FixtureType::Dishwasher, "🍽️", "Lave-vaisselle"},
        {HydraulicCalc::FixtureType::UrinalFlush, "🚹", "Urinoir"},
    };

    for (const auto& fixture : fixtures) {
        QPushButton *btn = new QPushButton(fixture.icon + " " + fixture.name);
        btn->setMinimumHeight(30);
        connect(btn, &QPushButton::clicked, [this, fixture]() {
            onAddFixtureModeActivated(fixture.type);
        });
        fixtureButtons.push_back(btn);
        fixturesLayout->addWidget(btn);
    }

    leftPanelLayout->addWidget(fixturesGroup);
}

void HydraulicCalculationsWindow::createSchemaView()
{
    schemaView = new HydraulicSchemaView();
}

void HydraulicCalculationsWindow::applyStyle()
{
    QString style = R"(
        QDialog {
            background-color: #ffffff;
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
        QToolButton {
            background-color: #ecf0f1;
            color: #2c3e50;
            border: 2px solid #bdc3c7;
            border-radius: 4px;
            padding: 5px 10px;
            text-align: left;
        }
        QToolButton:checked {
            background-color: #4472C4;
            color: white;
            border-color: #365a9e;
        }
        QToolButton:hover:!checked {
            background-color: #d5dbdb;
        }
        QComboBox, QDoubleSpinBox {
            padding: 5px;
            border: 1px solid #bdc3c7;
            border-radius: 4px;
            background-color: #ffffff;
        }
        QScrollArea {
            border: none;
            background-color: #f8f9fa;
        }
    )";
    setStyleSheet(style);
}

// ===== SLOTS =====

void HydraulicCalculationsWindow::onNetworkTypeChanged(int index)
{
    loopParametersGroup->setVisible(index == 2); // ECS avec bouclage
}

void HydraulicCalculationsWindow::onSelectModeActivated()
{
    schemaView->setInteractionMode(InteractionMode::Select);
    selectToolButton->setChecked(true);
}

void HydraulicCalculationsWindow::onAddSegmentModeActivated()
{
    // Créer un dialogue pour saisir les paramètres du segment
    HydraulicCalc::NetworkSegment newSegment;

    if (showSegmentDialog(newSegment, false)) {
        // Ajouter le segment aux données
        newSegment.id = "seg_" + std::to_string(networkSegments.size() + 1);
        networkSegments.push_back(newSegment);

        // Demander à l'utilisateur de dessiner le segment sur le schéma
        // Par défaut, on crée un segment vertical
        QPointF startPos(100, 100 + networkSegments.size() * 150);
        QPointF endPos = startPos + QPointF(0, newSegment.length * 2);  // Échelle approximative

        // Créer le segment graphique
        GraphicPipeSegment* graphicSegment = schemaView->addSegment(&networkSegments.back(), startPos, endPos);

        // Retourner en mode sélection
        onSelectModeActivated();
    }
}

void HydraulicCalculationsWindow::onAddFixtureModeActivated(HydraulicCalc::FixtureType type)
{
    // Demander la quantité
    bool ok;
    int quantity = QInputDialog::getInt(this, "Quantité d'appareils",
                                        "Combien d'appareils de ce type ?",
                                        1, 1, 100, 1, &ok);
    if (!ok) return;

    // Configurer la vue pour placer une fixture
    schemaView->setFixtureToPlace(type, quantity);
    schemaView->setInteractionMode(InteractionMode::AddFixture);

    QMessageBox::information(this, "Placement d'appareil",
                           "Cliquez sur un tronçon pour placer l'appareil.");
}

void HydraulicCalculationsWindow::onSegmentAdded(GraphicPipeSegment* segment)
{
    hasCalculated = false;
    exportButton->setEnabled(false);
}

void HydraulicCalculationsWindow::onSegmentSelected(GraphicPipeSegment* segment)
{
    currentSelectedSegment = segment;
    currentSelectedFixture = nullptr;
    editButton->setEnabled(true);
    deleteButton->setEnabled(true);
}

void HydraulicCalculationsWindow::onSegmentRemoved(GraphicPipeSegment* segment)
{
    // Retirer des données
    for (auto it = networkSegments.begin(); it != networkSegments.end(); ++it) {
        if (&(*it) == segment->getSegmentData()) {
            networkSegments.erase(it);
            break;
        }
    }

    currentSelectedSegment = nullptr;
    editButton->setEnabled(false);
    deleteButton->setEnabled(false);
    hasCalculated = false;
    exportButton->setEnabled(false);
}

void HydraulicCalculationsWindow::onEditSelectedSegment()
{
    if (!currentSelectedSegment) return;

    HydraulicCalc::NetworkSegment* segmentData = currentSelectedSegment->getSegmentData();
    if (showSegmentDialog(*segmentData, true)) {
        currentSelectedSegment->updateDisplay();
        hasCalculated = false;
        exportButton->setEnabled(false);
    }
}

void HydraulicCalculationsWindow::onDeleteSelectedSegment()
{
    if (!currentSelectedSegment) return;

    auto reply = QMessageBox::question(this, "Confirmation",
        "Supprimer ce tronçon et tous ses appareils ?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        schemaView->removeSegment(currentSelectedSegment);
        currentSelectedSegment = nullptr;
        editButton->setEnabled(false);
        deleteButton->setEnabled(false);
    }
}

void HydraulicCalculationsWindow::onFixtureAdded(FixturePoint* fixture, GraphicPipeSegment* segment)
{
    hasCalculated = false;
    exportButton->setEnabled(false);
}

void HydraulicCalculationsWindow::onFixtureSelected(FixturePoint* fixture)
{
    currentSelectedFixture = fixture;
    currentSelectedSegment = nullptr;
    editButton->setEnabled(true);
    deleteButton->setEnabled(true);
}

void HydraulicCalculationsWindow::onFixtureRemoved(FixturePoint* fixture)
{
    currentSelectedFixture = nullptr;
    editButton->setEnabled(false);
    deleteButton->setEnabled(false);
    hasCalculated = false;
    exportButton->setEnabled(false);
}

void HydraulicCalculationsWindow::onEditSelectedFixture()
{
    if (!currentSelectedFixture) {
        onEditSelectedSegment();
        return;
    }

    if (showFixtureDialog(currentSelectedFixture)) {
        currentSelectedFixture->updateDisplay();
        hasCalculated = false;
        exportButton->setEnabled(false);
    }
}

void HydraulicCalculationsWindow::onDeleteSelectedFixture()
{
    if (!currentSelectedFixture) {
        onDeleteSelectedSegment();
        return;
    }

    // Supprimer la fixture
    for (auto* segment : schemaView->getSegments()) {
        segment->removeFixturePoint(currentSelectedFixture);
    }

    schemaView->getScene()->removeItem(currentSelectedFixture);
    delete currentSelectedFixture;
    currentSelectedFixture = nullptr;
    editButton->setEnabled(false);
    deleteButton->setEnabled(false);
}

void HydraulicCalculationsWindow::onCalculate()
{
    if (networkSegments.empty()) {
        QMessageBox::warning(this, "Attention", "Ajoutez au moins un tronçon avant de calculer.");
        return;
    }

    // Mettre à jour les données des segments avec les fixtures graphiques
    updateNetworkSegmentsData();

    // Effectuer les calculs
    performCalculations();

    // Mettre à jour l'affichage des résultats sur le schéma
    schemaView->updateSegmentResults();

    hasCalculated = true;
    exportButton->setEnabled(true);

    QMessageBox::information(this, "Calcul terminé",
                           "Les résultats sont affichés sur le schéma.");
}

void HydraulicCalculationsWindow::performCalculations()
{
    // Préparer les paramètres de calcul du réseau
    HydraulicCalc::NetworkCalculationParameters networkParams;
    networkParams.networkType = static_cast<HydraulicCalc::NetworkType>(networkTypeCombo->currentIndex());
    networkParams.material = static_cast<HydraulicCalc::PipeMaterial>(materialCombo->currentIndex());
    networkParams.supplyPressure = supplyPressureSpin->value();
    networkParams.requiredPressure = requiredPressureSpin->value();
    networkParams.segments = networkSegments;

    // Paramètres bouclage si nécessaire
    if (networkTypeCombo->currentIndex() == 2) {
        networkParams.loopLength = loopLengthSpin->value();
        networkParams.waterTemperature = waterTempSpin->value();
        networkParams.ambientTemperature = ambientTempSpin->value();
        networkParams.insulationThickness = insulationSpin->value();
    }

    // Calcul
    calculator.calculateNetwork(networkParams);

    // Récupérer les résultats
    networkSegments = networkParams.segments;
}

void HydraulicCalculationsWindow::updateNetworkSegmentsData()
{
    // Mettre à jour les fixtures de chaque segment à partir des FixturePoint graphiques
    auto& graphicSegments = schemaView->getSegments();

    for (size_t i = 0; i < graphicSegments.size() && i < networkSegments.size(); ++i) {
        auto* graphicSegment = graphicSegments[i];
        auto& segmentData = networkSegments[i];

        // Effacer les anciennes fixtures
        segmentData.fixtures.clear();

        // Ajouter les fixtures depuis les points graphiques
        for (auto* fixturePoint : graphicSegment->getFixturePoints()) {
            segmentData.fixtures.push_back(fixturePoint->toFixture());
        }
    }
}

void HydraulicCalculationsWindow::onExportPDF()
{
    if (!hasCalculated) {
        QMessageBox::warning(this, "Attention", "Effectuez d'abord un calcul avant d'exporter.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        "Exporter le schéma", "", "Fichier PDF (*.pdf)");

    if (fileName.isEmpty()) return;

    if (!fileName.endsWith(".pdf", Qt::CaseInsensitive)) {
        fileName += ".pdf";
    }

    // Générer le HTML
    QString html = generatePDFHtml();

    // Créer le PDF
    QTextDocument document;
    document.setHtml(html);

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageSize(QPageSize::A4);
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

    // Imprimer le document HTML
    document.print(&printer);

    // Ajouter le schéma graphique sur une page séparée
    // (Note: pour simplifier, on n'ajoute que le HTML ici.
    // L'ajout de l'image du schéma nécessiterait QPainter)

    QMessageBox::information(this, "Export réussi",
        "Le schéma a été exporté:\n" + fileName);
}

QString HydraulicCalculationsWindow::generatePDFHtml()
{
    QString html = "<html><head><style>"
        "body { font-family: Arial, sans-serif; margin: 40px; }"
        "h1 { color: #2c3e50; border-bottom: 3px solid #4472C4; padding-bottom: 10px; }"
        "h2 { color: #4472C4; margin-top: 30px; }"
        "h3 { color: #5a8fd1; margin-top: 20px; border-left: 4px solid #4472C4; padding-left: 10px; }"
        "table { border-collapse: collapse; width: 100%; margin: 20px 0; }"
        "th, td { border: 1px solid #bdc3c7; padding: 10px; text-align: left; }"
        "th { background-color: #ecf0f1; font-weight: bold; }"
        ".result { background-color: #e8f5e9; font-weight: bold; }"
        ".info { color: #6c757d; font-size: 0.9em; margin-top: 30px; }"
        "</style></head><body>";

    html += "<h1>Schéma de Colonne - Dimensionnement Hydraulique</h1>";
    html += "<p><strong>Date:</strong> " + QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm") + "</p>";

    // Paramètres généraux
    html += "<h2>Paramètres généraux</h2>";
    html += "<table>";
    html += "<tr><th>Paramètre</th><th>Valeur</th></tr>";
    html += "<tr><td>Type de réseau</td><td>" + networkTypeCombo->currentText() + "</td></tr>";
    html += "<tr><td>Matériau</td><td>" + materialCombo->currentText() + "</td></tr>";
    html += "<tr><td>Pression d'alimentation</td><td>" + QString::number(supplyPressureSpin->value(), 'f', 1) + " bar</td></tr>";
    html += "<tr><td>Pression requise</td><td>" + QString::number(requiredPressureSpin->value(), 'f', 1) + " bar</td></tr>";
    html += "<tr><td>Nombre de tronçons</td><td>" + QString::number(networkSegments.size()) + "</td></tr>";
    html += "</table>";

    // Bouclage si applicable
    if (networkTypeCombo->currentIndex() == 2) {
        html += "<h2>Paramètres bouclage ECS</h2>";
        html += "<table>";
        html += "<tr><th>Paramètre</th><th>Valeur</th></tr>";
        html += "<tr><td>Longueur boucle</td><td>" + QString::number(loopLengthSpin->value(), 'f', 1) + " m</td></tr>";
        html += "<tr><td>Température eau</td><td>" + QString::number(waterTempSpin->value(), 'f', 1) + " °C</td></tr>";
        html += "<tr><td>Température ambiante</td><td>" + QString::number(ambientTempSpin->value(), 'f', 1) + " °C</td></tr>";
        html += "<tr><td>Isolation</td><td>" + QString::number(insulationSpin->value(), 'f', 0) + " mm</td></tr>";
        html += "</table>";
    }

    // Résultats par segment
    for (const auto& segment : networkSegments) {
        html += "<h2>Tronçon: " + QString::fromStdString(segment.name) + "</h2>";

        // Paramètres du segment
        html += "<h3>Paramètres</h3>";
        html += "<table>";
        html += "<tr><th>Paramètre</th><th>Valeur</th></tr>";
        html += "<tr><td>Longueur</td><td>" + QString::number(segment.length, 'f', 1) + " m</td></tr>";
        html += "<tr><td>Différence de hauteur</td><td>" + QString::number(segment.heightDifference, 'f', 1) + " m</td></tr>";
        html += "<tr><td>Pression entrée</td><td>" + QString::number(segment.inletPressure, 'f', 2) + " bar</td></tr>";
        html += "<tr><td>Pression sortie</td><td>" + QString::number(segment.outletPressure, 'f', 2) + " bar</td></tr>";
        html += "</table>";

        // Appareils
        if (!segment.fixtures.empty()) {
            html += "<h3>Appareils sanitaires</h3>";
            html += "<table>";
            html += "<tr><th>Type</th><th>Quantité</th><th>Débit unitaire (L/min)</th></tr>";
            for (const auto& fixture : segment.fixtures) {
                html += "<tr>";
                html += "<td>" + QString::fromStdString(HydraulicCalc::PipeCalculator::getFixtureName(fixture.type)) + "</td>";
                html += "<td>" + QString::number(fixture.quantity) + "</td>";
                html += "<td>" + QString::number(fixture.flowRate, 'f', 1) + "</td>";
                html += "</tr>";
            }
            html += "</table>";
        }

        // Résultats
        html += "<h3>Résultats du dimensionnement</h3>";
        html += "<table>";
        html += "<tr><th>Résultat</th><th>Valeur</th></tr>";
        html += "<tr class='result'><td>Débit</td><td>" + QString::number(segment.result.flowRate, 'f', 2) + " L/min</td></tr>";
        html += "<tr class='result'><td>Diamètre nominal</td><td>DN " + QString::number(segment.result.nominalDiameter) +
                " (Ø " + QString::number(segment.result.actualDiameter, 'f', 1) + " mm)</td></tr>";
        html += "<tr><td>Vitesse</td><td>" + QString::number(segment.result.velocity, 'f', 2) + " m/s</td></tr>";
        html += "<tr><td>Perte de charge</td><td>" + QString::number(segment.result.pressureDrop, 'f', 2) + " mCE</td></tr>";
        html += "</table>";

        // Retour si applicable
        if (segment.result.hasReturn) {
            html += "<h3>Retour de bouclage</h3>";
            html += "<table>";
            html += "<tr><th>Résultat</th><th>Valeur</th></tr>";
            html += "<tr class='result'><td>Diamètre retour</td><td>DN " + QString::number(segment.result.returnNominalDiameter) +
                    " (Ø " + QString::number(segment.result.returnActualDiameter, 'f', 1) + " mm)</td></tr>";
            html += "<tr><td>Débit retour</td><td>" + QString::number(segment.result.returnFlowRate, 'f', 2) + " L/min</td></tr>";
            html += "<tr><td>Vitesse retour</td><td>" + QString::number(segment.result.returnVelocity, 'f', 2) + " m/s</td></tr>";
            html += "<tr><td>Pertes thermiques</td><td>" + QString::number(segment.result.heatLoss, 'f', 0) + " W</td></tr>";
            html += "<tr><td>Température retour</td><td>" + QString::number(segment.result.returnTemperature, 'f', 1) + " °C</td></tr>";
            html += "</table>";
        }

        html += "<p><strong>Recommandations:</strong> " + QString::fromStdString(segment.result.recommendation) + "</p>";
    }

    html += "<div class='info'>";
    html += "<p><em>Calculs selon DTU 60.11 et formules de Darcy-Weisbach.</em></p>";
    html += "<p><em>Document généré par TC Hub - Module hydraulique graphique</em></p>";
    html += "</div>";

    html += "</body></html>";
    return html;
}

void HydraulicCalculationsWindow::onClear()
{
    auto reply = QMessageBox::question(this, "Confirmation",
        "Effacer tout le schéma ?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        schemaView->clearAllSegments();
        networkSegments.clear();
        currentSelectedSegment = nullptr;
        currentSelectedFixture = nullptr;
        hasCalculated = false;
        exportButton->setEnabled(false);
        editButton->setEnabled(false);
        deleteButton->setEnabled(false);
    }
}

void HydraulicCalculationsWindow::onResetView()
{
    schemaView->resetView();
}

bool HydraulicCalculationsWindow::showSegmentDialog(HydraulicCalc::NetworkSegment& segment, bool isEdit)
{
    QDialog dialog(this);
    dialog.setWindowTitle(isEdit ? "Modifier le tronçon" : "Nouveau tronçon");
    dialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit *nameEdit = new QLineEdit(&dialog);
    nameEdit->setText(QString::fromStdString(segment.name));
    if (!isEdit) {
        nameEdit->setText("Tronçon " + QString::number(networkSegments.size() + 1));
    }
    formLayout->addRow("Nom:", nameEdit);

    QComboBox *parentCombo = new QComboBox(&dialog);
    parentCombo->addItem("(Aucun - Segment racine)", "");
    for (const auto& seg : networkSegments) {
        if (&seg != &segment) {  // Ne pas pouvoir se choisir comme parent
            parentCombo->addItem(QString::fromStdString(seg.name), QString::fromStdString(seg.id));
        }
    }
    if (isEdit) {
        int idx = parentCombo->findData(QString::fromStdString(segment.parentId));
        if (idx >= 0) parentCombo->setCurrentIndex(idx);
    }
    formLayout->addRow("Parent:", parentCombo);

    QDoubleSpinBox *lengthSpin = new QDoubleSpinBox(&dialog);
    lengthSpin->setRange(0.1, 1000.0);
    lengthSpin->setValue(isEdit ? segment.length : 10.0);
    lengthSpin->setSuffix(" m");
    lengthSpin->setDecimals(1);
    formLayout->addRow("Longueur:", lengthSpin);

    QDoubleSpinBox *heightSpin = new QDoubleSpinBox(&dialog);
    heightSpin->setRange(-100.0, 100.0);
    heightSpin->setValue(isEdit ? segment.heightDifference : 3.0);
    heightSpin->setSuffix(" m");
    heightSpin->setDecimals(1);
    formLayout->addRow("Hauteur:", heightSpin);

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
        return true;
    }

    return false;
}

bool HydraulicCalculationsWindow::showFixtureDialog(FixturePoint* fixture)
{
    if (!fixture) return false;

    bool ok;
    int newQuantity = QInputDialog::getInt(this, "Modifier l'appareil",
                                          "Quantité:",
                                          fixture->getQuantity(), 1, 100, 1, &ok);
    if (ok) {
        fixture->setQuantity(newQuantity);
        return true;
    }

    return false;
}
