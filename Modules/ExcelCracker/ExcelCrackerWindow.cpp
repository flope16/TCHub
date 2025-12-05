#include "ExcelCrackerWindow.h"
#include "ExcelProtectionRemover.h"
#include "ExcelBruteForce.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QApplication>
#include <QScreen>
#include <QtConcurrent/QtConcurrent>

ExcelCrackerWindow::ExcelCrackerWindow(QWidget *parent)
    : QDialog(parent), isProcessing(false), workerThread(nullptr)
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

ExcelCrackerWindow::~ExcelCrackerWindow()
{
    if (workerThread != nullptr && workerThread->isRunning())
    {
        ExcelBruteForce::stop();
        workerThread->wait();
        delete workerThread;
    }
}

void ExcelCrackerWindow::setupUi()
{
    setWindowTitle("Excel Cracker - Suppression de protection");
    setMinimumSize(650, 650);
    resize(750, 700);

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

    // Mode
    QHBoxLayout *modeLayout = new QHBoxLayout();
    modeLabel = new QLabel("Mode :", this);
    modeLabel->setFixedWidth(120);
    modeLabel->setStyleSheet("QLabel { font-weight: bold; color: #2c3e50; background: transparent; padding: 5px; }");

    modeCombo = new QComboBox(this);
    modeCombo->setMinimumHeight(30);
    modeCombo->setFixedWidth(300);
    modeCombo->addItem("Supprimer protection des feuilles");
    modeCombo->addItem("Brute-force mot de passe");

    connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExcelCrackerWindow::onModeChanged);

    modeLayout->addWidget(modeLabel);
    modeLayout->addWidget(modeCombo);
    modeLayout->addStretch();

    configLayout->addLayout(modeLayout);

    // Fichier Excel
    QHBoxLayout *fileLayout = new QHBoxLayout();
    fileLabel = new QLabel("Fichier Excel :", this);
    fileLabel->setFixedWidth(120);
    fileLabel->setStyleSheet("QLabel { font-weight: bold; color: #2c3e50; background: transparent; padding: 5px; }");

    filePathEdit = new QLineEdit(this);
    filePathEdit->setPlaceholderText("Sélectionnez un fichier Excel (.xlsx)...");
    filePathEdit->setReadOnly(false);
    filePathEdit->setMinimumHeight(30);

    browseButton = new QPushButton("Parcourir...", this);
    browseButton->setFixedSize(120, 30);
    browseButton->setCursor(Qt::PointingHandCursor);
    connect(browseButton, &QPushButton::clicked, this, &ExcelCrackerWindow::onBrowseClicked);

    fileLayout->addWidget(fileLabel);
    fileLayout->addWidget(filePathEdit);
    fileLayout->addWidget(browseButton);

    configLayout->addLayout(fileLayout);

    mainLayout->addWidget(configGroup);

    // Groupe de configuration brute-force
    bruteForceConfigGroup = new QGroupBox("Configuration Brute-Force", this);
    bruteForceConfigGroup->setStyleSheet(configGroup->styleSheet());
    bruteForceConfigGroup->setVisible(false);

    QVBoxLayout *bfConfigLayout = new QVBoxLayout(bruteForceConfigGroup);
    bfConfigLayout->setSpacing(12);

    // Longueur du mot de passe
    QHBoxLayout *lengthLayout = new QHBoxLayout();
    QLabel *lengthLabel = new QLabel("Longueur:", this);
    lengthLabel->setFixedWidth(120);
    lengthLabel->setStyleSheet("QLabel { font-weight: bold; color: #2c3e50; background: transparent; padding: 5px; }");

    QLabel *minLabel = new QLabel("Min:", this);
    minLengthSpin = new QSpinBox(this);
    minLengthSpin->setMinimum(1);
    minLengthSpin->setMaximum(8);
    minLengthSpin->setValue(1);
    minLengthSpin->setFixedWidth(60);

    QLabel *maxLabel = new QLabel("Max:", this);
    maxLengthSpin = new QSpinBox(this);
    maxLengthSpin->setMinimum(1);
    maxLengthSpin->setMaximum(8);
    maxLengthSpin->setValue(4);
    maxLengthSpin->setFixedWidth(60);

    lengthLayout->addWidget(lengthLabel);
    lengthLayout->addWidget(minLabel);
    lengthLayout->addWidget(minLengthSpin);
    lengthLayout->addWidget(maxLabel);
    lengthLayout->addWidget(maxLengthSpin);
    lengthLayout->addStretch();

    bfConfigLayout->addLayout(lengthLayout);

    // Jeu de caractères
    QHBoxLayout *charsetLayout = new QHBoxLayout();
    QLabel *charsetLabel = new QLabel("Caractères:", this);
    charsetLabel->setFixedWidth(120);
    charsetLabel->setStyleSheet("QLabel { font-weight: bold; color: #2c3e50; background: transparent; padding: 5px; }");

    lowercaseCheck = new QCheckBox("a-z", this);
    lowercaseCheck->setChecked(true);

    uppercaseCheck = new QCheckBox("A-Z", this);
    uppercaseCheck->setChecked(false);

    digitsCheck = new QCheckBox("0-9", this);
    digitsCheck->setChecked(false);

    specialCharsCheck = new QCheckBox("!@#$%", this);
    specialCharsCheck->setChecked(false);

    charsetLayout->addWidget(charsetLabel);
    charsetLayout->addWidget(lowercaseCheck);
    charsetLayout->addWidget(uppercaseCheck);
    charsetLayout->addWidget(digitsCheck);
    charsetLayout->addWidget(specialCharsCheck);
    charsetLayout->addStretch();

    bfConfigLayout->addLayout(charsetLayout);

    QLabel *warningLabel = new QLabel("⚠️ ATTENTION: Le brute-force peut prendre beaucoup de temps!", this);
    warningLabel->setStyleSheet("QLabel { color: #e74c3c; font-weight: bold; background: transparent; }");
    bfConfigLayout->addWidget(warningLabel);

    mainLayout->addWidget(bruteForceConfigGroup);

    // Boutons d'action
    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->addStretch();

    processButton = new QPushButton("🚀 Traiter", this);
    processButton->setFixedSize(180, 45);
    processButton->setCursor(Qt::PointingHandCursor);
    processButton->setStyleSheet(
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
    connect(processButton, &QPushButton::clicked, this, &ExcelCrackerWindow::onProcessClicked);

    stopButton = new QPushButton("⏹️ Arrêter", this);
    stopButton->setFixedSize(120, 45);
    stopButton->setCursor(Qt::PointingHandCursor);
    stopButton->setVisible(false);
    stopButton->setStyleSheet(
        "QPushButton { "
        "   background-color: #e74c3c; "
        "   color: white; "
        "   font-size: 14px; "
        "   font-weight: bold; "
        "   border: none; "
        "   border-radius: 6px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #c0392b; "
        "}"
    );
    connect(stopButton, &QPushButton::clicked, this, &ExcelCrackerWindow::onStopClicked);

    actionLayout->addWidget(processButton);
    actionLayout->addWidget(stopButton);
    actionLayout->addStretch();

    mainLayout->addLayout(actionLayout);

    // Barre de progression
    progressBar = new QProgressBar(this);
    progressBar->setMinimumHeight(25);
    progressBar->setVisible(false);
    progressBar->setTextVisible(true);
    progressBar->setStyleSheet(
        "QProgressBar { "
        "   border: 2px solid #3498db; "
        "   border-radius: 5px; "
        "   text-align: center; "
        "   background-color: #ecf0f1; "
        "} "
        "QProgressBar::chunk { "
        "   background-color: #3498db; "
        "}"
    );
    mainLayout->addWidget(progressBar);

    // Label de progression
    progressLabel = new QLabel(this);
    progressLabel->setVisible(false);
    progressLabel->setStyleSheet("QLabel { color: #2c3e50; font-weight: bold; background: transparent; }");
    progressLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(progressLabel);

    // Groupe de résultats
    resultGroup = new QGroupBox("Résultats", this);
    resultGroup->setStyleSheet(configGroup->styleSheet());

    QVBoxLayout *resultLayout = new QVBoxLayout(resultGroup);

    statusText = new QTextEdit(this);
    statusText->setReadOnly(true);
    statusText->setMinimumHeight(150);
    statusText->setStyleSheet(
        "QTextEdit { "
        "   background-color: #2c3e50; "
        "   color: #ecf0f1; "
        "   font-family: 'Consolas', 'Courier New', monospace; "
        "   font-size: 11px; "
        "   border: 1px solid #34495e; "
        "   border-radius: 4px; "
        "   padding: 8px; "
        "}"
    );

    resultLayout->addWidget(statusText);

    mainLayout->addWidget(resultGroup);

    // Bouton Fermer
    QHBoxLayout *closeLayout = new QHBoxLayout();
    closeLayout->addStretch();

    closeButton = new QPushButton("Fermer", this);
    closeButton->setFixedSize(120, 35);
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setStyleSheet(
        "QPushButton { "
        "   background-color: #95a5a6; "
        "   color: white; "
        "   font-size: 12px; "
        "   font-weight: bold; "
        "   border: none; "
        "   border-radius: 6px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #7f8c8d; "
        "}"
    );
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    closeLayout->addWidget(closeButton);

    mainLayout->addLayout(closeLayout);

    // Message initial
    updateStatus("Prêt. Sélectionnez un fichier Excel et choisissez le mode de traitement.");
}

