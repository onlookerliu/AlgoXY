#include <iostream>
#include <map>
#include <functional>
#include <sstream>
#include <algorithm>
#include <list>
#include <iterator>

template<class Coll>
void print(const Coll& xs, std::string prefix=""){
  using namespace std;
  cout<<prefix<<": ";
  copy(xs.begin(), xs.end(), ostream_iterator<typename Coll::value_type>(cout, ", "));
  cout<<"\n";
}

// default mapping functor, map from search input to keys in Trie
template<class T>
struct map_identity: public std::unary_function<T, std::list<T> >{
  std::list<T> operator()(T x){
    return std::list<T>(1, x);
  }
};

// T9 mapping functor
struct t9_mapping: public std::unary_function<char, std::list<char> >{
  std::map<char, std::string> keypad;
  t9_mapping(){
    keypad['2']="abc";
    keypad['3']="def";
    keypad['4']="ghi";
    keypad['5']="jkl";
    keypad['6']="mno";
    keypad['7']="pqrs";
    keypad['8']="tuv";
    keypad['9']="wxyz";
  }

  std::list<char> operator()(char x){
    using namespace std;
    list<char> res;
    back_insert_iterator<list<char> > i(res);
    if(keypad.find(x)!=keypad.end())
      copy(keypad[x].begin(), keypad[x].end(), i);
    return res;
  }
};

// Definition
template<class T, class _MapFunc=map_identity<T> >
struct Trie{
  typedef _MapFunc MapFunc;
  typedef std::map<T, Trie<T, MapFunc>*> Children;

  Trie():count(0){}

  virtual ~Trie(){
    for(typename Children::iterator it=children.begin();
        it!=children.end(); ++it)
      delete it->second;
  }

  int count; //frequency.
  Children children;
};

// Example of how to add other augment data
template<class T, class U>
struct TrieDict: public Trie<T>{
  TrieDict(U x=U()): Trie<T>(), value(x){}
  U value; //explaination of a word etc.
};

// recursive insertion
template<class T, class MapFunc, class Coll>
Trie<T,MapFunc>* insert(Trie<T, MapFunc>* t, Coll value){
  if(!t)
    t=new Trie<T, MapFunc>;

  if(!value.empty()){
    typename Coll::iterator it=value.begin();
    t->children[*it]=insert(t->children[*it], 
                            Coll(++value.begin(), value.end())); 
  }
  return t;
}

// imperative insertion, with frequency
template<class T, class MapFunc, class Coll>
Trie<T, MapFunc>* trie_insert(Trie<T, MapFunc>* t, Coll value, int priority=1){
  if(!t)
    t= new Trie<T, MapFunc>;

  Trie<T, MapFunc>* p=t;
  for(typename Coll::iterator it=value.begin(); it!=value.end(); ++it){
    if(p->children.find(*it)==p->children.end())
      p->children[*it]=new Trie<T, MapFunc>;
    p=p->children[*it];
    p->count+=priority; // simple approach, disadvantage: sum of many low priority words > 1 high priority word
  }
  return t;
}

// helper functions to map appending element a list
// map (lambda e, es, -> (e+es)) xs
template<class T, class Coll>
Coll operator+(T x, Coll xs){
  if(xs.empty())
    xs.push_back(typename Coll::value_type(1, x));
  else
    for(typename Coll::iterator it=xs.begin(); it!=xs.end(); ++it){
      std::insert_iterator<typename Coll::value_type> i(*it, it->begin());
      *i++=x;
    }
  return xs;
}

// helper function to concat 2 collections together
// x = x + y; where + means append
template<class Coll>
Coll& operator+(Coll& x, Coll& y){
  std::back_insert_iterator<Coll> it(x);
  std::copy(y.begin(), y.end(), it);
  return x;
}

// heper function to prioritize the candidates
template<class Keys, class Map>
Keys prioritize(Keys keys, Map& ts){
  typedef std::multimap<int, typename Keys::value_type, std::greater<int> > Coll;
  Coll coll;
  for(typename Keys::iterator k=keys.begin(); k!=keys.end(); ++k)
    if(ts.find(*k)!=ts.end())
      coll.insert(std::make_pair(ts[*k]->count, *k));
  Keys res;
  for(typename Coll::iterator it=coll.begin(); it!=coll.end(); ++it)
    res.push_back(it->second);
  return res;
}

// recursive search
template<class T, class MapFunc, class Coll>
std::list<Coll> search(Trie<T, MapFunc>* t, Coll value){
  std::list<Coll> res;

  if(t->children.empty() || value.empty())
      return res;

  MapFunc f;
  typedef typename MapFunc::result_type Keys; 
  Keys keys=prioritize(f(*value.begin()), t->children);
  for(typename Keys::iterator k=keys.begin(); k!=keys.end(); ++k){
    std::list<Coll> xs=(*k)+search(t->children[*k], 
                                   Coll(++value.begin(), value.end()));
    res=res+xs;
  }
  return res;
}

