#ifndef ALTCFG_H
#define ALTCFG_H

#include <llvm/IR/BasicBlock.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Debug.h>
#include <llvm/ADT/APInt.h>

#include <memory>
#include <map>
#include <set>
#include <cassert>

using namespace llvm;
using namespace std;


namespace altepp {

#define SRC(E) \
    (E.first)

#define TGT(E) \
    (E.second)

typedef pair<BasicBlock*, BasicBlock*> Edge;
typedef SmallVector<Edge, 32> EdgeListTy; 
typedef SetVector<BasicBlock*, vector<BasicBlock*>,
                  DenseSet<BasicBlock*>> SuccListTy;
typedef MapVector<const BasicBlock *, SuccListTy> CFGTy;
typedef MapVector<Edge, pair<Edge, Edge>> FakeTableTy;
typedef DenseMap<const BasicBlock*, SmallVector<BasicBlock*, 4>> SuccCacheTy;
typedef MapVector<Edge, APInt> EdgeWtMapTy;

class altcfg {
    EdgeListTy Edges;
    EdgeWtMapTy Weights;
    CFGTy CFG;
    SuccCacheTy SuccCache;
    EdgeListTy get() const;
    EdgeListTy getSpanningTree(BasicBlock *);
    void spanningHelper(BasicBlock*, EdgeListTy&, 
            DenseSet<BasicBlock*>&);
    EdgeListTy getChords(EdgeListTy&) const;
    void computeIncrement(EdgeWtMapTy&, BasicBlock*, BasicBlock*, 
            EdgeListTy&, EdgeListTy&);
  protected:
    FakeTableTy Fakes;
  public:
    EdgeWtMapTy getIncrements(BasicBlock*, BasicBlock*);
    bool add(BasicBlock* Src, BasicBlock* Tgt, 
                BasicBlock* Entry = nullptr, BasicBlock* Exit = nullptr);
    APInt& operator[](const Edge&);
    void initWt(const Edge, const APInt);
    void print(raw_ostream & os = errs()) const;
    void dot(raw_ostream& os = errs()) const;
    SmallVector<BasicBlock*, 4> succs(const BasicBlock*);
    void clear() { Edges.clear(),  Weights.clear(), 
        CFG.clear(); SuccCache.clear(); }
};

class CFGInstHelper : public altcfg {
    EdgeWtMapTy Inc;
  public:
    CFGInstHelper(altcfg& A, BasicBlock* B, BasicBlock* C) 
        : altcfg(A), Inc(A.getIncrements(B, C)) { }
    std::tuple<bool, APInt, bool, APInt> get(Edge E) const {
            APInt Val1(128, 0, true), Val2(128, 0, true);
            bool NeedToLog = false, IncExists = false;
            if(Fakes.count(E)) {
                Edge E1 = {nullptr, nullptr}, E2 = {nullptr, nullptr};
                tie(E1, E2) = Fakes.lookup(E); 
                Val1 = Inc.lookup(E1);
                Val2 = Inc.lookup(E2);
                NeedToLog = IncExists = true; 
            } else {
                // If we request an edge which is not found 
                // in the increment map, this line will segfault when it
                // tries to assign a default constructed APInt (width 1)
                // to a 128 bit value. Inc is only initialized to Chords
                // so check before lookup.
                if(Inc.count(E)) {
                    Val1 = Inc.lookup(E); 
                    IncExists = true; 
                }
            }   
            return {IncExists, Val1, NeedToLog, Val2};
        }
};

}
#endif
