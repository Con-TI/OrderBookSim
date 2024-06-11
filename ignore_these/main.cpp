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
#include "include/color.cpp"
#include <random>

using namespace std;

enum class OrderType { BUY , SELL };
enum class FillType { FILLED , PARTIAL };
enum class TraderStatus { ORDERING , IDLE };

class OrderBook; class Trader; class ExchangeAgent;

multimap<int, int> priceBands = {{0, 50}, {1, 200}, {2, 500}, {3, 2000}, {4, 5000}};
multimap<int, int> priceIncrements = {{0, 1}, {1, 2}, {2, 5}, {3, 10}, {4, 25}};

int checkPriceBand(double price) {
    if (price > 5000) return 4;
    if (price > 2000) return 3;
    if (price > 500) return 2;
    if (price > 200) return 1;
    return 0;
}

struct Order {
    int stockId;
    int traderId;
    double price;
    int quantity;
    OrderType type;

    Order(int sI,int tI, double p, double q, OrderType t) : stockId(sI), traderId(tI), price(p), quantity(q), type(t) {}
};

class OrderBook{
private:
    multimap<double, multimap<int,vector<int>>> buyOrders;
    multimap<double, multimap<int,vector<int>>> sellOrders;

    vector<double> findIntersectingPrices(){
        set<double> buyPrices, sellPrices;
        for (const auto& elem : buyOrders) buyPrices.insert(elem.first);
        for (const auto& elem : sellOrders) sellPrices.insert(elem.first);

        vector<double> overlappingPrices;
        set_intersection(buyPrices.begin(),buyPrices.end(),sellPrices.begin(),sellPrices.end(),back_inserter(overlappingPrices));
        return overlappingPrices;
    };

public:

    void addOrder(const Order& order, const int& time){
        vector<int> vec = {order.quantity, order.traderId};
        if (order.type == OrderType::BUY){
            auto i = buyOrders.find(order.price);
            if (i!=buyOrders.end()){
                multimap<int,vector<int>> timeList = i -> second;
                timeList.insert(make_pair(time, vec));
                i -> second = timeList;
            }
            else{
                multimap<int,vector<int>> timeList;
                timeList.insert(make_pair(time, vec));
                buyOrders.insert(make_pair(order.price,timeList));
            }
        }
        else{
            auto i = sellOrders.find(order.price);
            if (i!=sellOrders.end()){
                multimap<int,vector<int>> timeList = i -> second;
                timeList.insert(make_pair(time, vec));
                i -> second = timeList;
            }
            else{
                multimap<int,vector<int>> timeList;
                timeList.insert(make_pair(time, vec));
                sellOrders.insert(make_pair(order.price,timeList));
            }
        }
    };

