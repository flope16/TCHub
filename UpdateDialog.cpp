#include "UpdateDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QCoreApplication>

UpdateDialog::UpdateDialog(const QString& currentVer,
                          const QString& newVer,
                          QWidget *parent)
    : QDialog(parent)
    , currentVersion(currentVer)
    , newVersion(newVer)
    , updateChecker(nullptr)
{
    setupUi();
    setWindowTitle("Mise à jour disponible");
    setModal(true);
    resize(450, 250);
}

void UpdateDialog::setUpdateChecker(UpdateChecker* checker)
{
    updateChecker = checker;

    if (updateChecker)
    {
        connect(updateChecker, &UpdateChecker::updateProgress,
                this, &UpdateDialog::onUpdateProgress);
        connect(updateChecker, &UpdateChecker::updateCompleted,
                this, &UpdateDialog::onUpdateCompleted);
    }
}

void UpdateDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Titre
    titleLabel = new QLabel("🔄 <b>Nouvelle version disponible !</b>", this);
    titleLabel->setStyleSheet(
        "QLabel {"
        "   font-size: 16px;"
        "   color: #2c3e50;"
        "   padding: 10px;"
        "}"
    );
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Message
    messageLabel = new QLabel(
        "Une nouvelle version de TC Hub est disponible sur le réseau.\n"
        "Voulez-vous mettre à jour maintenant ?",
        this
    );
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setStyleSheet(
        "QLabel {"
        "   font-size: 12px;"
        "   color: #34495e;"
        "   padding: 10px;"
        "}"
    );
    mainLayout->addWidget(messageLabel);

    // Informations de version
    versionLabel = new QLabel(this);
    versionLabel->setText(
        QString("Version actuelle : <b>%1</b><br/>"
               "Nouvelle version : <b style='color: #27ae60;'>%2</b>")
            .arg(currentVersion)
            .arg(newVersion)
    );
    versionLabel->setAlignment(Qt::AlignCenter);
    versionLabel->setStyleSheet(
        "QLabel {"
        "   font-size: 11px;"
        "   color: #7f8c8d;"
        "   padding: 5px;"
        "   background-color: #ecf0f1;"
        "   border-radius: 5px;"
        "}"
    );
    mainLayout->addWidget(versionLabel);

    // Barre de progression (cachée au début)
    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setStyleSheet(
        "QProgressBar {"
        "   border: 2px solid #bdc3c7;"
        "   border-radius: 5px;"
        "   text-align: center;"
        "   height: 25px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #3498db;"
        "   border-radius: 3px;"
        "}"
    );
    mainLayout->addWidget(progressBar);

    // Label de progression (caché au début)
    progressLabel = new QLabel(this);
    progressLabel->setVisible(false);
    progressLabel->setAlignment(Qt::AlignCenter);
    progressLabel->setStyleSheet("QLabel { color: #7f8c8d; font-size: 10px; }");
    mainLayout->addWidget(progressLabel);

    mainLayout->addStretch();

    // Boutons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    laterButton = new QPushButton("Plus tard", this);
    laterButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #95a5a6;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 12px;"
        "   font-weight: bold;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #7f8c8d;"
        "}"
    );
    connect(laterButton, &QPushButton::clicked, this, &UpdateDialog::onLaterClicked);
    buttonLayout->addWidget(laterButton);

    updateButton = new QPushButton("Mettre à jour maintenant", this);
    updateButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #27ae60;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 12px;"
        "   font-weight: bold;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #229954;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #bdc3c7;"
        "}"
    );
    connect(updateButton, &QPushButton::clicked, this, &UpdateDialog::onUpdateClicked);
    buttonLayout->addWidget(updateButton);

    mainLayout->addLayout(buttonLayout);
}

void UpdateDialog::onUpdateClicked()
{
    if (!updateChecker)
    {
        QMessageBox::warning(this, "Erreur",
                           "Le gestionnaire de mise à jour n'est pas disponible.");
        return;
    }

    // Désactiver les boutons
    updateButton->setEnabled(false);
    laterButton->setEnabled(false);

    // Afficher la barre de progression
    progressBar->setVisible(true);
    progressLabel->setVisible(true);
    progressBar->setValue(0);

    // Lancer la mise à jour
    updateChecker->performUpdate();
}

void UpdateDialog::onLaterClicked()
{
    reject();
}

void UpdateDialog::onUpdateProgress(int progress, const QString& message)
{
    progressBar->setValue(progress);
    progressLabel->setText(message);
}

void UpdateDialog::onUpdateCompleted(bool success)
{
    if (success)
    {
        QMessageBox::information(this, "Mise à jour réussie",
            "La mise à jour a été installée avec succès.\n\n"
            "L'application va se fermer. Veuillez la redémarrer pour utiliser la nouvelle version.");

        // Accepter le dialogue et fermer l'application
        accept();

        // Fermer l'application pour que la mise à jour de l'exe puisse se faire
        QCoreApplication::quit();
    }
    else
    {
        QMessageBox::warning(this, "Échec de la mise à jour",
            "La mise à jour n'a pas pu être effectuée.\n"
            "Veuillez vérifier que le réseau est accessible et réessayer.");

        // Réactiver les boutons
        updateButton->setEnabled(true);
        laterButton->setEnabled(true);
        progressBar->setVisible(false);
        progressLabel->setVisible(false);
    }
}
