#include "OrderBook.h"


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
