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
#include <QPalette>
#include <QApplication>
#include <QKeyEvent>
#include <cmath>
#include <cstdint>

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
    // Configurer une palette globale pour les dialogues
    QPalette dialogPalette;
    dialogPalette.setColor(QPalette::Window, QColor("#ffffff"));
    dialogPalette.setColor(QPalette::WindowText, QColor("#1e293b"));
    dialogPalette.setColor(QPalette::Base, QColor("#ffffff"));
    dialogPalette.setColor(QPalette::Text, QColor("#1e293b"));
    dialogPalette.setColor(QPalette::Button, QColor("#3b82f6"));
    dialogPalette.setColor(QPalette::ButtonText, QColor("#ffffff"));
    qApp->setPalette(dialogPalette);

    setObjectName("HydraulicCalculationsWindow");

    // Créer le filtre pour bloquer la molette sur les spinbox
    wheelFilter = new SpinBoxWheelFilter(this);

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
    connect(schemaView, &HydraulicSchemaView::segmentDrawingComplete, this, &HydraulicCalculationsWindow::onSegmentDrawingComplete);
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

void HydraulicCalculationsWindow::keyPressEvent(QKeyEvent* event)
{
    // Désactiver la fermeture par Échap
    if (event->key() == Qt::Key_Escape) {
        event->ignore();  // Ignorer l'événement Échap
        return;
    }

    // Propager les autres touches normalement
    QDialog::keyPressEvent(event);
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
    leftScrollArea->setMinimumWidth(380);
    leftScrollArea->setMaximumWidth(420);

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
    leftPanelLayout->setContentsMargins(20, 20, 20, 20);
    leftPanelLayout->setSpacing(16);

    // Titre moderne avec style
    QLabel *titleLabel = new QLabel("Dimensionnement Hydraulique");
    titleLabel->setObjectName("mainTitle");
    titleLabel->setStyleSheet(
        "#mainTitle {"
        "   font-size: 20px;"
        "   font-weight: 700;"
        "   color: #1e3a8a;"
        "   padding: 12px 0px;"
        "   letter-spacing: -0.5px;"
        "}"
    );
    leftPanelLayout->addWidget(titleLabel);

    // Paramètres généraux - Style carte moderne
    parametersGroup = new QGroupBox("Configuration");
    parametersGroup->setObjectName("modernCard");
    QFormLayout *paramsLayout = new QFormLayout(parametersGroup);
    paramsLayout->setSpacing(12);
    paramsLayout->setLabelAlignment(Qt::AlignLeft);
    paramsLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);

    // Labels avec style moderne
    QLabel *networkLabel = new QLabel("Type de réseau");
    networkLabel->setObjectName("formLabel");
    networkTypeCombo = new QComboBox();
    networkTypeCombo->setObjectName("modernCombo");
    networkTypeCombo->addItem("Eau froide");
    networkTypeCombo->addItem("Eau chaude sanitaire");
    networkTypeCombo->addItem("ECS avec bouclage");
    paramsLayout->addRow(networkLabel, networkTypeCombo);

    QLabel *materialLabel = new QLabel("Matériau");
    materialLabel->setObjectName("formLabel");
    materialCombo = new QComboBox();
    materialCombo->setObjectName("modernCombo");
    materialCombo->addItem("Cuivre");
    materialCombo->addItem("PER");
    materialCombo->addItem("Multicouche");
    materialCombo->addItem("Acier galvanisé");
    paramsLayout->addRow(materialLabel, materialCombo);

    QLabel *supplyLabel = new QLabel("Pression alimentation");
    supplyLabel->setObjectName("formLabel");
    supplyPressureSpin = new QDoubleSpinBox();
    supplyPressureSpin->setObjectName("modernSpin");
    supplyPressureSpin->setRange(0.5, 10.0);
    supplyPressureSpin->setValue(3.0);
    supplyPressureSpin->setSuffix(" bar");
    supplyPressureSpin->setDecimals(1);
    supplyPressureSpin->installEventFilter(wheelFilter);  // Désactiver molette
    paramsLayout->addRow(supplyLabel, supplyPressureSpin);

    QLabel *requiredLabel = new QLabel("Pression requise");
    requiredLabel->setObjectName("formLabel");
    requiredPressureSpin = new QDoubleSpinBox();
    requiredPressureSpin->setObjectName("modernSpin");
    requiredPressureSpin->setRange(0.5, 5.0);
    requiredPressureSpin->setValue(1.0);
    requiredPressureSpin->setSuffix(" bar");
    requiredPressureSpin->setDecimals(1);
    requiredPressureSpin->installEventFilter(wheelFilter);  // Désactiver molette
    paramsLayout->addRow(requiredLabel, requiredPressureSpin);

    leftPanelLayout->addWidget(parametersGroup);

    // Paramètres bouclage ECS - Style carte moderne
    loopParametersGroup = new QGroupBox("Bouclage ECS");
    loopParametersGroup->setObjectName("modernCard");
    loopParametersGroup->setVisible(false);
    QFormLayout *loopLayout = new QFormLayout(loopParametersGroup);
    loopLayout->setSpacing(12);

    QLabel *loopLengthLabel = new QLabel("Longueur boucle");
    loopLengthLabel->setObjectName("formLabel");
    loopLengthSpin = new QDoubleSpinBox();
    loopLengthSpin->setObjectName("modernSpin");
    loopLengthSpin->setRange(1.0, 500.0);
    loopLengthSpin->setValue(50.0);
    loopLengthSpin->setSuffix(" m");
    loopLengthSpin->setDecimals(1);
    loopLengthSpin->installEventFilter(wheelFilter);  // Désactiver molette
    loopLayout->addRow(loopLengthLabel, loopLengthSpin);

    QLabel *waterTempLabel = new QLabel("Température eau");
    waterTempLabel->setObjectName("formLabel");
    waterTempSpin = new QDoubleSpinBox();
    waterTempSpin->setObjectName("modernSpin");
    waterTempSpin->setRange(40.0, 80.0);
    waterTempSpin->setValue(60.0);
    waterTempSpin->setSuffix(" C");
    waterTempSpin->setDecimals(1);
    waterTempSpin->installEventFilter(wheelFilter);  // Désactiver molette
    loopLayout->addRow(waterTempLabel, waterTempSpin);

    QLabel *ambientTempLabel = new QLabel("Température ambiante");
    ambientTempLabel->setObjectName("formLabel");
    ambientTempSpin = new QDoubleSpinBox();
    ambientTempSpin->setObjectName("modernSpin");
    ambientTempSpin->setRange(5.0, 35.0);
    ambientTempSpin->setValue(20.0);
    ambientTempSpin->setSuffix(" C");
    ambientTempSpin->setDecimals(1);
    ambientTempSpin->installEventFilter(wheelFilter);  // Désactiver molette
    loopLayout->addRow(ambientTempLabel, ambientTempSpin);

    QLabel *insulationLabel = new QLabel("Épaisseur isolation");
    insulationLabel->setObjectName("formLabel");
    insulationSpin = new QDoubleSpinBox();
    insulationSpin->setObjectName("modernSpin");
    insulationSpin->setRange(6.0, 50.0);
    insulationSpin->setValue(13.0);
    insulationSpin->setSuffix(" mm");
    insulationSpin->setDecimals(0);
    insulationSpin->installEventFilter(wheelFilter);  // Désactiver molette
    loopLayout->addRow(insulationLabel, insulationSpin);

    leftPanelLayout->addWidget(loopParametersGroup);

    // Outils
    createToolsPanel();

    // Palette d'appareils
    createFixturePalette();

    // Groupe d'édition - Style moderne
    editGroup = new QGroupBox("Édition");
    editGroup->setObjectName("modernCard");
    QVBoxLayout *editLayout = new QVBoxLayout(editGroup);
    editLayout->setSpacing(8);

    editButton = new QPushButton("Modifier");
    editButton->setObjectName("secondaryButton");
    editButton->setEnabled(false);
    editButton->setMinimumHeight(36);
    editLayout->addWidget(editButton);

    deleteButton = new QPushButton("Supprimer");
    deleteButton->setObjectName("dangerButton");
    deleteButton->setEnabled(false);
    deleteButton->setMinimumHeight(36);
    editLayout->addWidget(deleteButton);

    leftPanelLayout->addWidget(editGroup);

    // Boutons d'action - Style moderne sans groupbox
    QLabel *actionsLabel = new QLabel("Actions");
    actionsLabel->setObjectName("sectionLabel");
    actionsLabel->setStyleSheet(
        "#sectionLabel {"
        "   font-size: 13px;"
        "   font-weight: 600;"
        "   color: #64748b;"
        "   text-transform: uppercase;"
        "   letter-spacing: 0.5px;"
        "   margin-top: 8px;"
        "}"
    );
    leftPanelLayout->addWidget(actionsLabel);

    calculateButton = new QPushButton("Calculer");
    calculateButton->setObjectName("primaryButton");
    calculateButton->setMinimumHeight(44);
    leftPanelLayout->addWidget(calculateButton);

    exportButton = new QPushButton("Export PDF");
    exportButton->setObjectName("secondaryButton");
    exportButton->setEnabled(false);
    exportButton->setMinimumHeight(36);
    leftPanelLayout->addWidget(exportButton);

    resetViewButton = new QPushButton("Réinitialiser vue");
    resetViewButton->setObjectName("secondaryButton");
    resetViewButton->setMinimumHeight(36);
    leftPanelLayout->addWidget(resetViewButton);

    clearButton = new QPushButton("Tout effacer");
    clearButton->setObjectName("dangerButton");
    clearButton->setMinimumHeight(36);
    leftPanelLayout->addWidget(clearButton);

    closeButton = new QPushButton("Fermer");
    closeButton->setObjectName("ghostButton");
    closeButton->setMinimumHeight(36);
    leftPanelLayout->addWidget(closeButton);

    leftPanelLayout->addStretch();
}

