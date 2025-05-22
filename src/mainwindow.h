#pragma once


#include <QMainWindow>
#include "commondef.h"

namespace Ui {
class MainWindow;
}

class MainView;
class QToolButton;
class QAction;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    MainView *m_view{};
    QToolBar *m_toolbar;
    QAction *m_startAction;
    QToolButton *m_toolButton;
};
