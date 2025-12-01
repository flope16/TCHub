#include "PDFParserWindow.h"
#include "ParserFactory.h"
#include "XlsxWriter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QApplication>
#include <QScreen>
#include <QTime>

PDFParserWindow::PDFParserWindow(QWidget *parent)
    : QDialog(parent)
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

PDFParserWindow::~PDFParserWindow()
{
}

void PDFParserWindow::setupUi()
{
    setWindowTitle("PDF Parser - Extraction de données");
    setMinimumSize(600, 500);
    resize(700, 550);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // Groupe de configuration
    configGroup = new QGroupBox("Configuration", this);
    configGroup->setStyleSheet(
        "QGroupBox { "
        "   font-size: 13px; "
        "   font-weight: bold; "
        "   border: 2px solid #3498db; "
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

    QVBoxLayout *configLayout = new QVBoxLayout(configGroup);
    configLayout->setSpacing(12);

    // Fournisseur
    QHBoxLayout *supplierLayout = new QHBoxLayout();
    supplierLabel = new QLabel("Fournisseur :", this);
    supplierLabel->setFixedWidth(100);
    supplierLabel->setStyleSheet("QLabel { font-weight: bold; color: #2c3e50; }");

    supplierCombo = new QComboBox(this);
    supplierCombo->setMinimumHeight(30);

    // Remplir avec les fournisseurs disponibles
    auto suppliers = ParserFactory::getSupportedSuppliers();
    for (const auto& supplier : suppliers)
    {
        supplierCombo->addItem(QString::fromStdString(supplier));
    }

    connect(supplierCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PDFParserWindow::onSupplierChanged);

    supplierLayout->addWidget(supplierLabel);
    supplierLayout->addWidget(supplierCombo);
    supplierLayout->addStretch();

    configLayout->addLayout(supplierLayout);

    // Fichier PDF
    QHBoxLayout *fileLayout = new QHBoxLayout();
    fileLabel = new QLabel("Fichier PDF :", this);
    fileLabel->setFixedWidth(100);
    fileLabel->setStyleSheet("QLabel { font-weight: bold; color: #2c3e50; }");

    filePathEdit = new QLineEdit(this);
    filePathEdit->setPlaceholderText("Sélectionnez un fichier PDF...");
    filePathEdit->setReadOnly(true);
    filePathEdit->setMinimumHeight(30);

    browseButton = new QPushButton("Parcourir...", this);
    browseButton->setFixedSize(120, 30);
    browseButton->setCursor(Qt::PointingHandCursor);
    connect(browseButton, &QPushButton::clicked, this, &PDFParserWindow::onBrowseClicked);

    fileLayout->addWidget(fileLabel);
    fileLayout->addWidget(filePathEdit);
    fileLayout->addWidget(browseButton);

    configLayout->addLayout(fileLayout);

    mainLayout->addWidget(configGroup);

    // Bouton Parser
    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->addStretch();

    parseButton = new QPushButton("🚀 Parser le PDF", this);
    parseButton->setFixedSize(180, 45);
    parseButton->setCursor(Qt::PointingHandCursor);
    parseButton->setStyleSheet(
        "QPushButton { "
        "   background-color: #27ae60; "
        "   color: white; "
        "   font-size: 14px; "
        "   font-weight: bold; "
        "   border: none; "
        "   border-radius: 6px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #229954; "
        "} "
        "QPushButton:pressed { "
        "   background-color: #1e8449; "
        "} "
        "QPushButton:disabled { "
        "   background-color: #bdc3c7; "
        "}"
    );
    connect(parseButton, &QPushButton::clicked, this, &PDFParserWindow::onParseClicked);

    actionLayout->addWidget(parseButton);
    actionLayout->addStretch();

    mainLayout->addLayout(actionLayout);

    // Barre de progression
    progressBar = new QProgressBar(this);
    progressBar->setMinimumHeight(25);
    progressBar->setVisible(false);
    progressBar->setTextVisible(true);
    mainLayout->addWidget(progressBar);

    // Groupe de résultat
    resultGroup = new QGroupBox("Résultat", this);
    resultGroup->setStyleSheet(
        "QGroupBox { "
        "   font-size: 13px; "
        "   font-weight: bold; "
        "   border: 2px solid #95a5a6; "
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

    QVBoxLayout *resultLayout = new QVBoxLayout(resultGroup);

    statusText = new QTextEdit(this);
    statusText->setReadOnly(true);
    statusText->setMinimumHeight(150);
    statusText->setStyleSheet(
        "QTextEdit { "
        "   background-color: #f8f9fa; "
        "   border: 1px solid #dee2e6; "
        "   border-radius: 4px; "
        "   padding: 10px; "
        "   font-family: 'Consolas', 'Courier New', monospace; "
        "   font-size: 11px; "
        "}"
    );
    updateStatus("Prêt à parser un fichier PDF");

    resultLayout->addWidget(statusText);
    mainLayout->addWidget(resultGroup, 1);

    // Bouton Fermer
    QHBoxLayout *footerLayout = new QHBoxLayout();
    footerLayout->addStretch();

    closeButton = new QPushButton("Fermer", this);
    closeButton->setFixedSize(100, 35);
    closeButton->setCursor(Qt::PointingHandCursor);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    footerLayout->addWidget(closeButton);
    mainLayout->addLayout(footerLayout);
}

void PDFParserWindow::applyModernStyle()
{
    QString styleSheet = R"(
        QDialog {
            background-color: #ffffff;
        }

        QComboBox {
            background-color: white;
            border: 1px solid #bdc3c7;
            border-radius: 4px;
            padding: 5px;
            min-height: 25px;
        }

        QComboBox:hover {
            border: 1px solid #3498db;
        }

        QComboBox::drop-down {
            border: none;
            width: 30px;
        }

        QComboBox::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 5px solid #2c3e50;
            margin-right: 10px;
        }

        QLineEdit {
            background-color: white;
            border: 1px solid #bdc3c7;
            border-radius: 4px;
            padding: 5px 10px;
            font-size: 11px;
        }

        QLineEdit:focus {
            border: 1px solid #3498db;
        }

        QPushButton {
            background-color: #3498db;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 12px;
            font-weight: bold;
        }

        QPushButton:hover {
            background-color: #2980b9;
        }

        QPushButton:pressed {
            background-color: #21618c;
        }

        QPushButton:disabled {
            background-color: #bdc3c7;
            color: #7f8c8d;
        }

        QProgressBar {
            border: 1px solid #bdc3c7;
            border-radius: 4px;
            text-align: center;
            background-color: #ecf0f1;
        }

        QProgressBar::chunk {
            background-color: #3498db;
            border-radius: 3px;
        }
    )";

    setStyleSheet(styleSheet);
}