void HydraulicCalculationsWindow::createToolsPanel()
{
    toolsGroup = new QGroupBox("Outils");
    toolsGroup->setObjectName("modernCard");
    QVBoxLayout *toolsLayout = new QVBoxLayout(toolsGroup);
    toolsLayout->setSpacing(8);

    toolsButtonGroup = new QButtonGroup(this);

    selectToolButton = new QToolButton();
    selectToolButton->setText("Sélectionner");
    selectToolButton->setObjectName("toolButton");
    selectToolButton->setCheckable(true);
    selectToolButton->setChecked(true);
    selectToolButton->setMinimumHeight(38);
    selectToolButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    toolsButtonGroup->addButton(selectToolButton);
    toolsLayout->addWidget(selectToolButton);

    addSegmentToolButton = new QToolButton();
    addSegmentToolButton->setText("Ajouter tronçon");
    addSegmentToolButton->setObjectName("toolButton");
    addSegmentToolButton->setCheckable(true);
    addSegmentToolButton->setMinimumHeight(38);
    addSegmentToolButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    toolsButtonGroup->addButton(addSegmentToolButton);
    toolsLayout->addWidget(addSegmentToolButton);

    panToolButton = new QToolButton();
    panToolButton->setText("Déplacer");
    panToolButton->setObjectName("toolButton");
    panToolButton->setCheckable(true);
    panToolButton->setMinimumHeight(38);
    panToolButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    toolsButtonGroup->addButton(panToolButton);
    toolsLayout->addWidget(panToolButton);

    leftPanelLayout->addWidget(toolsGroup);
}