// search all candidates with prefix as value
// Caution: if the trie is big, and the prefix 
// is very small, it may return a huge result.
template<class T, class MapFunc, class Coll>
std::list<Coll> search_all(Trie<T, MapFunc>* t, Coll prefix){
  std::list<Coll> res;
  
  if(t->children.empty()) return res;

  if(prefix.empty()){
    for(typename Trie<T, MapFunc>::Children::iterator it=t->children.begin(); 
        it!=t->children.end(); ++it){
      std::list<Coll> xs=(it->first)+search_all(it->second, Coll());
      res=res+xs;
    }
  }
  else{
    MapFunc f;
    typedef typename MapFunc::result_type Keys;
    Keys keys=f(*prefix.begin());
    for(typename Keys::iterator k=keys.begin(); k!=keys.end(); ++k){
      if(t->children.find(*k)!=t->children.end()){
        std::list<Coll> xs=(*k)+search_all(t->children[*k], 
                                       Coll(++prefix.begin(), prefix.end()));
        res=res+xs;
      }
    }
  }
}

// imperative search
template<class T, class MapFunc, class Coll>
std::list<Coll> trie_search(Trie<T, MapFunc>* t, Coll value){
  std::list<Coll> res;

  if(t->children.empty() || value.empty())
      return res;

  MapFunc f;
  typedef typename MapFunc::result_type Keys; 
  typedef std::list<std::pair<Coll, Trie<T, MapFunc>*> > Candidates;
  Candidates tries;
  tries.push_back(std::make_pair(Coll(), t));
  for(typename Coll::iterator it=value.begin(); it!=value.end(); ++it){
    Candidates nexts;
    for(typename Candidates::iterator tr=tries.begin(); tr!=tries.end(); ++tr){
      Keys keys=prioritize(f(*it), tr->second->children);
      for(typename Keys::iterator k=keys.begin(); k!=keys.end(); ++k){
        Coll prefix(tr->first);
        prefix.push_back(*k);
        nexts.push_back(std::make_pair(prefix, tr->second->children[*k]));
      }
    }
    tries=nexts;
  }

  for(typename Candidates::iterator tr=tries.begin(); tr!=tries.end(); ++tr)
    res.push_back(tr->first);
  return res;
}

// test helpers
template<class T, class MapFunc>
std::string trie_to_str(Trie<T, MapFunc>* t, std::string prefix=""){
  std::ostringstream s;
  s<<"("<<prefix;
  for(typename Trie<T, MapFunc>::Children::iterator it=t->children.begin();
      it!=t->children.end(); ++it)
    s<<", "<<trie_to_str(it->second, prefix+it->first);
  s<<")";
  return s.str();
}

class Test{
private:
  Trie<char>* t;
  Trie<char, t9_mapping>* t1;
  Trie<char, t9_mapping>* t2;

  void test_insert(){
    t=insert(t, std::string("a"));
    t=insert(t, std::string("b"));
    insert(t, std::string("good"));
    insert(t, std::string("home"));
    insert(t, std::string("gone"));
    std::cout<<"t=\n"<<trie_to_str(t)<<"\n";

    t1=insert(t1, std::string("good"));
    t1=insert(t1, std::string("home"));
    t1=insert(t1, std::string("gone"));
    t1=insert(t1, std::string("hood"));    
    t1=insert(t1, std::string("hello"));
    t1=insert(t1, std::string("a"));
    t1=insert(t1, std::string("an"));
    t1=insert(t1, std::string("the"));
    std::cout<<"t1=\n"<<trie_to_str(t1)<<"\n";

    // insert words with priority
    t2=trie_insert(t2, std::string("home"), 3);
    t2=trie_insert(t2, std::string("good"), 2);
    t2=trie_insert(t2, std::string("gone"));
    t2=trie_insert(t2, std::string("hood"));
    std::cout<<"t2=\n"<<trie_to_str(t2)<<"\n";
  }

  void test_search(){
    test_dict_search();
    test_t9_search();
    test_priority_search();
    test_search_all();
    test_imperative_search();
  }

  void test_dict_search(){
    std::cout<<"test dict search\n";
    std::list<std::string> res;
    res=search(t, std::string("a"));
    print(res, "search a");
    res=search(t, std::string("go"));
    print(res, "search go");
    res=search(t, std::string("bar"));
    print(res, "search bar");
    res=search(t, std::string("gone"));
    print(res, "search gone");
  }

  void test_t9_search(){
    std::cout<<"test t9 search\n";
    std::list<std::string> res;
    res=search(t1, std::string("46"));
    print(res, "t9 search 46");
    res=search(t1, std::string("4663"));
    print(res, "t9 search 4663");
    res=search(t1, std::string("2"));
    print(res, "t9 search 2");
  }

  void test_priority_search(){
    std::cout<<"test priority search\n";
    std::list<std::string> res;
    res=search(t2, std::string("4663"));
    print(res, "t9 search 4663 with priority");
  }

  void test_search_all(){
    std::cout<<"test serach all\n";
    std::list<std::string> res;
    res=search_all(t, std::string(""));
    print(res, "search all");
    res=search_all(t, std::string("go"));
    print(res, "search all start with go");
  }

  void test_imperative_search(){
    std::cout<<"test imperative search\n";
    std::list<std::string> res;
    res=trie_search(t1, std::string("46"));
    print(res, "t9 search 46");
    res=trie_search(t1, std::string("4663"));
    print(res, "t9 search 4663");
    res=trie_search(t1, std::string("2"));
    print(res, "t9 search 2");
  }

public:
  Test():t(0), t1(0), t2(0){ }

  ~Test(){
    delete t;
    delete t1;
    delete t2;
  }

  void run(){
    test_insert();
    test_search();
  }
};

int main(int, char**){
  Test().run();
}
