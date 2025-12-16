#include "MainWindow.h"
#include "Modules/PDFParser/PDFParserWindow.h"
#include "Modules/ExcelCracker/ExcelCrackerWindow.h"
#include "Modules/HydraulicCalculations/HydraulicCalculationsWindow.h"
#include <QApplication>
#include <QScreen>
#include <QIcon>
#include <QScrollArea>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    applyModernStyle();

    // Centrer la fenêtre
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    setWindowTitle("TC Hub - Centre de modules");
    setMinimumSize(600, 600);
    resize(700, 700);

    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // En-tête avec logo et titre
    QHBoxLayout *headerLayout = new QHBoxLayout();

    logoLabel = new QLabel(this);
    logoLabel->setFixedSize(64, 64);
    logoLabel->setScaledContents(true);
    logoLabel->setPixmap(QPixmap(":/Resources/TCHub_logo.png"));
    if (logoLabel->pixmap().isNull()) {
        logoLabel->setText("TC");
        logoLabel->setStyleSheet("QLabel { background-color: #4472C4; color: white; font-size: 24px; font-weight: bold; border-radius: 32px; }");
        logoLabel->setAlignment(Qt::AlignCenter);
    }
    headerLayout->addWidget(logoLabel);

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLabel = new QLabel("Bienvenue dans TC Hub", this);
    titleLabel->setStyleSheet("QLabel { font-size: 20px; font-weight: bold; color: #2c3e50; }");

    QLabel *subtitleLabel = new QLabel("Centre de modules d'automatisation", this);
    subtitleLabel->setStyleSheet("QLabel { font-size: 12px; color: #7f8c8d; }");

    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    titleLayout->addStretch();

    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    mainLayout->addLayout(headerLayout);

    // Ligne de séparation
    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("QFrame { color: #bdc3c7; }");
    mainLayout->addWidget(line);

    // Zone scrollable pour les catégories
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *scrollWidget = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setSpacing(20);
    scrollLayout->setContentsMargins(0, 0, 0, 0);

    // Catégorie: Calculs de dimensionnement
    calculationsGroup = new QGroupBox("⚙️ Calculs de dimensionnement", this);
    calculationsGroup->setStyleSheet(
        "QGroupBox { "
        "   font-size: 14px; "
        "   font-weight: bold; "
        "   border: 2px solid #28a745; "
        "   border-radius: 8px; "
        "   margin-top: 10px; "
        "   padding-top: 15px; "
        "   background-color: #f8fff9; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   subcontrol-position: top left; "
        "   padding: 5px 10px; "
        "   color: #28a745; "
        "}"
    );

    QVBoxLayout *calculationsLayout = new QVBoxLayout(calculationsGroup);
    calculationsLayout->setSpacing(10);

    // Module Calculs Hydrauliques
    QFrame *hydraulicFrame = createModuleCard(
        "💧",
        "Calculs Hydrauliques",
        "Dimensionnement des réseaux d'eau froide et eau chaude sanitaire avec bouclage",
        &hydraulicCalculationsButton
    );
    connect(hydraulicCalculationsButton, &QPushButton::clicked, this, &MainWindow::onHydraulicCalculationsClicked);
    calculationsLayout->addWidget(hydraulicFrame);

    scrollLayout->addWidget(calculationsGroup);

    // Catégorie: Divers
    diversGroup = new QGroupBox("🔧 Divers", this);
    diversGroup->setStyleSheet(
        "QGroupBox { "
        "   font-size: 14px; "
        "   font-weight: bold; "
        "   border: 2px solid #bdc3c7; "
        "   border-radius: 8px; "
        "   margin-top: 10px; "
        "   padding-top: 15px; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   subcontrol-position: top left; "
        "   padding: 5px 10px; "
        "   color: #2c3e50; "
        "}"
    );

    QVBoxLayout *diversLayout = new QVBoxLayout(diversGroup);
    diversLayout->setSpacing(10);

    // Module PDF Parser
    QFrame *pdfParserFrame = createModuleCard(
        "📄",
        "PDF Parser",
        "Convertit les fichiers PDF fournisseurs en fichiers Excel structurés",
        &pdfParserButton
    );
    connect(pdfParserButton, &QPushButton::clicked, this, &MainWindow::onPdfParserClicked);
    diversLayout->addWidget(pdfParserFrame);

    // Module Excel Cracker
    QFrame *excelCrackerFrame = createModuleCard(
        "🔓",
        "Excel Cracker",
        "Supprime la protection des feuilles Excel ou force le mot de passe par brute-force",
        &excelCrackerButton
    );
    connect(excelCrackerButton, &QPushButton::clicked, this, &MainWindow::onExcelCrackerClicked);
    diversLayout->addWidget(excelCrackerFrame);

    scrollLayout->addWidget(diversGroup);
    scrollLayout->addStretch();

    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea, 1);

    // Bouton Quitter
    QHBoxLayout *footerLayout = new QHBoxLayout();
    footerLayout->addStretch();

    quitButton = new QPushButton("Quitter", this);
    quitButton->setFixedSize(100, 35);
    quitButton->setCursor(Qt::PointingHandCursor);
    connect(quitButton, &QPushButton::clicked, this, &QMainWindow::close);

    footerLayout->addWidget(quitButton);
    mainLayout->addLayout(footerLayout);
}

QFrame* MainWindow::createModuleCard(const QString& icon, const QString& title, const QString& description, QPushButton** button)
{
    QFrame *frame = new QFrame(this);
    frame->setStyleSheet(
        "QFrame { "
        "   background-color: #f8f9fa; "
        "   border: 1px solid #dee2e6; "
        "   border-radius: 6px; "
        "   padding: 10px; "
        "}"
    );

    QHBoxLayout *layout = new QHBoxLayout(frame);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setText(icon);
    iconLabel->setStyleSheet("QLabel { font-size: 32px; background: transparent; border: none; }");
    layout->addWidget(iconLabel);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    QLabel *titleLabel = new QLabel(title, this);
    titleLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; color: #2c3e50; background: transparent; border: none; }");

    QLabel *descLabel = new QLabel(description, this);
    descLabel->setStyleSheet("QLabel { font-size: 11px; color: #6c757d; background: transparent; border: none; }");
    descLabel->setWordWrap(true);

    infoLayout->addWidget(titleLabel);
    infoLayout->addWidget(descLabel);
    layout->addLayout(infoLayout, 1);

    *button = new QPushButton("Ouvrir", this);
    (*button)->setFixedSize(100, 35);
    (*button)->setCursor(Qt::PointingHandCursor);
    layout->addWidget(*button);

    return frame;
}

void MainWindow::applyModernStyle()
{
    // Style moderne pour toute l'application
    QString styleSheet = R"(
        QMainWindow {
            background-color: #ffffff;
        }

        QPushButton {
            background-color: #4472C4;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 12px;
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
    )";

    setStyleSheet(styleSheet);
}

void MainWindow::onPdfParserClicked()
{
    PDFParserWindow *pdfWindow = new PDFParserWindow(this);
    pdfWindow->exec();
    delete pdfWindow;
}

void MainWindow::onExcelCrackerClicked()
{
    ExcelCrackerWindow *excelWindow = new ExcelCrackerWindow(this);
    excelWindow->exec();
    delete excelWindow;
}

void MainWindow::onHydraulicCalculationsClicked()
{
    HydraulicCalculationsWindow *hydraulicWindow = new HydraulicCalculationsWindow(this);
    hydraulicWindow->exec();
    delete hydraulicWindow;
}