void HydraulicCalculationsWindow::createFixturePalette()
{
    fixturesGroup = new QGroupBox("Appareils sanitaires");
    fixturesGroup->setObjectName("modernCard");
    QVBoxLayout *fixturesLayout = new QVBoxLayout(fixturesGroup);
    fixturesLayout->setSpacing(6);

    // Liste des appareils sans emojis - style moderne
    struct FixtureInfo {
        HydraulicCalc::FixtureType type;
        QString name;
    };

    std::vector<FixtureInfo> fixtures = {
        {HydraulicCalc::FixtureType::WashBasin, "Lavabo"},
        {HydraulicCalc::FixtureType::Sink, "Évier"},
        {HydraulicCalc::FixtureType::Shower, "Douche"},
        {HydraulicCalc::FixtureType::Bathtub, "Baignoire"},
        {HydraulicCalc::FixtureType::WC, "WC"},
        {HydraulicCalc::FixtureType::Bidet, "Bidet"},
        {HydraulicCalc::FixtureType::WashingMachine, "Lave-linge"},
        {HydraulicCalc::FixtureType::Dishwasher, "Lave-vaisselle"},
        {HydraulicCalc::FixtureType::UrinalFlush, "Urinoir"},
    };

    for (const auto& fixture : fixtures) {
        QPushButton *btn = new QPushButton(fixture.name);
        btn->setObjectName("fixtureButton");
        btn->setMinimumHeight(32);
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
        /* === LAYOUT PRINCIPAL === */
        QDialog {
            background-color: #f8fafc;
            color: #1e293b;
        }

        /* === FENÊTRE PRINCIPALE === */
        QDialog#HydraulicCalculationsWindow {
            background-color: #f8fafc;
        }

        /* === CARTES MODERNES === */
        QGroupBox#modernCard {
            font-weight: 600;
            font-size: 12pt;
            border: 1px solid #e2e8f0;
            border-radius: 12px;
            margin-top: 8px;
            padding: 16px 12px 12px 12px;
            background-color: #ffffff;
            color: #334155;
        }
        QGroupBox#modernCard::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0px 8px;
            background-color: transparent;
            color: #475569;
            font-weight: 600;
            font-size: 11pt;
        }

        /* === LABELS === */
        QLabel#formLabel {
            color: #64748b;
            background-color: transparent;
            font-size: 10pt;
            font-weight: 500;
            padding: 2px 0px;
        }

        /* === BOUTON PRIMAIRE === */
        QPushButton#primaryButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #3b82f6, stop:1 #2563eb);
            color: white;
            border: none;
            border-radius: 8px;
            padding: 12px 20px;
            font-weight: 600;
            font-size: 11pt;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        QPushButton#primaryButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2563eb, stop:1 #1d4ed8);
        }
        QPushButton#primaryButton:pressed {
            background: #1e40af;
        }

        /* === BOUTONS SECONDAIRES === */
        QPushButton#secondaryButton {
            background-color: #f1f5f9;
            color: #475569;
            border: 1px solid #cbd5e1;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: 500;
            font-size: 10pt;
        }
        QPushButton#secondaryButton:hover {
            background-color: #e2e8f0;
            border-color: #94a3b8;
        }
        QPushButton#secondaryButton:pressed {
            background-color: #cbd5e1;
        }
        QPushButton#secondaryButton:disabled {
            background-color: #f8fafc;
            color: #cbd5e1;
            border-color: #e2e8f0;
        }

        /* === BOUTONS DANGER === */
        QPushButton#dangerButton {
            background-color: #fef2f2;
            color: #dc2626;
            border: 1px solid #fecaca;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: 500;
            font-size: 10pt;
        }
        QPushButton#dangerButton:hover {
            background-color: #fee2e2;
            border-color: #fca5a5;
        }
        QPushButton#dangerButton:pressed {
            background-color: #fecaca;
        }
        QPushButton#dangerButton:disabled {
            background-color: #f8fafc;
            color: #cbd5e1;
            border-color: #e2e8f0;
        }

        /* === BOUTONS GHOST === */
        QPushButton#ghostButton {
            background-color: transparent;
            color: #64748b;
            border: 1px solid #cbd5e1;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: 500;
            font-size: 10pt;
        }
        QPushButton#ghostButton:hover {
            background-color: #f8fafc;
            color: #475569;
        }
        QPushButton#ghostButton:pressed {
            background-color: #f1f5f9;
        }

        /* === BOUTONS D'OUTILS === */
        QToolButton#toolButton {
            background-color: #f8fafc;
            color: #475569;
            border: 2px solid #e2e8f0;
            border-radius: 8px;
            padding: 10px 16px;
            text-align: center;
            font-weight: 500;
            font-size: 10pt;
        }
        QToolButton#toolButton:hover {
            background-color: #eff6ff;
            border-color: #93c5fd;
            color: #1e40af;
        }
        QToolButton#toolButton:checked {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #3b82f6, stop:1 #2563eb);
            border-color: #1d4ed8;
            color: white;
            font-weight: 600;
        }
        QToolButton#toolButton:pressed {
            background-color: #dbeafe;
        }

        /* === BOUTONS APPAREILS === */
        QPushButton#fixtureButton {
            background-color: #fafbfc;
            color: #374151;
            border: 1px solid #e5e7eb;
            border-radius: 6px;
            padding: 8px 12px;
            font-weight: 500;
            font-size: 10pt;
            text-align: left;
        }
        QPushButton#fixtureButton:hover {
            background-color: #eff6ff;
            border-color: #93c5fd;
            color: #1e40af;
        }
        QPushButton#fixtureButton:pressed {
            background-color: #dbeafe;
        }

        /* === CHAMPS DE SAISIE === */
        QComboBox#modernCombo, QDoubleSpinBox#modernSpin, QLineEdit {
            padding: 9px 12px;
            border: 2px solid #e2e8f0;
            border-radius: 8px;
            background-color: #ffffff;
            color: #1e293b;
            font-size: 10pt;
            font-weight: 500;
            selection-background-color: #3b82f6;
        }
        QComboBox#modernCombo:focus, QDoubleSpinBox#modernSpin:focus, QLineEdit:focus {
            border: 2px solid #3b82f6;
            background-color: #ffffff;
        }
        QComboBox#modernCombo:hover, QDoubleSpinBox#modernSpin:hover, QLineEdit:hover {
            border-color: #94a3b8;
        }

        /* === COMBOBOX DROPDOWN === */
        QComboBox#modernCombo::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 36px;
            border-left: 2px solid #e2e8f0;
            background-color: #f8fafc;
            border-top-right-radius: 8px;
            border-bottom-right-radius: 8px;
        }
        QComboBox#modernCombo::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 6px solid #64748b;
            margin: 0px;
        }
        QComboBox#modernCombo::drop-down:hover {
            background-color: #eff6ff;
        }
        QComboBox#modernCombo QAbstractItemView {
            border: 2px solid #e2e8f0;
            border-radius: 8px;
            background-color: #ffffff;
            selection-background-color: #3b82f6;
            selection-color: white;
            padding: 4px;
        }

        /* === SPINBOX === */
        QDoubleSpinBox#modernSpin::up-button, QDoubleSpinBox#modernSpin::down-button {
            width: 0px;
            height: 0px;
            border: none;
        }

        /* === SCROLL AREA === */
        QScrollArea {
            border: none;
            background: transparent;
        }
        QScrollBar:vertical {
            background: #f8fafc;
            width: 10px;
            border-radius: 5px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #cbd5e1;
            border-radius: 5px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: #94a3b8;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        /* === PANNEAU GAUCHE === */
        QWidget#leftPanel {
            background: transparent;
            border: none;
        }

        /* === SPLITTER === */
        QSplitter::handle {
            background-color: #cbd5e1;
            width: 1px;
        }
        QSplitter::handle:hover {
            background-color: #94a3b8;
        }
    )";
    setStyleSheet(style);

    // Configuration du panneau gauche
    if (leftPanel) {
        leftPanel->setObjectName("leftPanel");
    }
    if (leftScrollArea) {
        leftScrollArea->setFrameShape(QFrame::NoFrame);
    }
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
    // Activer le mode dessin de segment
    schemaView->setInteractionMode(InteractionMode::AddSegment);
    addSegmentToolButton->setChecked(true);

    QMessageBox::information(this, "Mode dessin",
                           "Cliquez 2 fois pour dessiner un tronçon :\n"
                           "1er clic = début du tronçon\n"
                           "2ème clic = fin du tronçon");
}

