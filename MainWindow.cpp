#include "MainWindow.h"
#include "Modules/PDFParser/PDFParserWindow.h"
#include <QApplication>
#include <QScreen>
#include <QIcon>

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
    setMinimumSize(500, 400);
    resize(600, 450);

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
    // Le logo sera chargé depuis les ressources
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

    // Groupe de modules
    modulesGroup = new QGroupBox("Modules disponibles", this);
    modulesGroup->setStyleSheet(
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

    QVBoxLayout *modulesLayout = new QVBoxLayout(modulesGroup);
    modulesLayout->setSpacing(10);

    // Module PDF Parser
    QFrame *pdfParserFrame = new QFrame(this);
    pdfParserFrame->setStyleSheet(
        "QFrame { "
        "   background-color: #f8f9fa; "
        "   border: 1px solid #dee2e6; "
        "   border-radius: 6px; "
        "   padding: 10px; "
        "}"
    );

    QHBoxLayout *pdfParserLayout = new QHBoxLayout(pdfParserFrame);

    QLabel *pdfIcon = new QLabel(this);
    pdfIcon->setText("📄");
    pdfIcon->setStyleSheet("QLabel { font-size: 32px; background: transparent; border: none; }");
    pdfParserLayout->addWidget(pdfIcon);

    QVBoxLayout *pdfInfoLayout = new QVBoxLayout();
    QLabel *pdfTitle = new QLabel("PDF Parser", this);
    pdfTitle->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; color: #2c3e50; background: transparent; border: none; }");

    QLabel *pdfDesc = new QLabel("Convertit les fichiers PDF fournisseurs en fichiers Excel structurés", this);
    pdfDesc->setStyleSheet("QLabel { font-size: 11px; color: #6c757d; background: transparent; border: none; }");
    pdfDesc->setWordWrap(true);

    pdfInfoLayout->addWidget(pdfTitle);
    pdfInfoLayout->addWidget(pdfDesc);
    pdfParserLayout->addLayout(pdfInfoLayout, 1);

    pdfParserButton = new QPushButton("Ouvrir", this);
    pdfParserButton->setFixedSize(100, 35);
    pdfParserButton->setCursor(Qt::PointingHandCursor);
    connect(pdfParserButton, &QPushButton::clicked, this, &MainWindow::onPdfParserClicked);
    pdfParserLayout->addWidget(pdfParserButton);

    modulesLayout->addWidget(pdfParserFrame);

    // Placeholder pour futurs modules
    QLabel *futureModules = new QLabel("D'autres modules seront ajoutés ici...", this);
    futureModules->setAlignment(Qt::AlignCenter);
    futureModules->setStyleSheet("QLabel { color: #95a5a6; font-style: italic; margin: 20px; }");
    modulesLayout->addWidget(futureModules);

    modulesLayout->addStretch();

    mainLayout->addWidget(modulesGroup, 1);

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