void ExcelCrackerWindow::applyModernStyle()
{
    setStyleSheet(
        "QDialog { "
        "   background-color: #ecf0f1; "
        "   font-family: 'Segoe UI', Arial, sans-serif; "
        "} "
        "QLineEdit { "
        "   border: 2px solid #bdc3c7; "
        "   border-radius: 4px; "
        "   padding: 5px; "
        "   background-color: white; "
        "   color: #2c3e50; "
        "} "
        "QLineEdit:focus { "
        "   border-color: #3498db; "
        "} "
        "QPushButton { "
        "   background-color: #3498db; "
        "   color: white; "
        "   font-size: 11px; "
        "   font-weight: bold; "
        "   border: none; "
        "   border-radius: 4px; "
        "   padding: 5px; "
        "} "
        "QPushButton:hover { "
        "   background-color: #2980b9; "
        "} "
        "QComboBox { "
        "   border: 2px solid #bdc3c7; "
        "   border-radius: 4px; "
        "   padding: 5px; "
        "   background-color: white; "
        "   color: #2c3e50; "
        "} "
        "QComboBox:focus { "
        "   border-color: #3498db; "
        "} "
        "QComboBox::drop-down { "
        "   border: none; "
        "} "
        "QComboBox::down-arrow { "
        "   image: none; "
        "   border-left: 5px solid transparent; "
        "   border-right: 5px solid transparent; "
        "   border-top: 5px solid #2c3e50; "
        "   margin-right: 5px; "
        "}"
    );
}

