#include "Trader.h"

int traderId;

int intelligence = rand()%100;
int trender = rand()%100;
multimap<int,vector<double>> midPriceMemory;
int smaLength = rand()%30+10;
int hft = rand()%100;
int meanReversion = rand()%100;
int tradeFreq = rand()%100*hft/10000;

multimap<int,pair<double,int>> holdings;
int money = rand()%100000000 + 500000;
multimap<int,pair<double,int>> pendingBuyOrders;
multimap<int,pair<double,int>> pendingSellOrders;

multimap<int,ExchangeAgent>* stocksPtr;
multimap<int,double> sentiments;

TraderStatus status = TraderStatus::ORDERING;
int timeTillNextTrade = 0;

void addPendingBuyOrder(const Order& order){
    int stockId = order.stockId;
    double orderPrice = order.price;
    int orderQuantity = order.quantity;
    if (pendingBuyOrders.find(stockId)!=pendingBuyOrders.end()){
        int existingQuantity = (pendingBuyOrders.find(stockId) ->second).second;
        existingQuantity += orderQuantity;
        pair<double,int> priceQuantityPair = {orderPrice,existingQuantity};
        pendingBuyOrders.find(stockId) -> second = priceQuantityPair;
    }
    else{
        pair<double,int> priceQuantityPair = {orderPrice,orderQuantity};
        pendingBuyOrders.insert(make_pair(stockId,priceQuantityPair));
    }
};

void addPendingSellOrder(const Order& order){
    int stockId = order.stockId;
    double orderPrice = order.price;
    int orderQuantity = order.quantity;
    if (pendingSellOrders.find(stockId)!=pendingSellOrders.end()){
        int existingQuantity = (pendingSellOrders.find(stockId) ->second).second;
        existingQuantity += orderQuantity;
        pair<double,int> priceQuantityPair = {orderPrice,existingQuantity};
        pendingSellOrders.find(stockId) -> second = priceQuantityPair;
    }
    else{
        pair<double,int> priceQuantityPair = {orderPrice,orderQuantity};
        pendingSellOrders.insert(make_pair(stockId,priceQuantityPair));
    }
};

void resetTimer(){
    random_device rd;
    mt19937 gen(rd());
    binomial_distribution<> binomDis(5,tradeFreq);
    timeTillNextTrade = 6 - binomDis(gen);
    status = TraderStatus::IDLE;
};

void resetSentiments(){
    for (auto i=sentiments.begin();i!=sentiments.end();++i){
        i->second = 0;
    }
};

void analyzeStock(const int& stockId,const int& time){
    // Ranges between -100 and 100
    ExchangeAgent& exchange = (*stocksPtr).find(stockId) -> second;
    double evalScore;
    
    double noise = ((rand()%2)*2-1)*(rand()%(100-intelligence))/20;

    vector<double>& priceMemoryVec = midPriceMemory.find(stockId)->second;
    double mean = accumulate(priceMemoryVec.begin(),priceMemoryVec.end(),0.0)/priceMemoryVec.size();
    double trend;
    if (mean*(1+smaLength/50)<priceMemoryVec.back()){
        trend = priceMemoryVec.back()/mean;
    } 
    else if (mean>priceMemoryVec.back()*(1+smaLength/50)){
        trend = -mean/priceMemoryVec.back();
    }
    else{
        trend=0;
    }
    double trendScore = trend*(rand()%trender);

    double& spread = exchange.bidAskSpread;
    double hftScore = spread*(rand()%hft)/2;

    double personalSentiment = sentiments.find(stockId)->second;
    double relFundamentalValue = exchange.baseFundamentalValue + personalSentiment;
    double meanRevScore = (relFundamentalValue-priceMemoryVec.back())*(rand()%meanReversion)*(rand()%meanReversion)*2/(relFundamentalValue+priceMemoryVec.back());

    evalScore = (trendScore + hftScore + meanRevScore)*(intelligence/50) + noise;
    evalScore /= pow(10,ceil(log10(evalScore)));
    
    if (evalScore>0){
        multimap<int,double> possiblePrices;
        double lowestAsk = exchange.lowestAsk;
        possiblePrices.insert(make_pair(7,lowestAsk));
        for (int i=6;i!=0;--i){
            int band = checkPriceBand(lowestAsk);
            int increment = priceIncrements.find(band) -> second;
            double nextPrice = lowestAsk-increment;
            possiblePrices.insert(make_pair(i,nextPrice));
        }
        random_device rd;
        mt19937 gen(rd());
        binomial_distribution<> binomDis(7,evalScore);
        int x = binomDis(gen);
        double orderPrice = possiblePrices.find(x) -> second;
        int orderQuantity = rand()%int(floor(money/orderPrice/10));
        Order order(stockId,traderId,orderPrice,orderQuantity,OrderType::BUY);
        if (orderQuantity != 0){
            exchange.traderToExchangeLimit(order,time);
            addPendingBuyOrder(order);
        }
    } 
    else if (evalScore<0 && holdings.find(stockId)!=holdings.end()){
        double highestBid = exchange.highestBid;
        multimap<int,double> possiblePrices;
        possiblePrices.insert(make_pair(7,highestBid));
        for (int i=6;i!=0;--i){
            int band = checkPriceBand(highestBid);
            int increment = priceIncrements.find(band) -> second;
            double nextPrice = highestBid + increment;
            possiblePrices.insert(make_pair(i,nextPrice));
        }
        random_device rd;
        mt19937 gen(rd());
        binomial_distribution<> binomDis(7,evalScore);
        int x = binomDis(gen);
        double orderPrice = possiblePrices.find(x) -> second;
        int holdingsQuantity = (holdings.find(stockId)->second).second;
        int orderQuantity = rand()%holdingsQuantity;
        Order order(stockId,traderId,orderPrice,orderQuantity,OrderType::SELL);
        if (orderQuantity != 0){
            exchange.traderToExchangeLimit(order,time);
            addPendingSellOrder(order);
        }
    }
};


