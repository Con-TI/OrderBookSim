#ifndef TRADER_H
#define TRADER_H

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

#include "ExchangeAgent.h"
#include "Others.h"

using namespace std;

class Trader{
public:
    int traderId;
    int intelligence;
    int trender;
    multimap<int,vector<double>> midPriceMemory;
    int smaLength;
    int hft;
    int meanReversion;
    int tradeFreq;

    multimap<int,pair<double,int>> holdings;
    int money;
    multimap<int,pair<double,int>> pendingBuyOrders;
    multimap<int,pair<double,int>> pendingSellOrders;
    multimap<int,ExchangeAgent>* stocksPtr;
    multimap<int,double> sentiments;

    TraderStatus status = TraderStatus::ORDERING;
    int timeTillNextTrade = 0;

    void addPendingBuyOrder(const Order& order);
    void addPendingSellOrder(const Order& order);
    void resetTimer();
    void resetSentiments();
    void analyzeStock(const int& stockId,const int& time);
    
    Trader(const int &id, multimap<int,ExchangeAgent>& exchangeAgentList);

    void updateTimer(const vector<int> & stockIds,const int & time);
    void updateSentiments(const int & time);
    void updatePriceMemory();
    void addHolding(const Order& order);
    void removeHolding(const Order& order);
    void receiveResponse(const Order& order);
};


#endif