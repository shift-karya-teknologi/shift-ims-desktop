#include "producteditor.h"
#include "ui_producteditor.h"
#include "product.h"

#include <QAbstractTableModel>
#include <QToolBar>
#include <QTableView>
#include <QMessageBox>
#include <QTabWidget>
#include <QBoxLayout>
#include <QKeyEvent>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QTimer>

class ProductEditor::UomModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    static const int MaxCount = 5;

    struct Item
    {
        quint64 id;
        QString name;
        quint64 quantity;

        Item() : id(0), quantity(0) {}

        bool isNull() const {
            return id == 0 && quantity == 0 && name == QString();
        }
    };

    QList<Item> items;
    QList<quint64> deletedIds;

    QString baseUom;

    UomModel(QObject* parent)
        : QAbstractTableModel(parent)
    {
        items << Item();
    }

    bool load(quint16 productId)
    {
        QSqlQuery q("select id, name, quantity from product_uoms where productId=?", QSqlDatabase::database());
        q.bindValue(0, productId);
        if (!q.exec()) {
            qDebug() << q.lastError().text();
            return false;
        }

        beginResetModel();
        items.clear();
        while (q.next()) {
            Item item;
            item.id = q.value("id").toULongLong();
            item.name = q.value("name").toString();
            item.quantity = q.value("quantity").toULongLong();
            items << item;
        }
        if (items.size() < 5)
            items << Item();
        endResetModel();
        return true;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        if (parent.isValid())
            return 0;

        return items.size();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const
    {
        return parent.isValid() ? 0 : 3;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole) {
            if (orientation == Qt::Horizontal) {
                switch (section) {
                case 0: return "Nama Satuan";
                case 1: return "Kwantitas";
                case 2: return "Keterangan";
                }
            }
            else {
                Item item = items.at(section);
                if (item.isNull())
                    return "*";
                return QString::number(section + 1);
            }
        }

        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex &index) const
    {
        Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

        Item item = items.at(index.row());

        if ((item.isNull() && index.column() != 0) || index.column() == 2)
            return f;

        return f | Qt::ItemIsEditable;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        if (index.row() == items.size())
            return QVariant();

        Item item = items.at(index.row());

        if (role == Qt::DisplayRole) {
            switch (index.column()) {
            case 0: return item.name;
            case 1: return item.quantity ? QLocale().toString(item.quantity) : QVariant();
            case 2: return item.isNull() ? QVariant()
                                         : QString("1 %2 = %3 %4").arg(item.name, QLocale().toString(item.quantity), baseUom.size() != 0 ? baseUom : "satuan");
            }
        }
        else if (role == Qt::EditRole) {
            switch (index.column()) {
            case 0: return item.name;
            case 1: return item.quantity;
            }
        }
        else if (role == Qt::TextAlignmentRole) {
            switch (index.column()) {
            case 1: return Qt::AlignRight ^ Qt::AlignVCenter;
            }
        }

        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)
    {
        if (role != Qt::EditRole || (index.row() == items.size() && index.column() != 0))
            return false;

        Item &item = items[index.row()];

        if (index.column() == 0) {
            QString name = value.toString().trimmed();
            if (name.isEmpty())
                return false;

            QString lcaseName = name.toLower();
            if (lcaseName == baseUom.toLower()) {
                return false;
            }

            for (int i = 0; i < items.size(); i++) {
                if (i == index.row())
                    continue;
                if (items.at(i).name.toLower() == lcaseName)
                    return false;
            }

            for (Item &tmp: items) {
                if (&tmp == &item)
                    continue;
                if (tmp.name.toLower() == lcaseName)
                    return false;
            }

            if (item.isNull() && index.row() < MaxCount - 1) {
                beginInsertRows(QModelIndex(), index.row() + 1, index.row() + 1);
                items << Item();
                endInsertRows();
                emit headerDataChanged(Qt::Horizontal, index.row(), index.row());
            }
            else if (index.row() == MaxCount - 1 && item.isNull()) {
                emit headerDataChanged(Qt::Horizontal, index.row(), index.row());
            }

            item.name = name;
            emit dataChanged(index, index.sibling(index.row(), columnCount() - 1));
            return true;
        }
        else if (index.column() == 1) {
            quint64 quantity = value.value<quint64>();
            if (!quantity) return false;
            item.quantity = quantity;
            emit dataChanged(index.sibling(index.row(), 0), index.sibling(index.row(), columnCount() - 1));
            return true;
        }

        return false;
    }

    void removeItemAt(int row)
    {
        if (row < 0 || row >= items.size())
            return;

        Item item = items.at(row);
        if (item.isNull())
            return;

        if (row == MaxCount - 1) {
            items[row] = Item();
            emit dataChanged(index(row, 0), index(row, columnCount() - 1));
        }
        else {
            beginRemoveRows(QModelIndex(), row, row);
            items.removeAt(row);
            endRemoveRows();
        }

        if (item.id) deletedIds << item.id;
    }

