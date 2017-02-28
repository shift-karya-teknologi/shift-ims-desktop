#ifndef PRODUCTLISTWIDGET_H
#define PRODUCTLISTWIDGET_H

#include <QTableView>

class ProductListWidget : public QWidget
{
    Q_OBJECT

private:
    class Model;
    class ProxyModel;

public:
    QTableView* view;
    Model* model;
    ProxyModel* proxyModel;

    ProductListWidget(QWidget *parent = 0);

signals:
    void newActionTriggered();
    void activated(quint16 id);

private slots:
    void _onViewActivated(const QModelIndex& index);

public slots:
    void refresh();
};

#endif // PRODUCTLISTWIDGET_H
