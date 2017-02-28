#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class ProductManagerWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget*parent = 0);
    ~MainWindow();

public slots:
    void showProductManager();
    bool closeTab(int index);
    void closeAllTabs();

private:
    template <typename T> void _initTab(T** widget) {
        int index = -1;
        if (*widget == nullptr) {
            *widget = new T(this);
            index = _tabWidget->addTab(*widget, (*widget)->windowIcon(), (*widget)->windowTitle());
        }
        else {
            index = _tabWidget->indexOf(*widget);
        }

        _tabWidget->setCurrentIndex(index);
    }

private:
    Ui::MainWindow *ui;
    QTabWidget *_tabWidget;

    ProductManagerWidget *_productManagerWidget;
};

#endif // MAINWINDOW_H
