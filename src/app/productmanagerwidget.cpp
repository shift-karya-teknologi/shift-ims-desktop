#include "productmanagerwidget.h"
#include "producteditor.h"
#include "productlistwidget.h"

#include <QTabWidget>

ProductManagerWidget::ProductManagerWidget(QWidget *parent)
    : QSplitter(parent)
{
    setWindowTitle("Produk");

    _listWidget = new ProductListWidget(this);
    connect(_listWidget, SIGNAL(newActionTriggered()), SLOT(newProduct()));
    connect(_listWidget, SIGNAL(activated(quint16)), SLOT(editProduct(quint16)));

    _editorsTabWidget = new QTabWidget(this);
    _editorsTabWidget->setDocumentMode(true);
    _editorsTabWidget->setTabsClosable(true);
    _editorsTabWidget->setMovable(true);
    _editorsTabWidget->hide();
    connect(_editorsTabWidget, SIGNAL(tabCloseRequested(int)), SLOT(closeTab(int)));
}

bool ProductManagerWidget::closeTab(int index)
{
    ProductEditor* editor = qobject_cast<ProductEditor*>(_editorsTabWidget->widget(index));

    if (!editor->close()) {
        return false;
    }

    if (_editorByIds.contains(editor->id))
        _editorByIds.remove(editor->id);

    delete editor;

    if (_editorsTabWidget->count() == 0) {
        _editorsTabWidget->hide();
    }

    return true;
}

void ProductManagerWidget::closeAllTabs()
{
    for (int i = _editorsTabWidget->count() - 1; i <= 0; i--) {
        if (!closeTab(i)) {
            continue;
        }
    }
}

void ProductManagerWidget::newProduct()
{
    ProductEditor *editor = new ProductEditor(_editorsTabWidget);
    setupTab(editor);
    handleEditorSignals(editor);
}

void ProductManagerWidget::duplicateProduct(quint16 fromId)
{
    ProductEditor *editor = new ProductEditor(_editorsTabWidget);
    if (!editor->duplicateFrom(fromId)) {
        delete editor;
        return;
    }

    setupTab(editor);
    handleEditorSignals(editor);
}

void ProductManagerWidget::editProduct(quint16 id)
{
    QWidget* existingWidget = _editorByIds.value(id, 0);

    if (existingWidget) {
        _editorsTabWidget->setCurrentWidget(existingWidget);
        return;
    }

    ProductEditor *editor = new ProductEditor(_editorsTabWidget);
    if (!editor->load(id)) {
        delete editor;
        return;
    }

    _editorByIds.insert(id, editor);
    setupTab(editor);
    handleEditorSignals(editor);
}

void ProductManagerWidget::setupTab(QWidget* widget)
{
    int index = _editorsTabWidget->addTab(widget, widget->windowIcon(), widget->windowTitle());
    _editorsTabWidget->setCurrentIndex(index);
    if (!_editorsTabWidget->isVisible()) {
        _editorsTabWidget->show();
    }
}

void ProductManagerWidget::updateTabText(const QString& title)
{
    QWidget* widget = qobject_cast<QWidget*>(sender());
    _editorByIds.insert(qobject_cast<ProductEditor*>(widget)->id, widget);
    _editorsTabWidget->setTabText(_editorsTabWidget->indexOf(widget), title);
}

void ProductManagerWidget::handleEditorSignals(ProductEditor* editor)
{
    connect(editor, SIGNAL(duplicateRequested(quint16)), SLOT(duplicateProduct(quint16)));
    connect(editor, SIGNAL(saved(quint16)), _listWidget, SLOT(refresh()));
    connect(editor, SIGNAL(removed(quint16)), SLOT(handleProductRemoved()));
    connect(editor, SIGNAL(windowTitleChanged(QString)), SLOT(updateTabText(QString)));
}

void ProductManagerWidget::handleProductRemoved()
{
    _listWidget->refresh();
    ProductEditor* editor = qobject_cast<ProductEditor*>(sender());
    closeTab(_editorsTabWidget->indexOf(editor));
}