    multimap<FillType,pair<double,multimap<OrderType,vector<int>>>> matchOrders(){
        multimap<FillType,pair<double,multimap<OrderType,vector<int>>>> response;
        int buyTime; int buyerID; int sellTime; int sellerID; int quantityBuy; int quantitySell;
        multimap<OrderType,vector<int>> filledOrders; vector<int> filledOrder;
        multimap<OrderType,vector<int>> partialOrders; vector<int> partialOrder;
    
        vector<double> overlappingPrices = findIntersectingPrices();
        for (const double& price: overlappingPrices){
            int q;
            auto i = buyOrders.find(price); auto j = sellOrders.find(price);
            multimap<int,vector<int>> buyTimeList = i -> second; multimap<int,vector<int>> sellTimeList = i -> second;
            
            for (auto k = buyTimeList.begin(); k  != buyTimeList.end(); ++k){
                vector<int> bvec = k -> second;
                quantityBuy = bvec[0]; buyerID = bvec[1]; buyTime = k->first;

                for (auto w = sellTimeList.begin(); w != sellTimeList.end(); ++w){
                    vector<int> svec = w -> second;
                    quantitySell = svec[0]; sellerID = svec[1]; sellTime = w->first;

                    q = quantitySell - quantityBuy;
                    if (q<0){
                        sellTimeList.erase(w);
                        filledOrder = {sellTime, sellerID, quantitySell};
                        filledOrders.insert(make_pair(OrderType::SELL,filledOrder));

                        quantityBuy = abs(q);
                    }
                    else if(q>0){
                        buyTimeList.erase(k);
                        filledOrder = {buyTime, buyerID, quantityBuy};
                        filledOrders.insert(make_pair(OrderType::BUY,filledOrder));

                        quantitySell = q;
                        svec = {quantitySell,sellerID};
                        w -> second = svec;
                        partialOrder = {sellTime, sellerID, quantitySell};
                        partialOrders.insert(make_pair(OrderType::SELL,partialOrder));
                        break;
                    }
                    else{
                        buyTimeList.erase(k);
                        filledOrder = {buyTime, buyerID, quantityBuy};
                        filledOrders.insert(make_pair(OrderType::BUY,filledOrder));

                        sellTimeList.erase(w);
                        filledOrder = {sellTime, sellerID, quantitySell};
                        filledOrders.insert(make_pair(OrderType::SELL,filledOrder));
                        break;
                    }
                }
                if (q<0){
                    quantityBuy = abs(q);
                    bvec = {quantityBuy,buyerID};
                    k -> second = bvec;
                    partialOrder = {buyTime, buyerID, quantityBuy};
                    partialOrders.insert(make_pair(OrderType::BUY,partialOrder));
                    break;
                }

            }
            if (buyTimeList.empty()){
                buyOrders.erase(i);
            }
            else{
                i->second = buyTimeList;
            }
            if (sellTimeList.empty()){
                sellOrders.erase(j);
            }
            else{
                j->second = sellTimeList;
            }
            response.insert(make_pair(FillType::FILLED,make_pair(price,filledOrders)));
            response.insert(make_pair(FillType::PARTIAL,make_pair(price,partialOrders)));
        }
        return response;
    };

    multimap<OrderType,pair<double,int>> compressBook(){
        multimap<OrderType,pair<double,int>> neatBook;
        pair<double,int> priceQuantitypair;
        for (auto i = buyOrders.begin(); i!=buyOrders.end();++i){
            double priceBuy = i->first;
            int quantityBuy = 0;
            multimap<int,vector<int>> buyTimeList;
            for (auto j = buyTimeList.begin(); j!= buyTimeList.end();++j){
                vector<int> bvec = j->second;
                int qB = bvec[0]; 
                quantityBuy += qB;
            }
            priceQuantitypair = {priceBuy,quantityBuy};
            neatBook.insert(make_pair(OrderType::BUY,priceQuantitypair));
        }
        for (auto i = sellOrders.begin(); i!=sellOrders.end();++i){
            double priceSell = i->first;
            int quantitySell = 0;
            multimap<int,vector<int>> sellTimeList;
            for (auto j = sellTimeList.begin(); j!= sellTimeList.end();++j){
                vector<int> svec = j->second;
                int qS = svec[0]; 
                quantitySell += qS;
            }
            priceQuantitypair = {priceSell,quantitySell};
            neatBook.insert(make_pair(OrderType::SELL,priceQuantitypair));
        }
        return neatBook;
    };

    void resetBook(){
        buyOrders.clear();
        sellOrders.clear();
    };

};

class ExchangeAgent{
private:
    OrderBook Book;
    int stockId;
    multimap<OrderType,pair<double,int>> neatBook;
    int startingPriceBand = rand()%4;
    int minPrice = 25 + (priceBands.find(startingPriceBand) -> second);
    int maxPrice = priceBands.find(startingPriceBand+1) -> second;
    int increment = priceIncrements.find(startingPriceBand) -> second;
    double startingPrice = minPrice + (rand() % ((maxPrice-minPrice)/increment))*increment;
    int shares = rand()%4990000 + 10000;
    multimap<int,Trader>* tradersPtr;

public:
    ExchangeAgent(int id){
        stockId = id;
    }

    double baseFundamentalValue = startingPrice + ((rand()%2)*2-1)*5;
    double midPrice = startingPrice;
    double bidAskSpread = 0;
    double lowestAsk = startingPrice;
    double highestBid = startingPrice;

    void setTradersList(multimap<int,Trader>& tradersList){
        tradersPtr = &tradersList;
    }

    void updatefundamentalValue(){
        if (baseFundamentalValue > 10){
            double change = ((rand()%2)*2-1)*(rand()%3);
            baseFundamentalValue += change;
        }
        else{
            double change = rand()%3;
            baseFundamentalValue += change;
        }
    }

