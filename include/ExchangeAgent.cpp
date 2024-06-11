#include "ExchangeAgent.h"

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

ExchangeAgent::ExchangeAgent(int id){
    stockId = id;
};

double baseFundamentalValue = startingPrice + ((rand()%2)*2-1)*5;
double midPrice = startingPrice;
double bidAskSpread = 0;
double lowestAsk = startingPrice;
double highestBid = startingPrice;

void setTradersList(multimap<int,Trader>& tradersList){
    tradersPtr = &tradersList;
};

void updatefundamentalValue(){
    if (baseFundamentalValue > 10){
        double change = ((rand()%2)*2-1)*(rand()%3);
        baseFundamentalValue += change;
    }
    else{
        double change = rand()%3;
        baseFundamentalValue += change;
    }
};

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
};

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