public slots:
    void updateBaseUom(const QString& uom)
    {
        baseUom = uom;

        if (items.size() > 0)
            emit dataChanged(index(0, 2), index(items.size() - 1, 2));
    }

};

class ProductEditor::PriceModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    static const int MaxCount = 5;

    typedef QPair<qulonglong, qulonglong> ItemPricePair;
    typedef QPair<qulonglong, qulonglong> QuantityPair;

    struct Item
    {
        quint64 id;
        QuantityPair quantity;
        ItemPricePair price1;
        ItemPricePair price2;
        ItemPricePair price3;

        Item() : id(0), quantity(ItemPricePair(0, 0)),
            price1(ItemPricePair(0, 0)), price2(ItemPricePair(0, 0)), price3(ItemPricePair(0, 0))
        {}

        QString quantityString() const {
            if (quantity.first && quantity.first == quantity.second)
                return QLocale().toString(quantity.first);
            else if (quantity.first && !quantity.second)
                return QString(">= %1").arg(QLocale().toString(quantity.first));
            else if (quantity.first < quantity.second)
                return QString("%1 - %2").arg(QLocale().toString(quantity.first), QLocale().toString(quantity.second));

            return QString();
        }

        QString price1String() const { return priceString(price1); }
        QString price2String() const { return priceString(price2); }
        QString price3String() const { return priceString(price3); }

        QString priceString(const ItemPricePair& p) const {
            if (p.first && p.second) {
                if (p.first == p.second)
                    return QLocale().toString(p.first);
                return QString("%1 - %2").arg(QLocale().toString(p.first), QLocale().toString(p.second));
            }

            return QString();
        }

        bool isNull() const {
            return id == 0
                && quantity.first == 0 && quantity.second == 0
                && price1.first == 0 && price1.second == 0
                && price2.first == 0 && price2.second == 0
                && price3.first == 0 && price3.second == 0;
        }
    };

    QList<Item> items;
    QList<quint64> deletedIds;

    QString baseUom;

    PriceModel(QObject* parent)
        : QAbstractTableModel(parent)
    {
        items << Item();
    }

    bool load(quint16 productId)
    {
        QSqlQuery q("select * from product_prices where productId=?", QSqlDatabase::database());
        q.bindValue(0, productId);
        if (!q.exec()) {
            qDebug() << q.lastError().text();
            return false;
        }

        beginResetModel();
        items.clear();
        while (q.next()) {
            Item item;
            item.id = q.value("id").toULongLong();
            item.quantity.first  = q.value("quantityMin").toULongLong();
            item.quantity.second = q.value("quantityMax").toULongLong();
            item.price1.first  = q.value("price1Min").toULongLong();
            item.price1.second = q.value("price1Max").toULongLong();
            item.price2.first  = q.value("price2Min").toULongLong();
            item.price2.second = q.value("price2Max").toULongLong();
            item.price3.first  = q.value("price3Min").toULongLong();
            item.price3.second = q.value("price3Max").toULongLong();
            items << item;
        }
        endResetModel();

        addDummyRow();

        return true;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        return parent.isValid() ? 0 : items.size();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const
    {
        return parent.isValid() ? 0 : 4;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole) {
            if (orientation == Qt::Horizontal) {
                switch (section) {
                case 0: return "Kwantitas";
                case 1: return "Harga 1";
                case 2: return "Harga 2";
                case 3: return "Harga 3";
                }
            }
            else {
                Item item = items.at(section);
                if (item.isNull())
                    return "*";
                return QString::number(section + 1);
            }
        }

        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex &/*index*/) const
    {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        if (index.row() == items.size())
            return QVariant();

        Item item = items.at(index.row());

        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            switch (index.column()) {
            case 0: return item.quantityString();
            case 1: return item.price1String();
            case 2: return item.price2String();
            case 3: return item.price3String();
            }
        }
        else if (role == Qt::TextAlignmentRole) {
            return Qt::AlignCenter;
        }

        return QVariant();
    }

    void addDummyRow() {
        int row = items.size();
        beginInsertRows(QModelIndex(), row, row);
        items << Item();
        endInsertRows();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)
    {
        if (role != Qt::EditRole || (index.row() == items.size() && index.column() != 0))
            return false;

        QString str = value.toString().trimmed();
        if (str.isEmpty())
            return false;

        Item &item = items[index.row()];
        qulonglong min = 0;
        qulonglong max = 0;

        if (index.column() == 0) {
            if (str.startsWith(">=")) {
                min = QLocale().toULongLong(str.replace(">=", "").trimmed());
            }
            else if (str.contains("-")) {
                QStringList strList = str.split("-");
                if (strList.size() != 2)
                    return false;
                min = QLocale().toULongLong(strList.first().trimmed());
                max = QLocale().toULongLong(strList.last().trimmed());
                if (min >= max)
                    return false;
            }
            else {
                min = max = QLocale().toULongLong(str);
                if (min <= 0)
                    return false;
            }

            if (index.row() == items.size() - 1)
                addDummyRow();

            item.quantity.first = min;
            item.quantity.second = max;
            emit dataChanged(index, index);
            return true;
        }
        else if (index.column() >= 1 && index.column() <= 3) {
            if (str.contains("-")) {
                QStringList strList = str.split("-");
                if (strList.size() != 2)
                    return false;
                min = QLocale().toULongLong(strList.first().trimmed());
                max = QLocale().toULongLong(strList.last().trimmed());
                if (min >= max)
                    return false;
            }
            else {
                min = max = QLocale().toULongLong(str);
            }

            if (index.row() == items.size() - 1)
                addDummyRow();

            if (index.column() == 1) {
                item.price1.first  = min;
                item.price1.second = max;
            }
            else if (index.column() == 2) {
                item.price2.first  = min;
                item.price2.second = max;
            }
            else if (index.column() == 3) {
                item.price3.first  = min;
                item.price3.second = max;
            }
            emit dataChanged(index, index);
            return true;
        }

        return false;
    }

    void removeItemAt(int row)
    {
        if (row < 0 || row >= items.size())
            return;

        Item item = items.at(row);
        if (item.isNull())
            return;

        beginRemoveRows(QModelIndex(), row, row);
        items.removeAt(row);
        endRemoveRows();

        if (item.id) deletedIds << item.id;
    }
};