    void initialDistributionOfStocks(multimap<int,Trader>& traders){
        int totalNum = 1000;
        for (int i = 0; i!=9; ++i){
            totalNum+=1000*(rand()%2);
        }
        int storeNum = totalNum;
        int relative;
        while (storeNum!=0){
            for (auto i = traders.begin();i!=traders.end();++i){
                Trader& trader = i->second;
                int divisor = rand()%10 + 4;
                relative = storeNum%divisor;
                storeNum -= relative;
                int traderShare= shares * (relative/totalNum);
                Order order(stockId,-1,startingPrice,traderShare,OrderType::BUY);
                trader.addHolding(order);
                i->second = trader;
            }
        }
    };

    void traderToExchangeLimit(const Order& order, const int& time){
        Book.addOrder(order, time);
    };

    void endDay(){
        Book.resetBook();
    }

    void compileBook(){
        multimap<int,Trader> & traders = *tradersPtr;
        multimap<FillType,pair<double,multimap<OrderType,vector<int>>>> response = Book.matchOrders();
        neatBook = Book.compressBook();
        for (auto i = response.begin();i!=response.end();++i){
            FillType filltype = i->first;
            double price = (i->second).first;
            multimap<OrderType,vector<int>> orders = (i->second).second;
            for (auto j = orders.begin();j!=orders.end();++j){
                OrderType type = j->first;
                vector<int> vec = j->second;
                int traderId = vec[1];
                int quantity = vec[2];
                Trader& trader = traders.find(traderId) -> second;
                Order order(stockId,-1,price,-quantity,type);
                trader.receiveResponse(order);
            }
        }
    };

    void displayBook(){
        system("cls");
        cout<<endl;
        cout<<endl;
        cout<<"Stock ID: "<<stockId<<endl;
        cout<<endl;
        cout<<" |     Quantity     |    "<<dye::red("Bid")<<"    |    "<<dye::green("Ask")<<"    |     Quantity     | "<<endl;
        cout<<" --------------------------------------------------------------- "<<endl;

        multimap<double,int, greater<double>> Bids;
        multimap<double,int> Asks;
        auto buy_range = neatBook.equal_range(OrderType::BUY);
        auto sell_range = neatBook.equal_range(OrderType::SELL);
        for (auto i = buy_range.first; i!=buy_range.second; ++i){
            Bids.insert(i -> second);
        }
        for (auto i = sell_range.first; i!=sell_range.second; ++i){
            Asks.insert(i -> second);
        }

        auto bidI = Bids.begin();
        auto askI = Asks.begin();
        int even;
        int space;
        while (bidI != Bids.end() || askI != Asks.end()){
            if (bidI != Bids.end()){
                cout<< " |";
                int qDigits = floor(log10(bidI->second)+1);
                even = qDigits %2 ;
                if (even == 0){
                    space = (18-qDigits)/2;
                }
                else{
                    space = (18-1-qDigits)/2;
                }
                cout<<setw(space)<<bidI -> second<<setw(18-qDigits-space)<<"|";

                int pDigits = floor(log10(bidI->first)+1);
                even = pDigits % 2;
                if (even == 0){
                    space = (11-1-pDigits)/2;
                }
                else{
                    space = (11-pDigits)/2;
                }
                cout<<setw(space)<<dye::red(bidI -> first)<<setw(11-pDigits-space);

                ++bidI;
            }
            else{
                cout<<" |";
                cout<<setw(18)<<"|"<<setw(11);
                ++bidI;
            }

            cout<<"|";

            if (askI != Asks.end()){
                int pDigits = floor(log10(askI->first)+1);
                even = pDigits % 2;
                if (even == 0){
                    space = (11-1-pDigits)/2;
                }
                else{
                    space = (11-pDigits)/2;
                }
                cout<<setw(space)<<dye::green(askI -> first)<<setw(11-pDigits-space)<<"|";

                int qDigits = floor(log10(askI->second)+1);
                even = qDigits %2 ;
                if (even == 0){
                    space = (18-qDigits)/2;
                }
                else{
                    space = (18-1-qDigits)/2;
                }
                cout<<setw(space)<<askI -> second<<setw(18-qDigits-space);
                cout<<"| "<<endl;
                ++askI;
            }
            else{
                cout<<setw(11)<<"|"<<setw(18);
                cout<<"| "<<endl;
                ++askI;
            }
        }
        cout<<" |"<<setw(18)<<"|"<<setw(11)<<"|"<<setw(11)<<"|"<<setw(18)<<"| "<<endl;
        cout<<endl<<endl;

        bidI = Bids.begin();
        askI = Asks.begin();
        lowestAsk = askI->first;
        highestBid = bidI->first;
        double midprice = round((((highestBid) + (lowestAsk)))/2*100)/100;
        double BAspread = round((((lowestAsk)-(highestBid))/(highestBid)*100)*100)/100;
        midPrice = midprice;
        bidAskSpread = BAspread;
        cout<<"Highest Bid: "<<dye::red(highestBid)<<endl;
        cout<<"Lowest Ask: "<<dye::green(lowestAsk)<<endl;
        cout<<"Mid Price: "<<midprice<<endl;
        cout<<"Bid Ask Spread: "<<BAspread<<endl;
    };
};

