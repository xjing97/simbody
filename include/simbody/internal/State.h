#ifndef SimTK_STATE_H_
#define SimTK_STATE_H_

/* Copyright (c) 2005-6 Stanford University and Michael Sherman.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "SimTKcommon.h"
#include "simbody/internal/common.h"

namespace SimTK {

class SimTK_SIMBODY_API DiscreteVariable {
public:
    DiscreteVariable() : rep(0) { }
    DiscreteVariable(const DiscreteVariable&);
    DiscreteVariable& operator=(const DiscreteVariable&);
    ~DiscreteVariable();

    // This takes ownership of the AbstractValue pointer.
    DiscreteVariable(Stage, AbstractValue* vp);

    Stage getStage() const;
    const AbstractValue& getValue() const;
    AbstractValue&       updValue();

private:
    class DiscreteVariableRep* rep;
};

class SimTK_SIMBODY_API CacheEntry : public DiscreteVariable {
public:
    CacheEntry() : DiscreteVariable() { }

    // This takes ownership of the AbstractValue pointer.
    CacheEntry(Stage g, AbstractValue* vp)
        : DiscreteVariable(g,vp) { }

    CacheEntry(const CacheEntry& ce) : DiscreteVariable(ce) { }
    CacheEntry& operator=(const CacheEntry& ce) {
        DiscreteVariable::operator=(ce);
        return *this;
    }
};

/**
 * This is the handle class for the hidden State implementation.
 * The default constructor creates a State containing no state variables
 * and with its realization cache stage set to Stage::Allocated.
 * During Subsystem construction, variables and cache entries for any
 * stage can be allocated, however *all* Modeled stage variables
 * must be allocated during this time. At the end of construction,
 * call advanceToStage(Built) which will put the State at Stage::Built.
 * Then the Subsystems realize their Modeled stages, during which 
 * variables at any stage > Modeled, and cache entries at any stage
 * >= Modeled can be allocated. After that call advanceToStage(Modeled) which
 * sets the stage to Stage::Modeled and disallows further allocation.
 *
 * Note that there is a global Stage for the state as a whole, and individual
 * Stages for each subsystem. The global stage can never be higher than
 * the lowest subsystem stage. Global resources are allocated when the
 * global Stage advances to "Modeled" and tossed out if that stage is
 * invalidated.
 */
class SimTK_SIMBODY_API State {
public:
    /// Create an empty State with one registered subsystem.
    State();
    /// Create an empty State with the indicated name and version for
    /// the 0th subsystem, which belongs to the System.
    State(const String& name, const String& version);
    ~State();
    State(const State&);
    State& operator=(const State&);

    /// Register a new subsystem as a client of this State. The
    /// supplied strings are stored with the State but are not
    /// interpreted by it. The intent is that they can be used to
    /// perform "santity checks" on deserialized States to make
    /// sure they match the currently instatiated System.
    /// The subsystem index is returned. It will always be 
    /// greater than zero since the 0th subsystem is reserved for
    /// private State entries owned by the System itself.
    int addSubsystem(const String& name, const String& version);

    int getNSubsystems() const;
    const String& getSubsystemName(int subsys) const;
    const String& getSubsystemVersion(int subsys) const;

    const Stage& getSubsystemStage(int subsys) const;
    const Stage& getSystemStage() const;

    // If any subsystem or the system stage is currently at or
    // higher than the passed-in one, back up to the stage just prior.
    // Otherwise do nothing.
    void invalidateAll(Stage) const;  // cache is mutable

    // Advance the current stage by one to the indicated stage.
    // The stage is passed in just to give us a chance to verify
    // that all is as expected. You can only advance one stage at
    // a time. Advancing to "Built" and "Modeled" stages affect
    // what you can do later.
    void advanceSubsystemToStage(int subsys, Stage) const;
    void advanceSystemToStage(Stage g) const;

    int allocateQ(int subsys, const Vector& qInit); // qdot, qdotdot also allocated in cache
    int allocateU(int subsys, const Vector& uInit); // udot                    "
    int allocateZ(int subsys, const Vector& zInit); // zdot                    "
    int allocateDiscreteVariable(int subsys, Stage, AbstractValue* v);
    int allocateCacheEntry(int subsys, Stage, AbstractValue* v);

    // You can call these as long as stage >= Modeled.
    const Real&   getTime() const;
    const Vector& getQ() const;
    const Vector& getU() const;
    const Vector& getZ() const;

    // You can call these as long as stage >= Modeled, but the
    // stage will be backed up if necessary to the indicated stage.
    Real&   updTime();  // Stage::Timed-1
    Vector& updQ();     // Stage::Configured-1
    Vector& updU();     // Stage::Moving-1
    Vector& updZ();     // Stage::Dynamics-1

    const Vector& getQDot() const; // Stage::Moving
    const Vector& getZDot() const; // Stage::Dynamics
    const Vector& getUDot() const; // Stage::Reacting
    const Vector& getQDotDot() const; // Stage::Reacting

    // These are mutable
    Vector& updQDot() const;    // Stage::Moving-1
    Vector& updZDot() const;    // Stage::Dynamics-1
    Vector& updUDot() const;    // Stage::Reacting-1
    Vector& updQDotDot() const; // Stage::Reacting-1

    // OK if dv.stage==Modeled or stage >= Modeled
    const AbstractValue& getDiscreteVariable(int subsys, int index) const;

    // OK if dv.stage==Modeled or stage >= Modeled; set stage to dv.stage-1
    AbstractValue&       updDiscreteVariable(int subsys, int index);

    // Stage >= ce.stage
    const AbstractValue& getCacheEntry(int subsys, int index) const;

    // Stage >= ce.stage-1; does not change stage
    AbstractValue& updCacheEntry(int subsys, int index) const; // mutable

    String toString() const;
    String cacheToString() const;

// ignore everything below here, please.
    class StateRep* rep;
    const StateRep& getRep() const {assert(rep); return *rep;}
    StateRep&       updRep()       {assert(rep); return *rep;}
};

inline std::ostream& 
operator<<(std::ostream& o, const State& s) {
    o << "STATE:" << std::endl;
    o << s.toString() << std::endl;
    o << "CACHE:" << std::endl;
    return o << s.cacheToString() << std::endl;
}


} // namespace SimTK

#endif // SimTK_STATE_H_
