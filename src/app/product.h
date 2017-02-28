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

    static QString typeString(Type type);
};


#endif // PRODUCT_H
