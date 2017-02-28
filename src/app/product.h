#ifndef PRODUCT_H
#define PRODUCT_H

#include <QtGlobal>

class Product
{
public:
    enum Type
    {
        Stocked,
        NonStocked,
        Service
    };

    enum CostingMethod
    {
        Manual,
        Average,
        Last,
    };

    static QString costingMethodString(CostingMethod type);
    static QString typeString(Type type);
    static QString formatCode(quint16 id);
};


#endif // PRODUCT_H
