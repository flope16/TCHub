#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QProgressBar>
#include <QGroupBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QThread>

/**
 * @brief Fenêtre Qt pour l'outil Excel Cracker
 *
 * Cette fenêtre permet de:
 * 1. Supprimer la protection des feuilles Excel (fichiers non cryptés)
 * 2. Forcer le mot de passe d'un fichier Excel crypté (brute-force)
 */
class ExcelCrackerWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ExcelCrackerWindow(QWidget *parent = nullptr);
    ~ExcelCrackerWindow();

private slots:
    void onBrowseClicked();
    void onProcessClicked();
    void onStopClicked();
    void onModeChanged(int index);
    void updateProgress(int attempts, const QString& currentPassword);
    void onBruteForceConfigChanged();

private:
    void setupUi();
    void applyModernStyle();
    void updateStatus(const QString &message, bool isError = false);
    void processFile();
    void removeProtection();
    void bruteForcePassword();
    void updateEstimation();
    long long calculateTotalCombinations();
    QString formatTime(long long seconds);

    // Widgets
    QComboBox *modeCombo;
    QLineEdit *filePathEdit;
    QPushButton *browseButton;
    QPushButton *processButton;
    QPushButton *stopButton;
    QPushButton *closeButton;
    QTextEdit *statusText;
    QProgressBar *progressBar;
    QLabel *modeLabel;
    QLabel *fileLabel;
    QGroupBox *configGroup;
    QGroupBox *resultGroup;
    QGroupBox *bruteForceConfigGroup;

    // Brute-force specific widgets
    QSpinBox *minLengthSpin;
    QSpinBox *maxLengthSpin;
    QCheckBox *lowercaseCheck;
    QCheckBox *uppercaseCheck;
    QCheckBox *digitsCheck;
    QCheckBox *specialCharsCheck;
    QLabel *progressLabel;
    QLabel *estimationLabel;

    // État
    bool isProcessing;
    QThread* workerThread;
};
