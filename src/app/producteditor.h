#ifndef PRODUCTEDITOR_H
#define PRODUCTEDITOR_H

#include <QWidget>

class QFrame;

namespace Ui {
    class ProductEditor;
}

class ProductEditor : public QWidget
{
    Q_OBJECT

public:
    quint16 id;

    class UomModel;
    class PriceModel;

    QFrame* mainFrame;
    Ui::ProductEditor *ui;
    UomModel* uomModel;
    PriceModel* priceModel;

    ProductEditor(QWidget *parent);
    ~ProductEditor();

    bool eventFilter(QObject *watched, QEvent *event);

    bool load(quint16 productId);
    bool duplicateFrom(quint16 productId);

signals:
    void duplicateRequested(quint16 id);
    void saved(quint16 id);
    void removed(quint16 id);

public slots:
    void save();
    void remove();

private slots:
    void onDuplicateActionTriggered();

private:
    QAction* duplicateAction;
    QAction* removeAction;
};


#endif // PRODUCTEDITOR_H
