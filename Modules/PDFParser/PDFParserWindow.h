#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QProgressBar>
#include <QGroupBox>

class PDFParserWindow : public QDialog
{
    Q_OBJECT

public:
    explicit PDFParserWindow(QWidget *parent = nullptr);
    ~PDFParserWindow();

private slots:
    void onBrowseClicked();
    void onParseClicked();
    void onSupplierChanged(int index);

private:
    void setupUi();
    void applyModernStyle();
    void updateStatus(const QString &message, bool isError = false);
    void parsePdfFile();

    // Widgets
    QComboBox *supplierCombo;
    QLineEdit *filePathEdit;
    QPushButton *browseButton;
    QPushButton *parseButton;
    QPushButton *closeButton;
    QTextEdit *statusText;
    QProgressBar *progressBar;
    QLabel *supplierLabel;
    QLabel *fileLabel;
    QGroupBox *configGroup;
    QGroupBox *resultGroup;
};