ProductEditor::ProductEditor(QWidget *parent)
    : QWidget(parent)
    , id(0)
    , ui(new Ui::ProductEditor)
{
    uomModel = new UomModel(this);
    priceModel = new PriceModel(this);

    QToolBar* toolBar = new QToolBar(this);
    QAction* saveAction = toolBar->addAction("Simpan");
    connect(saveAction, SIGNAL(triggered(bool)), SLOT(save()));

    duplicateAction = toolBar->addAction("Duplikat");
    duplicateAction->setEnabled(false);
    connect(duplicateAction, SIGNAL(triggered(bool)), SLOT(onDuplicateActionTriggered()));

    toolBar->addSeparator();

    removeAction = toolBar->addAction("Hapus");
    removeAction->setEnabled(false);
    connect(removeAction, SIGNAL(triggered(bool)), SLOT(remove()));

    mainFrame = new QFrame(this);
    ui->setupUi(mainFrame);

    ui->typeComboBox->addItem(Product::typeString(Product::Type::Stocked), Product::Type::Stocked);
    ui->typeComboBox->addItem(Product::typeString(Product::Type::NonStocked), Product::Type::NonStocked);
    ui->typeComboBox->addItem(Product::typeString(Product::Type::Service), Product::Type::Service);
    ui->typeComboBox->setCurrentIndex(ui->typeComboBox->findData(Product::Type::Stocked));

    ui->costingMethodComboBox->addItem(Product::costingMethodString(Product::CostingMethod::Manual), Product::CostingMethod::Manual);
    ui->costingMethodComboBox->addItem(Product::costingMethodString(Product::CostingMethod::Average), Product::CostingMethod::Average);
    ui->costingMethodComboBox->addItem(Product::costingMethodString(Product::CostingMethod::Last), Product::CostingMethod::Last);
    ui->costingMethodComboBox->setCurrentIndex(ui->costingMethodComboBox->findData(Product::CostingMethod::Average));

    connect(ui->baseUomEdit, SIGNAL(textEdited(QString)), uomModel, SLOT(updateBaseUom(QString)));

    ui->uomTableView->installEventFilter(this);
    ui->uomTableView->setModel(uomModel);

    ui->priceTableView->installEventFilter(this);
    ui->priceTableView->setModel(priceModel);

    QBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(toolBar);
    mainLayout->addWidget(mainFrame);

    setWindowTitle("Produk Baru");
    QTimer::singleShot(0, ui->nameEdit, SLOT(setFocus()));
}

