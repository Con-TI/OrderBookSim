#include "include/ExchangeAgent.h"
#include "include/Trader.h"

using namespace std;

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