#ifndef OTHERS_H
#define OTHERS_H

#include <map>

using namespace std;

enum class OrderType { BUY , SELL };
enum class FillType { FILLED , PARTIAL };
enum class TraderStatus { ORDERING , IDLE };
struct Order {
    int stockId;
    int traderId;
    double price;
    int quantity;
    OrderType type;

    Order(int sI,int tI, double p, double q, OrderType t) : stockId(sI), traderId(tI), price(p), quantity(q), type(t) {}
};

multimap<int, int> priceBands = {{0, 50}, {1, 200}, {2, 500}, {3, 2000}, {4, 5000}};
multimap<int, int> priceIncrements = {{0, 1}, {1, 2}, {2, 5}, {3, 10}, {4, 25}};

int checkPriceBand(double price) {
    if (price > 5000) return 4;
    if (price > 2000) return 3;
    if (price > 500) return 2;
    if (price > 200) return 1;
    return 0;
}

#endif