ProductEditor::~ProductEditor()
{
    delete ui;
}

bool ProductEditor::load(quint16 productId) {
    id = productId;

    QSqlQuery q("select * from products where id=?", QSqlDatabase::database());
    q.bindValue(0, productId);
    if (!q.exec()) {
        qDebug() << q.lastError().text();
        return false;
    }

    if (!q.next()) {
        return false;
    }

    quint8 type = q.value("type").value<quint8>();
    if (type >= 200)
        return false;

    QString productCode = Product::formatCode(id);
    QString baseUom = q.value("baseUom").toString();

    ui->idEdit->setText(productCode);
    ui->nameEdit->setText(q.value("name").toString());
    ui->typeComboBox->setCurrentIndex(ui->typeComboBox->findData(type));
    ui->statusComboBox->setCurrentIndex(q.value("active").toBool());
    uomModel->load(id);
    ui->baseUomEdit->setText(baseUom);
    uomModel->updateBaseUom(baseUom);
    priceModel->load(id);
    ui->costingMethodComboBox->setCurrentIndex(ui->costingMethodComboBox->findData(q.value("costingMethod").toInt()));
    ui->manualCostEdit->setText(QLocale().toString(q.value("manualCost").toULongLong()));
    ui->averageCostEdit->setText(QLocale().toString(q.value("averageCost").toULongLong()));
    ui->lastPurchaseCostEdit->setText(QLocale().toString(q.value("lastPurchaseCost").toULongLong()));

    duplicateAction->setEnabled(true);
    removeAction->setEnabled(true);
    setWindowTitle(productCode);

    return true;
}

bool ProductEditor::duplicateFrom(quint16 productId)
{
    if (!load(productId))
        return false;

    id = 0;
    for (UomModel::Item& item: uomModel->items)
        item.id = 0;
    for (PriceModel::Item& item: priceModel->items)
        item.id = 0;

    setWindowTitle("Produk Baru");
    ui->idEdit->clear();
    duplicateAction->setEnabled(false);
    removeAction->setEnabled(false);

    QTimer::singleShot(0, ui->nameEdit, SLOT(setFocus()));
    QTimer::singleShot(0, ui->nameEdit, SLOT(selectAll()));

    return true;
}