class Trader{
private:
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
    }

    void resetSentiments(){
        for (auto i=sentiments.begin();i!=sentiments.end();++i){
            i->second = 0;
        }
    }

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

public:
    Trader(const int &id, multimap<int,ExchangeAgent>& exchangeAgentList){
        traderId = id;
        stocksPtr = &exchangeAgentList;
        for (auto i=exchangeAgentList.begin();i!=exchangeAgentList.end();++i){
            int stockId = i->first;
            sentiments.insert(make_pair(stockId,0));
        }
    }

    void updateTimer(const vector<int> & stockIds,const int & time){
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
    }

    void updateSentiments(const int & time){
        if (time==0){
            resetSentiments();
        } 
        else{
            for (auto i=sentiments.begin();i!=sentiments.end();++i){
                i->second += rand()%3;
            }
        }
    }

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
    }

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
    }
};

int main(){
    int time = 0;
    int day = 0;
    int dayLength = 100;
    int dayLimit = 1;

    int num_stocks;
    cout << "How many stocks? ";
    cin >> num_stocks;
    int num_traders;
    cout<< "How many traders? ";
    cin>> num_traders;
    cout<<endl;

    vector<int> stockIds;
    vector<int> traderIds;
    multimap<int,ExchangeAgent> stocks;
    multimap<int,Trader> traders;
    for (int i =0; i!=num_stocks; ++i){
        int stockId = i+1;
        ExchangeAgent exchange(stockId);
        stocks.insert(make_pair(stockId,exchange));
        stockIds.push_back(stockId);
    }
    for(int i=0; i!=num_traders; ++i){
        int traderId = i+1;
        Trader trader(traderId, stocks);
        traders.insert(make_pair(traderId,trader));
        traderIds.push_back(traderId);
    }
    for (auto i = stocks.begin();i!=stocks.end();++i){
        ExchangeAgent exchange = i->second;
        exchange.setTradersList(traders);
        i->second = exchange;
    }

    for (auto i = stocks.begin();i!=stocks.end();++i){
        ExchangeAgent exchange = i->second;
        exchange.initialDistributionOfStocks(traders);
    }


    do{
        for (auto i = stocks.begin();i!=stocks.end();++i){
            ExchangeAgent& exchange = i->second;
            exchange.updatefundamentalValue();
        }
        for (auto i = traders.begin();i!=traders.end();++i){
            Trader& trader = i->second;
            trader.updateSentiments(time);
            trader.updatePriceMemory();
            trader.updateTimer(stockIds,time);
        }
        for (auto i = stocks.begin(); i!=stocks.end();++i){
            ExchangeAgent& exchange = i->second;
            exchange.compileBook();
            exchange.displayBook();
        }

        time+=1;
        if (time==dayLength){
            time = 0;
            day += 1;
            for (auto i = stocks.begin();i!=stocks.end();++i){
                ExchangeAgent& exchange = i->second;
                exchange.endDay();
            }
            for (auto i = traders.begin();i!=traders.end();++i){
                Trader& trader = i->second;
                trader.updateSentiments(time);
            }
        }
    } while (day!=dayLimit);


}