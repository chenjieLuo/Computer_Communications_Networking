#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

using namespace std;

//  DESIGNED A DATA STRUCTURE CALLED NODE TO SAVE EACH ENTITY IN LRU CACHE.
struct Node{
    string key;
    string url;
    string expireat;
    string date;
    string last_modified;
    Node* next;
    Node(): key(""), url(""), expireat(""), date(""), last_modified(""), next(NULL) {}
    Node(string key, string url, string expireat, string date, string last_modified){
        this->key = key;
        this->url = url;
        this->next = NULL;
        this->expireat = expireat;
        this->date = date;
        this->last_modified = last_modified;
    }
};

//  LRU CACHE IS USED TO CACHE RECENT USED K ENTITIES. WHEN A REQUEST ARRIVES, WE FIRSTLY CHECK IF IT IS CONTAINED IN THE CACHE. IF NOT WE SEND REQUEST TO THE WEB SERVER
class LRUcache{
public:
    unordered_map<string, Node*> map;
    int size;
    int capacity;
    Node* header;
    Node* tail;
    LRUcache(int capacity) {
        this->header = new Node();
        this->tail = header;
        this->size = 0;
        this->capacity = capacity;
        map.clear();
    }
    // MOVE THE NODE TO THE END OF END OF THE QUEUE TO MAINTAIN LRU CACHE
    void movetoTail(Node* prev){
        if (prev->next == tail)
            return;
        Node* temp = prev->next;
        prev->next = temp->next;
        map[temp->next->key] = prev;
        map[temp->key] = tail;
        tail->next = temp;
        tail = tail->next;
    }
    Node* get(string key) {
        if (map.find(key) == map.end())
            return NULL;
        movetoTail(map[key]);
        return map[key]->next;
    }
    
    void push(string key, string url, string _expireat, string _date, string _last_modified) {
        if (map.find(key) != map.end()){
            map[key]->next->url = url;
            map[key]->next->expireat = _expireat;
            map[key]->next->date = _date;
            map[key]->next->last_modified = _last_modified;
            movetoTail(map[key]);
        }
        else{
            Node *temp = new Node(key, url, _expireat, _date, _last_modified);
            map[key] = tail;
            tail->next = temp;
            tail = tail->next;
            size += 1;
            if (size > capacity){
                Node* temp2 = header->next;
                map.erase(temp2->key);
                header->next = temp2->next;
                if (header->next != NULL)
                    map[temp2->next->key] = header;
                size -= 1;
            }
        }
    }
    void print_status(){
        cout << "Currently there are " << map.size() << " entities in our cache..." << endl;
        int cnt = 0;
        for (auto &x: map){
            cout << ++cnt << ": " << x.first << endl;
            cout << "Expires at: " << map[x.first]->next->expireat << endl;
            cout << "Last accessed: " << map[x.first]->next->date << endl;
            cout << "Last modified: " << map[x.first]->next->last_modified << endl;
        }
    }
};

int main(){
    LRUcache cache(10);
    return 0;
}
