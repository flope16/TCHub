#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onPdfParserClicked();

private:
    void setupUi();
    void applyModernStyle();

    // Widgets
    QWidget *centralWidget;
    QLabel *titleLabel;
    QLabel *logoLabel;
    QGroupBox *modulesGroup;
    QPushButton *pdfParserButton;
    QPushButton *quitButton;
};
