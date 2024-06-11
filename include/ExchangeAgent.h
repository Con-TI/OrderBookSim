#ifndef EXCHANGEAGENT_H
#define EXCHANGEAGENT_H

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
#include "color.cpp"

#include "Trader.h"
#include "OrderBook.h"

using namespace std;

class ExchangeAgent{
public:
    OrderBook Book;
    int stockId;
    multimap<OrderType,pair<double,int>> neatBook;
    int startingPriceBand;
    int minPrice;
    int maxPrice;
    int increment;
    double startingPrice;
    int shares;
    multimap<int,Trader>* tradersPtr;

    ExchangeAgent(int id);

    double baseFundamentalValue;
    double midPrice;
    double bidAskSpread;
    double lowestAsk;
    double highestBid;

    void setTradersList(multimap<int,Trader>& tradersList);

    void updatefundamentalValue();

    void initialDistributionOfStocks(multimap<int,Trader>& traders);

    void traderToExchangeLimit(const Order& order, const int& time);

    void endDay();

    void compileBook();

    void displayBook();
};

#endif