Trader::Trader(const int &id, multimap<int,ExchangeAgent>& exchangeAgentList){
    traderId = id;
    stocksPtr = &exchangeAgentList;
    for (auto i=exchangeAgentList.begin();i!=exchangeAgentList.end();++i){
        int stockId = i->first;
        sentiments.insert(make_pair(stockId,0));
    }
};

void Trader::updateTimer(const vector<int> & stockIds,const int & time){
    timeTillNextTrade -= 1;
    if (timeTillNextTrade <= 0){
        status = TraderStatus::ORDERING;
        int numStocks = stockIds.size();
        int divisor = ceil(numStocks/5);
        for (int id:stockIds){
            if(rand()%divisor==0){
                analyzeStock(id,time);
            }
        }
        resetTimer();
    }
};

void updateSentiments(const int & time){
    if (time==0){
        resetSentiments();
    } 
    else{
        for (auto i=sentiments.begin();i!=sentiments.end();++i){
            i->second += rand()%3;
        }
    }
};

void updatePriceMemory(){
    multimap<int,ExchangeAgent>& exchangeAgentList = *stocksPtr;
    for (auto i=exchangeAgentList.begin();i!=exchangeAgentList.end();++i){
        int stockId = i->first;
        ExchangeAgent& exchange = i->second;
        double midPrice = exchange.midPrice;
        if (midPriceMemory.find(stockId) == midPriceMemory.end()){
            vector<double> vec = {midPrice};
            midPriceMemory.insert(make_pair(stockId,vec)); 
        }
        else{
            vector<double> vec = midPriceMemory.find(stockId)->second;
            vec.push_back(midPrice);
            if (vec.size() > smaLength){
                vec.erase(vec.begin());
            }
            midPriceMemory.find(stockId)->second = vec;
        }
    }
};

void addHolding(const Order& order){
    if (holdings.find(order.stockId)==holdings.end()){
        pair<double,int> priceQuantityPair = {order.price,order.quantity};
        holdings.insert(make_pair(order.stockId, priceQuantityPair));
    }
    else{
        pair<double,int>& pair = holdings.find(order.stockId)->second;
        pair.second += order.quantity;
    }
};

void removeHolding(const Order& order){
    pair<double,int>& pair = holdings.find(order.stockId) -> second;
    pair.second -= order.quantity;
    if (pair.second==0){
        holdings.erase(order.stockId);
    }
};

void receiveResponse(const Order& order){
    if (order.type == OrderType::BUY){
        int qBef = (pendingBuyOrders.find(order.stockId)->second).second;
        addPendingBuyOrder(order);
        int qAft = (pendingBuyOrders.find(order.stockId)->second).second;
        if (qAft == 0){
            pendingBuyOrders.erase(order.stockId);
        }
        Order forHoldings(order.stockId,-1,order.price,qBef-qAft,order.type);
        addHolding(forHoldings);
        money -= abs(order.price*order.quantity);
    }
    else{
        int qBef = (pendingSellOrders.find(order.stockId)->second).second;
        addPendingSellOrder(order);
        int qAft = (pendingSellOrders.find(order.stockId)->second).second;
        if (qAft == 0){
            pendingSellOrders.erase(order.stockId);
        }
        Order forHoldings(order.stockId,-1,order.price,qBef-qAft,order.type);
        removeHolding(forHoldings);
        money += abs(order.price*order.quantity);
    }
};