void ExcelCrackerWindow::onModeChanged(int index)
{
    bruteForceConfigGroup->setVisible(index == 1);
    adjustSize();
}

void ExcelCrackerWindow::onBrowseClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Sélectionner un fichier Excel",
        QString(),
        "Fichiers Excel (*.xlsx);;Tous les fichiers (*.*)"
    );

    if (!fileName.isEmpty())
    {
        filePathEdit->setText(fileName);
        updateStatus("Fichier sélectionné: " + fileName);
    }
}

void ExcelCrackerWindow::onProcessClicked()
{
    QString filePath = filePathEdit->text().trimmed();

    if (filePath.isEmpty())
    {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un fichier Excel.");
        return;
    }

    if (!QFile::exists(filePath))
    {
        QMessageBox::warning(this, "Erreur", "Le fichier sélectionné n'existe pas.");
        return;
    }

    processFile();
}

void ExcelCrackerWindow::onStopClicked()
{
    ExcelBruteForce::stop();
    updateStatus("Arrêt demandé...", false);
    stopButton->setEnabled(false);
}

void ExcelCrackerWindow::processFile()
{
    int mode = modeCombo->currentIndex();

    if (mode == 0)
    {
        removeProtection();
    }
    else
    {
        bruteForcePassword();
    }
}

void ExcelCrackerWindow::removeProtection()
{
    QString filePath = filePathEdit->text().trimmed();
    updateStatus("Suppression de la protection en cours...");

    progressBar->setVisible(true);
    progressBar->setRange(0, 0); // Mode indéterminé
    processButton->setEnabled(false);

    // Exécuter dans un thread séparé
    QFuture<bool> future = QtConcurrent::run([filePath]() {
        return ExcelProtectionRemover::removeProtection(filePath.toStdString());
    });

    // Attendre que le traitement soit terminé
    while (!future.isFinished())
    {
        QApplication::processEvents();
    }

    bool success = future.result();

    progressBar->setVisible(false);
    processButton->setEnabled(true);

    if (success)
    {
        QString outputPath = QString::fromStdString(
            ExcelProtectionRemover::generateOutputPath(filePath.toStdString())
        );
        updateStatus("✅ Succès! Fichier créé: " + outputPath);
        QMessageBox::information(this, "Succès",
            "La protection a été supprimée avec succès!\n\n"
            "Fichier de sortie: " + outputPath);
    }
    else
    {
        QString error = QString::fromStdString(ExcelProtectionRemover::getLastError());
        updateStatus("❌ Erreur: " + error, true);
        QMessageBox::warning(this, "Erreur", error);
    }
}

