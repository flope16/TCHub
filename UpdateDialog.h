#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include "UpdateChecker.h"

/**
 * @brief Boîte de dialogue pour proposer une mise à jour à l'utilisateur
 */
class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog(const QString& currentVersion,
                         const QString& newVersion,
                         QWidget *parent = nullptr);

    /**
     * @brief Définit le UpdateChecker à utiliser
     */
    void setUpdateChecker(UpdateChecker* checker);

private slots:
    void onUpdateClicked();
    void onLaterClicked();
    void onUpdateProgress(int progress, const QString& message);
    void onUpdateCompleted(bool success);

private:
    void setupUi();

    QString currentVersion;
    QString newVersion;
    UpdateChecker* updateChecker;

    // Widgets
    QLabel* titleLabel;
    QLabel* messageLabel;
    QLabel* versionLabel;
    QPushButton* updateButton;
    QPushButton* laterButton;
    QProgressBar* progressBar;
    QLabel* progressLabel;
};
