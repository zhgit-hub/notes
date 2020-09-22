#include <bits/stdc++.h>
using namespace std;

class base_cnt{
public:
    base_cnt() : share_cnt(0),
                    weak_cnt(0){
        ;
    }
    int share_cnt;
    int weak_cnt;
};


template<class T>
class weak;

template<class T>
class shared{
public:
    shared(T *arg = NULL) : pointer(arg),
                            cnt(NULL){
        if(!arg){
            return;
        }
        cnt = new base_cnt();
        cnt->share_cnt = 1;
    }
    ~shared(){
        relase();
    }
    shared(const shared<T> &arg){
        pointer = arg.pointer;
        cnt = arg.cnt;
        if(pointer && cnt){
            cnt->share_cnt++;
        }
    }
    shared &operator=(const shared<T> &arg){
        relase();
        pointer = arg.pointer;
        cnt = arg.cnt;
        if(pointer && cnt){
            cnt->share_cnt++;
        }
        return *this;
    }
    int use_cnt(){
        if(cnt){
            cout<<"shared_cnt: "<<cnt->share_cnt<<endl;
            return cnt->share_cnt;
        }
        cout<<"shared_cnt: 0"<<endl;
        return 0;
    }
    friend class weak<T>;
private:
    void relase(){
        if(!cnt){
            return;
        }
        cnt->share_cnt--;
        if(cnt->share_cnt < 1){
            delete pointer;
            pointer = NULL;
            if(cnt->weak_cnt < 1){
                delete cnt;
                cnt = NULL;
            }
        }
    }

    T *pointer; 
    base_cnt *cnt;
};


template<class T>
class weak{
public:
    weak(const shared<T> &arg) : pointer(arg.pointer),
                            cnt(NULL){
        if(pointer){
            cnt = arg.cnt;
            cnt->weak_cnt += 1;
        }
    }
    ~weak(){
        relase();
    }
    weak(const weak<T> &arg) : pointer(arg.pointer),
                                cnt(NULL){
        if(pointer){
            cnt = arg.cnt;
            cnt->weak_cnt += 1;
        }
    }
    weak &operator=(const weak<T> &arg){
        relase();
        pointer = arg.pointer;
        cnt = arg.cnt;
        if(cnt){
            cnt->weak_cnt++;
        }
        return *this;
    }

    int use_cnt(){
        if(cnt){
            cout<<"shared_cnt :"<<cnt->share_cnt<<" "<<"weak_cnt :"<<cnt->weak_cnt<<endl;
            return  cnt->share_cnt;
        }
        cout<<"shared_cnt : 0 weak_cnt : 0"<<endl;
        return 0;
    }
private:
    void relase(){
        if(!cnt){
            return;
        }
        cnt->weak_cnt--;
        if(cnt->share_cnt < 1 && cnt->weak_cnt < 1){
            delete cnt;
        }
    }

    T *pointer;
    base_cnt *cnt;
};