void ExcelCrackerWindow::bruteForcePassword()
{
    QString filePath = filePathEdit->text().trimmed();

    // Construire le jeu de caractères
    std::string charset = "";
    if (lowercaseCheck->isChecked()) charset += "abcdefghijklmnopqrstuvwxyz";
    if (uppercaseCheck->isChecked()) charset += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (digitsCheck->isChecked()) charset += "0123456789";
    if (specialCharsCheck->isChecked()) charset += "!@#$%^&*()-_=+[]{}|;:,.<>?";

    if (charset.empty())
    {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner au moins un jeu de caractères.");
        return;
    }

    int minLength = minLengthSpin->value();
    int maxLength = maxLengthSpin->value();

    if (minLength > maxLength)
    {
        QMessageBox::warning(this, "Erreur", "La longueur minimale doit être inférieure ou égale à la longueur maximale.");
        return;
    }

    updateStatus("Brute-force en cours...\nCela peut prendre beaucoup de temps selon la complexité du mot de passe.");

    progressBar->setVisible(true);
    progressBar->setRange(0, 0); // Mode indéterminé
    progressLabel->setVisible(true);
    processButton->setVisible(false);
    stopButton->setVisible(true);
    stopButton->setEnabled(true);

    // Configuration
    ExcelBruteForce::Config config;
    config.minLength = minLength;
    config.maxLength = maxLength;
    config.charset = charset;
    config.progressInterval = 100;

    // Exécuter dans un thread séparé
    QFuture<std::string> future = QtConcurrent::run([filePath, config, this]() {
        return ExcelBruteForce::bruteForce(
            filePath.toStdString(),
            config,
            [this](int attempts, const std::string& password) {
                // Mettre à jour l'interface depuis le thread
                QString msg = QString("Tentatives: %1, Mot de passe testé: %2")
                    .arg(attempts)
                    .arg(QString::fromStdString(password));

                QMetaObject::invokeMethod(this, "updateProgress",
                    Qt::QueuedConnection,
                    Q_ARG(int, attempts),
                    Q_ARG(QString, QString::fromStdString(password)));
            }
        );
    });

    // Attendre que le traitement soit terminé
    while (!future.isFinished())
    {
        QApplication::processEvents();
    }

    std::string password = future.result();

    progressBar->setVisible(false);
    progressLabel->setVisible(false);
    processButton->setVisible(true);
    stopButton->setVisible(false);

    if (!password.empty())
    {
        QString msg = "🎉 Mot de passe trouvé: " + QString::fromStdString(password);
        updateStatus(msg);
        QMessageBox::information(this, "Succès", msg);
    }
    else
    {
        QString error = QString::fromStdString(ExcelBruteForce::getLastError());
        updateStatus("❌ " + error, true);
        QMessageBox::warning(this, "Échec", error);
    }
}

void ExcelCrackerWindow::updateProgress(int attempts, const QString& currentPassword)
{
    progressLabel->setText(QString("Tentatives: %1 | Mot de passe: %2")
        .arg(attempts)
        .arg(currentPassword));
}

void ExcelCrackerWindow::updateStatus(const QString &message, bool isError)
{
    QString timestamp = QTime::currentTime().toString("hh:mm:ss");
    QString color = isError ? "#ff6b6b" : "#51cf66";  // Rouge plus clair / Vert plus clair pour meilleur contraste

    QString html = QString("<span style='color: #e1e8ed;'>[%1]</span> "  // Gris très clair pour timestamp
                          "<span style='color: %2; font-weight: bold;'>%3</span>")
                      .arg(timestamp)
                      .arg(color)
                      .arg(message);

    statusText->append(html);

    // Auto-scroll
    QTextCursor cursor = statusText->textCursor();
    cursor.movePosition(QTextCursor::End);
    statusText->setTextCursor(cursor);
}
