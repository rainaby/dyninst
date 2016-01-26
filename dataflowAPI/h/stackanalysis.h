/*
 * See the dyninst/COPYRIGHT file for copyright information.
 * 
 * We provide the Paradyn Tools (below described as "Paradyn")
 * on an AS IS basis, and do not warrant its validity or performance.
 * We reserve the right to update, modify, or discontinue this
 * software at any time.  We shall have no obligation to supply such
 * updates or modifications or any other form of support to you.
 * 
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#if !defined(STACK_ANALYSIS_H)
#define STACK_ANALYSIS_H

#if !defined(_MSC_VER) && !defined(os_freebsd)
#include <values.h>
#endif

#include "dyntypes.h"
#include <set>
#include <map>
#include <string>
#include <list>
#include "util.h"
#include "dyn_regs.h"
#include "Absloc.h"

// To define StackAST
#include "DynAST.h"

// FreeBSD is missing a MINLONG and MAXLONG
#if defined(os_freebsd) 
#if defined(arch_64bit)
#define MINLONG INT64_MIN
#define MAXLONG INT64_MAX
#else
#define MINLONG INT32_MIN
#define MAXLONG INT32_MAX
#endif
#endif

// These are _NOT_ in the Dyninst namespace...
namespace Dyninst {

  namespace ParseAPI {
    class Function;
    class Block;
	class Edge;
  };

  namespace InstructionAPI {
    class Instruction;
    class Expression;
  };

 
class StackAnalysis {
  typedef boost::shared_ptr<InstructionAPI::Instruction> InstructionPtr;
  typedef boost::shared_ptr<InstructionAPI::Expression> ExpressionPtr;

 public:

    class DATAFLOW_EXPORT Height {
    public:
        typedef signed long Height_t;
        
        static const Height_t uninitialized = MAXLONG;
        static const Height_t notUnique = MINLONG;
        
        static const Height bottom;
        static const Height top;
        
      Height(const Height_t h) : height_(h) {};
      Height() : height_(uninitialized) {};
        
        Height_t height() const { return height_; }
        
        // FIX THIS!!! if we stop using TOP == MAXINT and
        // BOT == MININT...
        bool operator<(const Height &rhs) const {
            return (height_ < rhs.height_);
        }

        bool operator>(const Height &rhs) const {
            return (height_ > rhs.height_);
        }

        bool operator<=(const Height &rhs) const {
            return (height_ <= rhs.height_);
        }

        bool operator>=(const Height &rhs) const {
            return (height_ >= rhs.height_);
        }
        
        Height &operator+= (const Height &other) {
            if (isBottom()) return *this;
            if (other.isBottom()) {
                *this = bottom;
                return *this;
            }
            if (isTop() && other.isTop()) {
                // TOP + TOP = TOP
                *this = top;
                return *this;
            }
            if (isTop() || other.isTop()) {
                // TOP + height = BOTTOM
                *this = bottom;
                return *this;
            }
            
            height_ += other.height_;
            return *this;
        }

        const Height operator+(const Height &rhs) const {
            if (isBottom()) return bottom;
            if (rhs.isBottom()) return rhs;
            if (isTop() && rhs.isTop()) return top;
            if (isTop() || rhs.isTop()) return bottom;

            return Height(height_ + rhs.height_);
        }

        const Height operator-(const Height &rhs) const {
            if (isBottom()) return bottom;
            if (rhs.isBottom()) return rhs;
            if (isTop() && rhs.isTop()) return top;
            if (isTop() || rhs.isTop()) return bottom;

            return Height(height_ - rhs.height_);
        }

        Height &operator+=(const signed long &rhs) {
            if (isBottom()) return *this;
            if (isTop()) return *this;

            height_ += rhs;
            return *this;
        }

	const Height operator+(const signed long &rhs) const {
	   if (isBottom()) return bottom;
           if (isTop()) return top;

	   return Height(height_ + rhs);
	}

        bool operator==(const Height &rhs) const {
           return height_ == rhs.height_;
        }
        bool operator!=(const Height &rhs) const {
            return !(*this == rhs);
        }
        
        std::string format() const {
            if (isTop()) return "TOP";
            if (isBottom()) return "BOTTOM";
            
            std::stringstream retVal;
            retVal << height_;
            return retVal.str();
        }                                        
        
        bool isBottom() const {
            return height_ == notUnique;
        }
        
        bool isTop() const {
            return height_ == uninitialized; 
        }

        static Height meet(const Height &lhs, const Height &rhs) {
           if (rhs == lhs) return rhs;
           if (rhs == top) return lhs;
           if (lhs == top) return rhs;
           return bottom;
        }

        static Height meet (std::set<Height> &ins) {
           if (ins.empty()) {
              return top;
           }
           
           // If there is a single element in the set return it.
           if (ins.size() == 1) {
              return *(ins.begin());
           }
           
           // MEET (bottom, ...) == bottom
           // Since ins is sorted, if bottom is in the set it must
           // be the start element...
           
           if ((*(ins.begin())) == bottom)  {
              return bottom;
           }
           
           // MEET (N, top) == N
           if ((ins.size() == 2) &&
               ((*(ins.rbegin())) == top)) {
              return *(ins.begin());
           }
           
           // There are 2 or more elements; the last one is not
           // top; therefore there is a conflict, and we return
           // bottom.
           return bottom;
        }
        
    private:
        Height_t height_;
    };


    // We need to represent the effects of instructions. We do this
    // in terms of transfer functions. We recognize the following 
    // effects on the stack.
    //
    // Offset by known amount: push/pop/etc. 
    // Set to known value: leave
    // Assign the stack pointer to/from an alias
    //
    // There are also: 
    // Offset by unknown amount expressible in a range [l, h]
    // Set to unknown value expressible in a range [l, h]
    // which we don't handle yet. 
    //
    // This gives us the following transfer functions. A transfer function
    // is a function T : (RegisterVector, RegisterID, RegisterID, value) -> (RegisterVector)
    //
    // Delta(RV, f, t, v) -> RV[f] += v;
    // Abs(RV, f, t, v) -> RV[f] = v;
    // Alias(RV, f, t, v) -> RV[t] = RV[f];
    // In the implementations below, we provide f, t, v at construction time (as they are
    // fixed) and RV as a parameter.

    
    typedef std::map<Absloc, Height> AbslocState;

    struct TransferFunc {
       static const long uninitialized = MAXLONG;
       static const long notUnique = MINLONG;

       static const TransferFunc top;
       static const TransferFunc bottom;

       typedef enum {
          Bottom,
          Delta,
          Abs,
          Alias } Type;
       
       static TransferFunc deltaFunc(Absloc r, long d);
       static TransferFunc absFunc(Absloc r, long a, bool i = false);
       static TransferFunc aliasFunc(Absloc f, Absloc t, bool i = false);
       static TransferFunc bottomFunc(Absloc r);
       static TransferFunc retopFunc(Absloc r);
       static TransferFunc sibFunc(std::map<Absloc, std::pair<long,bool> > f,
          long d, Absloc t);

       bool isBottom() const;
       bool isTop() const;
       bool isRetop() const;
       bool isAbs() const;
       bool isAlias() const;
       bool isDelta() const;
       bool isSIB() const;

       bool isTopBottom() const {
           return topBottom;
       }

    TransferFunc() :
       from(Absloc()), target(Absloc()), delta(0), abs(uninitialized),
       retop(false), topBottom(false) {};
    TransferFunc(long a, long d, Absloc f, Absloc t, bool i = false,
       bool rt = false) :
       from(f), target(t), delta(d), abs(a), retop(rt), topBottom(i) {};
    TransferFunc(std::map<Absloc,std::pair<long,bool> > f, long d, Absloc t) :
       from(Absloc()), target(t),
       delta(d), abs(uninitialized),
       retop(false),
       topBottom(false),
       fromRegs(f) {}

       Height apply(const AbslocState &inputs) const;
       void accumulate(std::map<Absloc, TransferFunc> &inputs);

       std::string format() const;
       Type type() const;

       Absloc from;
       Absloc target;
       long delta;
       long abs;

       // Distinguish between default-constructed transfer functions and
       // explicitly-retopped transfer functions.
       bool retop;

       // Annotate transfer functions that have the following characteristic:
       // if target is TOP, keep as TOP
       // else, target must be set to BOTTOM
       // E.g., sign-extending a register:
       //   if the register had an uninitialized stack height (TOP),
       //       the sign-extension has no effect
       //   if the register had a valid or notunique (BOTTOM) stack height,
       //       the sign-extension must result in a BOTTOM stack height
       bool topBottom;

       // Handle complex math from SIB functions
       std::map<Absloc, std::pair<long, bool> > fromRegs;
    };

    typedef std::list<TransferFunc> TransferFuncs;
    typedef std::map<Absloc, TransferFunc> TransferSet;

    // Summarize the effects of a series (list!) of transfer functions.
    // Intended to summarize a block. We may want to do a better job of
    // summarizing, but this works...
    struct SummaryFunc {
       static const long uninitialized = MAXLONG;
       static const long notUnique = MINLONG;

       SummaryFunc() {};

       void apply(const AbslocState &in, AbslocState &out) const;
       std::string format() const;
	   void validate() const;

       void add(TransferFuncs &f);

       TransferSet accumFuncs;
    };

    // The results of the stack analysis is a series of 
    // intervals. For each interval we have the following
    // information: 
    //   a) Whether the function has a well-defined
    //      stack frame. This is defined as follows:
    //        x86/AMD-64: a frame pointer
    //        POWER: an allocated frame pointed to by GPR1
    //   b) The "depth" of the stack; the distance between
    //      the stack pointer and the caller's stack pointer.
    //   c) The "depth" of any aliases of the stack pointer. 
    
    typedef std::map<Offset, AbslocState> StateIntervals;
    typedef std::map<ParseAPI::Block *, StateIntervals> Intervals;
    
    typedef std::map<ParseAPI::Function *, Height> FuncCleanAmounts;
    
    typedef std::map<ParseAPI::Block *, SummaryFunc> BlockEffects;
    typedef std::map<ParseAPI::Block *, AbslocState> BlockState;

    // To build intervals, we must replay the effect of each instruction.
    // To avoid sucking enormous time, we keep those transfer functions around...
    typedef std::map<ParseAPI::Block *, std::map<Offset, TransferFuncs> > InstructionEffects;

    DATAFLOW_EXPORT StackAnalysis();
    DATAFLOW_EXPORT StackAnalysis(ParseAPI::Function *f);
    
    DATAFLOW_EXPORT Height find(ParseAPI::Block *, Address addr, Absloc loc);
    // And a commonly used shortcut
    DATAFLOW_EXPORT Height findSP(ParseAPI::Block *, Address addr);
    DATAFLOW_EXPORT Height findFP(ParseAPI::Block *, Address addr);
    DATAFLOW_EXPORT void findDefinedHeights(ParseAPI::Block* b, Address addr, std::vector<std::pair<Absloc, Height> >& heights);
    
    
    DATAFLOW_EXPORT void debug();
    
 private:
    
    std::string format(const AbslocState &input) const;

    MachRegister sp();
    MachRegister fp();

    bool analyze();
    void summarizeBlocks();
    void summarize();

    void fixpoint();
    
    void createIntervals();

    void createEntryInput(AbslocState &input);
    void meetInputs(ParseAPI::Block *b, AbslocState& blockInput, AbslocState &input);
    void meet(const AbslocState &source, AbslocState &accum);
    AbslocState getSrcOutputLocs(ParseAPI::Edge* e);
    void computeInsnEffects(ParseAPI::Block *block,
                            InstructionPtr insn,
                            const Offset off,
                            TransferFuncs &xferFunc);

    bool isCall(InstructionPtr insn);
    bool handleNormalCall(InstructionPtr insn, ParseAPI::Block *block, Offset off, TransferFuncs &xferFuncs);
    bool handleThunkCall(InstructionPtr insn, TransferFuncs &xferFuncs);
    void handlePushPop(InstructionPtr insn, int sign, TransferFuncs &xferFuncs);
    void handleReturn(InstructionPtr insn, TransferFuncs &xferFuncs);
    void handleAddSub(InstructionPtr insn, ParseAPI::Block *block,
        const Offset off, int sign, TransferFuncs &xferFuncs);
    void handleLEA(InstructionPtr insn, TransferFuncs &xferFuncs);
    void handleLeave(TransferFuncs &xferFuncs);
    void handlePushPopFlags(int sign, TransferFuncs &xferFuncs);
    void handlePushPopRegs(int sign, TransferFuncs &xferFuncs);
    void handlePowerAddSub(InstructionPtr insn, int sign, TransferFuncs &xferFuncs);
    void handlePowerStoreUpdate(InstructionPtr insn, TransferFuncs &xferFuncs);
    void handleMov(InstructionPtr insn, ParseAPI::Block *block,
        const Offset off, TransferFuncs &xferFuncs);
    void handleZeroExtend(InstructionPtr insn, TransferFuncs &xferFuncs);
    void handleSignExtend(InstructionPtr insn, TransferFuncs &xferFuncs);
    void handleXor(InstructionPtr insn, TransferFuncs &xferFuncs);
    void handleDiv(InstructionPtr insn, TransferFuncs &xferFuncs);
    void handleMul(InstructionPtr insn, TransferFuncs &xferFuncs);
    void handleDefault(InstructionPtr insn, TransferFuncs &xferFuncs);

    long extractDelta(InstructionAPI::Result deltaRes);

    Height getStackCleanAmount(ParseAPI::Function *func);

    ParseAPI::Function *func;

    

    // SP effect tracking
    BlockEffects blockEffects;
    InstructionEffects insnEffects;

    BlockState blockInputs;
    BlockState blockOutputs;



    Intervals *intervals_; // Pointer so we can make it an annotation
    
    FuncCleanAmounts funcCleanAmounts;
    int word_size;
    ExpressionPtr theStackPtr;
    ExpressionPtr thePC;

};

};

DATAFLOW_EXPORT std::ostream &operator<<(std::ostream &os, const Dyninst::StackAnalysis::Height &h);

namespace Dyninst {
  DEF_AST_LEAF_TYPE(StackAST, Dyninst::StackAnalysis::Height);

};


#endif