bool ProductEditor::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui->uomTableView) {
        if (event->type() == QEvent::KeyRelease) {
            QKeyEvent* e = static_cast<QKeyEvent*>(event);
            if (e->key() == Qt::Key_Delete) {
                if (QMessageBox::question(0, "Konfirmasi", "Hapus satuan?", "&Ya", "&Tidak"))
                uomModel->removeItemAt(ui->uomTableView->currentIndex().row());
                return true;
            }
        }
    }
    else if (object == ui->priceTableView) {
        if (event->type() == QEvent::KeyRelease) {
            QKeyEvent* e = static_cast<QKeyEvent*>(event);
            if (e->key() == Qt::Key_Delete) {
                if (QMessageBox::question(0, "Konfirmasi", "Hapus harga?", "&Ya", "&Tidak"))
                priceModel->removeItemAt(ui->priceTableView->currentIndex().row());
                return true;
            }
        }
    }

    return QWidget::eventFilter(object, event);
}

void ProductEditor::save()
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);

    bool isNewRecord = id == 0;
    QString name = ui->nameEdit->text().trimmed();
    quint8 type = ui->typeComboBox->currentData().toInt();
    bool active = ui->statusComboBox->currentIndex();
    QString baseUom = ui->baseUomEdit->text().trimmed();
    quint8 costingMethod = ui->costingMethodComboBox->currentData().toInt();
    int cost = 0;
    int manualCost = QLocale().toInt(ui->manualCostEdit->text());
    int averageCost = QLocale().toInt(ui->averageCostEdit->text());
    int lastPurchaseCost = QLocale().toInt(ui->lastPurchaseCostEdit->text());

    if (name.isEmpty()) {
        ui->nameEdit->setFocus();
        QMessageBox::warning(0, "Peringatan", "Nama produk harus diisi!");
        return;
    }

    if (isNewRecord) {
        q.prepare("select count(0) from products where name=?");
        q.bindValue(0, name);
    }
    else {
        q.prepare("select count(0) from products where name=? and id<>?");
        q.bindValue(0, name);
        q.bindValue(1, id);
    }
    q.exec();
    q.next();
    if (q.value(0).toInt() > 0) {
        ui->nameEdit->setFocus();
        ui->nameEdit->selectAll();
        QMessageBox::warning(0, "Peringatan", "Nama produk sudah digunakan!");
        return;
    }
    q.clear();

    if (baseUom.isEmpty()) {
        ui->baseUomEdit->setFocus();
        QMessageBox::warning(0, "Peringatan", "Nama satuan dasar harus diisi!");
        return;
    }

    switch (costingMethod) {
    case Product::CostingMethod::Manual:
        cost = manualCost;
        break;
    case Product::CostingMethod::Average:
        cost = averageCost;
        break;
    case Product::CostingMethod::Last:
        cost = lastPurchaseCost;
        break;
    }

    if (name.isEmpty()) {
        ui->nameEdit->setFocus();
        return;
    }

    db.transaction();

    if (id == 0) {
        q.prepare("insert into products"
                  "( name, type, active, baseUom, costingMethod, cost, manualCost, averageCost, lastPurchaseCost)"
                  " values"
                  "(:name,:type,:active,:baseUom,:costingMethod,:cost,:manualCost,:averageCost,:lastPurchaseCost)");
    }
    else {
        q.prepare("update products set"
                  " name=:name, type=:type, active=:active, baseUom:=baseUom,"
                  " costingMethod=:costingMethod, cost=:cost,"
                  " manualCost=:manualCost, averageCost=:averageCost, lastPurchaseCost=:lastPurchaseCost"
                  " where id=:id");
        q.bindValue(":id", id);
    }

    q.bindValue(":name", name);
    q.bindValue(":type", type);
    q.bindValue(":active", active);
    q.bindValue(":baseUom", baseUom);
    q.bindValue(":costingMethod", costingMethod);
    q.bindValue(":cost", cost);
    q.bindValue(":manualCost", manualCost);
    q.bindValue(":averageCost", averageCost);
    q.bindValue(":lastPurchaseCost", lastPurchaseCost);

    if (!q.exec()) {
        qDebug() << __FILE__ << __LINE__ << q.lastError().text();
        db.rollback();
        return;
    }

    if (id == 0) {
        id = q.lastInsertId().value<quint16>();
        QString idText = Product::formatCode(id);
        setWindowTitle(idText);
        ui->idEdit->setText(idText);
    }

    QSqlQuery q2(db);
    for (UomModel::Item &item: uomModel->items) {
        if (item.isNull())
            continue;

        if (!item.id) {
            q2.prepare("insert into product_uoms(productId,name,quantity)"
                       " values(:productId,:name,:quantity)");
            q2.bindValue(":productId", id);
        }
        else {
            q2.prepare("update product_uoms set name=:name, quantity=:quantity where id=:id");
            q2.bindValue(":id", item.id);
        }
        q2.bindValue(":name", item.name);
        q2.bindValue(":quantity", item.quantity);
        if (!q2.exec()) {
            qDebug() << __FILE__ << __LINE__ << q2.lastError().text();
            db.rollback();
            return;
        }

        if (!item.id)
            item.id = q2.lastInsertId().toULongLong();
    }

    q2.clear();
    for (PriceModel::Item &item: priceModel->items) {
        if (item.isNull())
            continue;

        if (!item.id) {
            q2.prepare("insert into product_prices"
                       "( productId, quantityMin, quantityMax, price1Min, price1Max, price2Min, price2Max, price3Min, price3Max) values"
                       "(:productId,:quantityMin,:quantityMax,:price1Min,:price1Max,:price2Min,:price2Max,:price3Min,:price3Max)");
            q2.bindValue(":productId", id);
        }
        else {
            q2.prepare("update product_prices set"
                       " quantityMin=:quantityMin, quantityMax=:quantityMax,"
                       " price1Min=:price1Min, price1Max=:price1Max,"
                       " price2Min=:price2Min, price2Max=:price2Max,"
                       " price3Min=:price3Min, price3Max=:price3Max"
                       " where id=:id");
            q2.bindValue(":id", item.id);
        }
        q2.bindValue(":quantityMin", item.quantity.first);
        q2.bindValue(":quantityMax", item.quantity.second);
        q2.bindValue(":price1Min", item.price1.first);
        q2.bindValue(":price1Max", item.price1.second);
        q2.bindValue(":price2Min", item.price2.first);
        q2.bindValue(":price2Max", item.price2.second);
        q2.bindValue(":price3Min", item.price3.first);
        q2.bindValue(":price3Max", item.price3.second);
        if (!q2.exec()) {
            qDebug() << __FILE__ << __LINE__ << q2.lastError().text();
            db.rollback();
            return;
        }

        if (!item.id)
            item.id = q2.lastInsertId().toULongLong();
    }

    q2.clear();
    for (qulonglong id: uomModel->deletedIds) {
        q2.prepare("delete from product_uoms where id=?");
        q2.bindValue(0, id);
        if (!q2.exec()) {
            qDebug() << __FILE__ << __LINE__ << q2.lastError().text();
            db.rollback();
            return;
        }
    }

    q2.clear();
    for (qulonglong id: priceModel->deletedIds) {
        q2.prepare("delete from product_prices where id=?");
        q2.bindValue(0, id);
        if (!q2.exec()) {
            qDebug() << __FILE__ << __LINE__ << q2.lastError().text();
            db.rollback();
            return;
        }
    }

    if (!db.commit()) {
        qDebug() << __FILE__ << __LINE__ << db.lastError().text();
        db.rollback();
        return;
    }

    if (isNewRecord) {
        duplicateAction->setEnabled(true);
        removeAction->setEnabled(true);
    }

    emit saved(id);
}

void ProductEditor::remove()
{
    if (QMessageBox::question(0, "Konfirmasi", "Hapus produk?", "&Ya", "&Tidak"))
        return;

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("delete from products where id=?");
    q.bindValue(0, id);
    if (!q.exec()) {
        qDebug() << __FILE__ << __LINE__ << db.lastError().text();
        return;
    }

    emit removed(id);
}

void ProductEditor::onDuplicateActionTriggered()
{
    if (id)
        emit duplicateRequested(id);
}

#include "producteditor.moc"