void HydraulicCalculationsWindow::onSegmentDrawingComplete(const QPointF& start, const QPointF& end)
{
    // Créer des copies modifiables pour le snapping exact
    QPointF snappedStart = start;
    QPointF snappedEnd = end;

    // L'utilisateur a terminé de dessiner le segment
    // Demander les paramètres du segment
    HydraulicCalc::NetworkSegment newSegment;

    // Détecter automatiquement le parent en cherchant si le point de départ est proche d'un segment existant
    QString autoParentId = "";
    const double snapDistance = 30.0;  // Distance de snapping

    for (auto* graphicSeg : schemaView->getSegments()) {
        if (!graphicSeg) continue;

        QPointF segStart = graphicSeg->getStartPoint();
        QPointF segEnd = graphicSeg->getEndPoint();

        // Vérifier si le point de départ du nouveau segment est proche du point de fin d'un segment existant
        QPointF deltaStart = snappedStart - segStart;
        QPointF deltaEnd = snappedStart - segEnd;

        double distToStart = std::sqrt(deltaStart.x() * deltaStart.x() + deltaStart.y() * deltaStart.y());
        double distToEnd = std::sqrt(deltaEnd.x() * deltaEnd.x() + deltaEnd.y() * deltaEnd.y());

        if (distToStart < snapDistance || distToEnd < snapDistance) {
            // Trouvé un segment parent potentiel
            auto* parentData = graphicSeg->getSegmentData();
            if (parentData) {
                autoParentId = QString::fromStdString(parentData->id);

                // SNAP exact au point de connexion du parent pour fusionner les points
                snappedStart = (distToStart < distToEnd) ? segStart : segEnd;

                break;
            }
        }
    }

    // Calculer automatiquement la longueur et la hauteur d'après les points SNAPPÉS
    QPointF delta = snappedEnd - snappedStart;
    double length = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y()) / 2.0;  // échelle : 1m = 2 pixels
    double heightDiff = -delta.y() / 2.0;  // Négatif car Y augmente vers le bas

    newSegment.length = std::max(0.1, length);
    newSegment.heightDifference = heightDiff;

    // Pré-remplir le parent dans le nouveau segment
    if (!autoParentId.isEmpty()) {
        newSegment.parentId = autoParentId.toStdString();
    }

    if (showSegmentDialog(newSegment, false)) {
        // Ajouter le segment aux données
        newSegment.id = "seg_" + std::to_string(networkSegments.size() + 1);
        networkSegments.push_back(newSegment);

        // Créer le segment graphique avec les points SNAPPÉS pour éviter la superposition
        GraphicPipeSegment* graphicSegment = schemaView->addSegment(&networkSegments.back(), snappedStart, snappedEnd);

        // Retourner en mode sélection
        onSelectModeActivated();
    } else {
        // L'utilisateur a annulé, retourner en mode sélection
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

    // Vérifier que le pointeur est valide
    if (!segmentData) {
        QMessageBox::warning(this, "Erreur", "Le segment sélectionné n'est plus valide.");
        currentSelectedSegment = nullptr;
        editButton->setEnabled(false);
        deleteButton->setEnabled(false);
        return;
    }

    // Créer une copie locale pour éviter les problèmes de pointeur invalide
    HydraulicCalc::NetworkSegment segmentCopy = *segmentData;

    if (showSegmentDialog(segmentCopy, true)) {
        // Retrouver le segment dans le vector et le mettre à jour
        bool found = false;
        for (auto& seg : networkSegments) {
            if (seg.id == segmentCopy.id) {
                seg = segmentCopy;
                found = true;
                break;
            }
        }

        if (found) {
            currentSelectedSegment->updateDisplay();
            hasCalculated = false;
            exportButton->setEnabled(false);
        }
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
    if (!schemaView) return;

    auto& graphicSegments = schemaView->getSegments();

    // Pour chaque segment graphique, trouver le segment de données correspondant par ID (pas par index !)
    for (auto* graphicSegment : graphicSegments) {
        // Vérifier que le segment graphique est valide
        if (!graphicSegment) continue;

        auto* graphicSegmentData = graphicSegment->getSegmentData();
        if (!graphicSegmentData) continue;

        std::string graphicSegmentId = graphicSegmentData->id;

        // Trouver le segment de données correspondant par ID
        bool found = false;
        for (auto& segmentData : networkSegments) {
            if (segmentData.id == graphicSegmentId) {
                // Trouvé le segment correspondant, mettre à jour ses fixtures
                segmentData.fixtures.clear();

                try {
                    const auto& fixturePoints = graphicSegment->getFixturePoints();
                    for (auto* fixturePoint : fixturePoints) {
                        // Vérifier que le pointeur est valide
                        if (fixturePoint) {
                            segmentData.fixtures.push_back(fixturePoint->toFixture());
                        }
                    }
                } catch (...) {
                    // En cas d'erreur, continuer avec le segment suivant
                }

                found = true;
                break;
            }
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
        html += "<tr><td>Température eau</td><td>" + QString::number(waterTempSpin->value(), 'f', 1) + " C</td></tr>";
        html += "<tr><td>Température ambiante</td><td>" + QString::number(ambientTempSpin->value(), 'f', 1) + " C</td></tr>";
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
                " (D=" + QString::number(segment.result.actualDiameter, 'f', 1) + " mm)</td></tr>";
        html += "<tr><td>Vitesse</td><td>" + QString::number(segment.result.velocity, 'f', 2) + " m/s</td></tr>";
        html += "<tr><td>Perte de charge</td><td>" + QString::number(segment.result.pressureDrop, 'f', 2) + " mCE</td></tr>";
        html += "</table>";

        // Retour si applicable
        if (segment.result.hasReturn) {
            html += "<h3>Retour de bouclage</h3>";
            html += "<table>";
            html += "<tr><th>Résultat</th><th>Valeur</th></tr>";
            html += "<tr class='result'><td>Diamètre retour</td><td>DN " + QString::number(segment.result.returnNominalDiameter) +
                    " (D=" + QString::number(segment.result.returnActualDiameter, 'f', 1) + " mm)</td></tr>";
            html += "<tr><td>Débit retour</td><td>" + QString::number(segment.result.returnFlowRate, 'f', 2) + " L/min</td></tr>";
            html += "<tr><td>Vitesse retour</td><td>" + QString::number(segment.result.returnVelocity, 'f', 2) + " m/s</td></tr>";
            html += "<tr><td>Pertes thermiques</td><td>" + QString::number(segment.result.heatLoss, 'f', 0) + " W</td></tr>";
            html += "<tr><td>Température retour</td><td>" + QString::number(segment.result.returnTemperature, 'f', 1) + " C</td></tr>";
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
    dialog.setMinimumWidth(450);

    // Style explicite pour le dialogue
    dialog.setStyleSheet(
        "QDialog {"
        "   background-color: white;"
        "   color: #1e293b;"
        "}"
        "QLabel {"
        "   color: #1e293b;"
        "   background-color: transparent;"
        "   font-size: 10pt;"
        "}"
        "QLineEdit, QComboBox, QDoubleSpinBox {"
        "   padding: 8px;"
        "   border: 2px solid #e2e8f0;"
        "   border-radius: 6px;"
        "   background-color: white;"
        "   color: #1e293b;"
        "   font-size: 10pt;"
        "}"
        "QPushButton {"
        "   background-color: #3b82f6;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 6px;"
        "   padding: 10px 20px;"
        "   font-weight: 600;"
        "   font-size: 10pt;"
        "   min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #2563eb;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #1e40af;"
        "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    // Copier les informations pour éviter les références invalides
    QString currentSegmentId = QString::fromStdString(segment.id);
    QString currentSegmentName = QString::fromStdString(segment.name);
    QString currentParentId = QString::fromStdString(segment.parentId);

    QLineEdit *nameEdit = new QLineEdit(&dialog);
    nameEdit->setText(currentSegmentName);
    if (!isEdit) {
        nameEdit->setText("Tronçon " + QString::number(networkSegments.size() + 1));
    }
    formLayout->addRow("Nom:", nameEdit);

    QComboBox *parentCombo = new QComboBox(&dialog);
    parentCombo->addItem("(Aucun - Segment racine)", "");

    // Créer une copie des segments pour éviter les problèmes d'itération
    std::vector<std::pair<QString, QString>> segmentList;
    for (const auto& seg : networkSegments) {
        QString segId = QString::fromStdString(seg.id);
        QString segName = QString::fromStdString(seg.name);
        if (segId != currentSegmentId) {  // Ne pas pouvoir se choisir comme parent
            segmentList.push_back({segName, segId});
        }
    }

    for (const auto& [name, id] : segmentList) {
        parentCombo->addItem(name, id);
    }

    // Présélectionner le parent s'il existe (création ou édition)
    if (!currentParentId.isEmpty()) {
        int idx = parentCombo->findData(currentParentId);
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
