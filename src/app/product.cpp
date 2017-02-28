#include "product.h"
#include <QString>

QString Product::typeString(Product::Type type)
{
    switch (type) {
    case Stocked: return "Stok";
    case NonStocked: return "Non Stok";
    case Service: return "Jasa";
    }
    return QString();
}

QString Product::costingMethodString(CostingMethod type)
{
    switch (type) {
    case Manual: return "Harga Beli Manual";
    case Average: return "Harga Beli Rata-rata";
    case Last: return "Harga Beli Terakhir";
    }
    return QString();
}

QString Product::formatCode(quint16 id)
{
    return QString("P-%1").arg(id, 5, 10, QChar('0'));
}
