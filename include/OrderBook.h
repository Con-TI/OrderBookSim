#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <string>
#include <set>
#include <vector>
#include <map>
#include <numeric>
#include <algorithm>
#include <iomanip>
#include <iterator>
#include <random>
#include "Others.h"

using namespace std;

class OrderBook{
public:
    multimap<double, multimap<int,vector<int>>> buyOrders;
    multimap<double, multimap<int,vector<int>>> sellOrders;

    vector<double> findIntersectingPrices();

    void addOrder(const Order& order, const int& time);

    multimap<FillType,pair<double,multimap<OrderType,vector<int>>>> matchOrders();

    multimap<OrderType,pair<double,int>> compressBook();

    void resetBook();

};

#endif