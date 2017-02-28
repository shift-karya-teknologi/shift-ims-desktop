#include "product.h"
#include <QString>

QString Product::typeString(Product::Type type)
{
    switch (type) {
    case Stocked: return "Stok";
    case NonStocked: return "Non Stok";
    case Service: return "Jasa";
    }
}
