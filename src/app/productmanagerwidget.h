#ifndef PRODUCTMANAGERWIDGET_H
#define PRODUCTMANAGERWIDGET_H

#include <QSplitter>

class QTabWidget;
class ProductListWidget;
class ProductEditor;

class ProductManagerWidget : public QSplitter
{
    Q_OBJECT

public:
    explicit ProductManagerWidget(QWidget *parent = 0);

signals:

public slots:
    void newProduct();
    void editProduct(quint16 id);
    void duplicateProduct(quint16 fromId);

    bool closeTab(int index);
    void closeAllTabs();

private:
    void setupTab(QWidget* widget);
    void handleEditorSignals(ProductEditor* editor);


private slots:
    void updateTabText(const QString& title);
    void handleProductRemoved();

private:
    ProductListWidget* _listWidget;
    QTabWidget* _editorsTabWidget;
    QHash<quint16, QWidget*> _editorByIds;
};

#endif // PRODUCTMANAGERWIDGET_H