void PDFParserWindow::updateStatus(const QString &message, bool isError)
{
    QString color = isError ? "#e74c3c" : "#2c3e50";
    QString timestamp = QTime::currentTime().toString("hh:mm:ss");

    QString html = QString("<span style='color: #95a5a6;'>[%1]</span> <span style='color: %2;'>%3</span>")
                   .arg(timestamp)
                   .arg(color)
                   .arg(message);

    statusText->append(html);
}

void PDFParserWindow::onBrowseClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Sélectionner un fichier PDF",
        "",
        "Fichiers PDF (*.pdf);;Tous les fichiers (*.*)"
    );

    if (!fileName.isEmpty())
    {
        filePathEdit->setText(fileName);
        updateStatus("Fichier sélectionné : " + QFileInfo(fileName).fileName());
    }
}

void PDFParserWindow::onSupplierChanged(int index)
{
    if (index >= 0)
    {
        updateStatus("Fournisseur sélectionné : " + supplierCombo->currentText());
    }
}

void PDFParserWindow::onParseClicked()
{
    parsePdfFile();
}

void PDFParserWindow::parsePdfFile()
{
    // Vérifier qu'un fichier est sélectionné
    QString filePath = filePathEdit->text();
    if (filePath.isEmpty())
    {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un fichier PDF");
        updateStatus("❌ Erreur : Aucun fichier sélectionné", true);
        return;
    }

    // Vérifier que le fichier existe
    if (!QFile::exists(filePath))
    {
        QMessageBox::warning(this, "Erreur", "Le fichier spécifié n'existe pas");
        updateStatus("❌ Erreur : Fichier introuvable", true);
        return;
    }

    // Désactiver les boutons pendant le parsing
    parseButton->setEnabled(false);
    browseButton->setEnabled(false);
    progressBar->setVisible(true);
    progressBar->setValue(0);

    updateStatus("🔄 Début du parsing...");
    progressBar->setValue(20);

    // Récupérer le fournisseur
    std::string supplierName = supplierCombo->currentText().toStdString();
    Supplier supplier = ParserFactory::supplierFromString(supplierName);

    updateStatus("📦 Création du parseur " + supplierCombo->currentText());
    progressBar->setValue(40);

    // Créer le parseur
    auto parser = ParserFactory::createParser(supplier);
    if (!parser)
    {
        QMessageBox::critical(this, "Erreur", "Impossible de créer le parseur");
        updateStatus("❌ Erreur : Parseur non disponible", true);
        parseButton->setEnabled(true);
        browseButton->setEnabled(true);
        progressBar->setVisible(false);
        return;
    }

    updateStatus("📄 Parsing du fichier PDF en cours...");
    progressBar->setValue(60);

    // Parser le fichier
    std::vector<PdfLine> lines;
    try
    {
        lines = parser->parse(filePath.toStdString());
    }
    catch (const std::exception& e)
    {
        QString errorMsg = QString("Une erreur est survenue : %1").arg(e.what());
        QMessageBox::critical(this, "Erreur", errorMsg);
        updateStatus("❌ " + errorMsg, true);
        parseButton->setEnabled(true);
        browseButton->setEnabled(true);
        progressBar->setVisible(false);
        return;
    }

    progressBar->setValue(80);

    if (lines.empty())
    {
        QMessageBox::warning(this, "Avertissement", "Aucune ligne de produit n'a été trouvée dans le PDF");
        updateStatus("⚠️ Aucune donnée extraite du PDF", true);
        parseButton->setEnabled(true);
        browseButton->setEnabled(true);
        progressBar->setVisible(false);
        return;
    }

    updateStatus(QString("✅ %1 lignes extraites").arg(lines.size()));

    // Générer le nom du fichier de sortie
    QFileInfo fileInfo(filePath);
    QString outputPath = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".xml";

    updateStatus("💾 Génération du fichier Excel...");

    // Écrire le fichier XLSX
    if (!XlsxWriter::writeToXlsx(outputPath.toStdString(), lines))
    {
        QMessageBox::critical(this, "Erreur", "Impossible d'écrire le fichier de sortie");
        updateStatus("❌ Erreur : Échec de l'écriture du fichier", true);
        parseButton->setEnabled(true);
        browseButton->setEnabled(true);
        progressBar->setVisible(false);
        return;
    }

    progressBar->setValue(100);

    // Succès
    QString successMsg = QString("✅ Succès !\n\n%1 lignes extraites\n\nFichier généré :\n%2")
                         .arg(lines.size())
                         .arg(outputPath);

    updateStatus("🎉 Parsing terminé avec succès !");
    updateStatus("📁 Fichier : " + outputPath);

    QMessageBox::information(this, "Succès", successMsg);

    // Réactiver les boutons
    parseButton->setEnabled(true);
    browseButton->setEnabled(true);
    progressBar->setVisible(false);
}
