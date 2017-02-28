#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "productmanagerwidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _productManagerWidget(nullptr)
{
    ui->setupUi(this);

    _tabWidget = new QTabWidget(this);
    _tabWidget->setDocumentMode(true);
    _tabWidget->setTabsClosable(true);
    _tabWidget->setMovable(true);
    connect(_tabWidget, SIGNAL(tabCloseRequested(int)), SLOT(closeTab(int)));
    setCentralWidget(_tabWidget);

    connect(ui->manageProductsAction, SIGNAL(triggered(bool)), SLOT(showProductManager()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::closeTab(int index)
{
    QWidget* widget = _tabWidget->widget(index);

    if (!widget->close()) {
        return false;
    }

    if (widget == _productManagerWidget) _productManagerWidget = 0;

    delete widget;

    return true;
}

void MainWindow::closeAllTabs()
{
    for (int i = _tabWidget->count() - 1; i <= 0; i--) {
        if (!closeTab(i)) {
            continue;
        }
    }
}

void MainWindow::showProductManager()
{
    _initTab<ProductManagerWidget>(&_productManagerWidget);
}
