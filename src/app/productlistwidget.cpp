#include "productlistwidget.h"
#include "product.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <QToolBar>
#include <QTableView>
#include <QHeaderView>
#include <QBoxLayout>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

class ProductListWidget::Model : public QAbstractTableModel
{
    Q_OBJECT

public:
    struct Item {
        quint16 id;
        quint8 type;
        bool active;
        QString code;
        QString name;

        QString statusString() const
        {
            return active ? "Aktif" : "Nonaktif";
        }

        QString typeString() const
        {
            switch (type) {
            case 0: return "Stok";
            case 1: return "Non Stok";
            case 2: return "Jasa";
            case 255: return "Voucher ShiftNet";
            }

            return QString();
        }
    };
    QList<Item> items;

    enum Column {
        Code,
        Name,
        Type,
        Active,
        _COUNT
    };

    Model(QObject* parent)
        : QAbstractTableModel(parent)
    {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        Q_UNUSED(parent)
        return items.size();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const
    {
        Q_UNUSED(parent)
        return Column::_COUNT;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        Item item = items.at(index.row());
        if (role == Qt::DisplayRole) {
            switch (index.column()) {
            case Column::Code: return item.code;
            case Column::Name: return item.name;
            case Column::Type: return item.typeString();
            case Column::Active: return item.statusString();
            }
        }
        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal) {
            if (role == Qt::DisplayRole) {
                switch (section) {
                case Column::Code: return "Kode";
                case Column::Name: return "Nama Produk";
                case Column::Type: return "Jenis";
                case Column::Active: return "Status";
                }
            }
        }

        return QVariant();
    }

public slots:
    bool refresh()
    {
        QSqlQuery q("select id, name, type, active from products where type <= 200", QSqlDatabase::database());
        if (!q.exec()) {
            qDebug() << "SQL ERROR:" << qPrintable(q.lastError().text());
            return false;
        }
        beginResetModel();
        items.clear();

        while (q.next()) {
            Item item;
            item.id = q.value("id").value<quint16>();
            item.name = q.value("name").toString();
            item.type = q.value("type").value<quint8>();
            item.active = q.value("active").toBool();
            item.code = Product::formatCode(item.id);
            items << item;
        }
        endResetModel();

        return true;
    }

};

class ProductListWidget::ProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    ProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {}
};

ProductListWidget::ProductListWidget(QWidget *parent)
    : QWidget(parent)
{
    QToolBar* toolBar = new QToolBar(this);
    QAction* refreshAction = toolBar->addAction("Refresh");
    connect(refreshAction, SIGNAL(triggered(bool)), SLOT(refresh()));
    QAction* newAction = toolBar->addAction("Tambah");
    connect(newAction, SIGNAL(triggered(bool)), SIGNAL(newActionTriggered()));

    model = new Model(this);
    model->refresh();

    proxyModel = new ProxyModel(this);
    proxyModel->setSourceModel(model);

    view = new QTableView(this);
    view->setAlternatingRowColors(true);
    view->setSortingEnabled(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->horizontalHeader()->setHighlightSections(false);
    view->verticalHeader()->setDefaultSectionSize(20);
    view->verticalHeader()->setMinimumSectionSize(20);
    view->verticalHeader()->setMaximumSectionSize(20);
    view->verticalHeader()->setVisible(false);
    view->setModel(proxyModel);

    connect(view, SIGNAL(activated(QModelIndex)), SLOT(_onViewActivated(QModelIndex)));

    QBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(toolBar);
    mainLayout->addWidget(view);
}

void ProductListWidget::_onViewActivated(const QModelIndex& index)
{
    QModelIndex srcIndex = proxyModel->mapToSource(index);
    Model::Item item = model->items.at(srcIndex.row());
    emit activated(item.id);
}

void ProductListWidget::refresh()
{
    model->refresh();
}

#include "productlistwidget.moc"
