#include "../kalshi/KalshiWsClient.hpp"
#include "../infra/RingBuffer.hpp"


#include <iostream>
#include <string>
#include <thread>
#include <format>
#include <atomic>

void testwebsocket(KalshiWsClient& ws_client){
    RingBuffer ring(1024, 1024);

    auto printOutputs = [](const std::string& msg){
        std::cout<<msg<<"\n";
    };

    std::atomic<bool> done = false;

    int num_messages_received = 0;
    int num_messages_outputted = 0;
    auto writeToRingBuffer = [&ring, &num_messages_received](std::string& msg){
        num_messages_received++;
        ring.TryWrite(msg);
    };

    auto consume = [&ring, &num_messages_outputted, &done](){
        while (ring.TryRead() != nullptr || !done.load(std::memory_order_acquire)){
            if (ring.TryRead() != nullptr){
                std::cout<< *ring.TryRead()<<"\n";
                num_messages_outputted++;
                ring.FinishRead();
            }
        }
    };

    ws_client.start(writeToRingBuffer, "KXHIGHNY-26JUN30-B89.5");
    

    std::thread consumer_thread = std::thread(consume);
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout<<"Stopping...\n";
    
    ws_client.stop();
    done.store(true, std::memory_order_release);
    consumer_thread.join();

    std::cout <<"Done\n";
    std::cout << std::format("Received: {}; Outputted: {}\n", num_messages_received, num_messages_outputted);
}