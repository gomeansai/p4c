/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _IR_VISITOR_H_
#define _IR_VISITOR_H_

#include <stdexcept>
#include <unordered_map>
#include "lib/cstring.h"
#include "ir/ir.h"
#include "lib/exceptions.h"
#include "lib/castable.h"

// declare this outside of Visitor so it can be forward declared in node.h
struct Visitor_Context {
    // We maintain a linked list of Context structures on the stack
    // in the Visitor::apply_visitor functions as we do the recursive
    // descent traversal.  pre/postorder function can access this
    // context via getContext/findContext
    const Visitor_Context       *parent;
    const IR::Node              *node, *original;
    mutable int                 child_index;
    mutable const char          *child_name;
    int                         depth;
};

class Visitor : public ICastable {
 public:
    typedef Visitor_Context Context;
    class profile_t {
        // for profiling -- a profile_t object is created when a pass
        // starts and destroyed when it ends.  Moveable but not copyable.
        Visitor         &v;
        uint64_t        start;
        explicit profile_t(Visitor &);
        profile_t() = delete;
        profile_t(const profile_t &) = delete;
        profile_t &operator=(const profile_t &) = delete;
        profile_t &operator= (profile_t &&) = delete;
        friend class Visitor;
     public:
        ~profile_t();
        profile_t(profile_t &&);
    };
    virtual ~Visitor() = default;

    mutable cstring internalName;
    // Some visitors are created and applied by other visitors.
    // This field keeps track of the caller.
    const Visitor* called_by = nullptr;

    const Visitor& setCalledBy(const Visitor* visitor) { called_by = visitor; return *this; }
    // init_apply is called (once) when apply is called on an IR tree
    // it expects to allocate a profile record which will be destroyed
    // when the traversal completes.  Visitor subclasses may extend this
    // to do any additional initialization they need.  They should call their
    // parent's init_apply to do further initialization
    virtual profile_t init_apply(const IR::Node *root);
    profile_t init_apply(const IR::Node *root, const Context *parent_context);
    // End_apply is called symmetrically with init_apply, after the visit
    // is completed.  Both functions will be called in the event of a normal
    // completion, but only the 0-argument version will be called in the event
    // of an exception, as there is no root in that case.
    virtual void end_apply();
    virtual void end_apply(const IR::Node* root);

    // apply_visitor is the main traversal function that manages the
    // depth-first recursive traversal.  `visit` is a convenience function
    // that calls it on a variable.
    virtual const IR::Node *apply_visitor(const IR::Node *n, const char *name = 0) = 0;
    void visit(const IR::Node *&n, const char *name = 0) { n = apply_visitor(n, name); }
    void visit(const IR::Node *const &n, const char *name = 0) {
        auto t = apply_visitor(n, name);
        if (t != n) visitor_const_error(); }
    void visit(const IR::Node *&n, const char *name, int cidx) {
        ctxt->child_index = cidx;
        n = apply_visitor(n, name); }
    void visit(const IR::Node *const &n, const char *name, int cidx) {
        ctxt->child_index = cidx;
        auto t = apply_visitor(n, name);
        if (t != n) visitor_const_error(); }
    void visit(IR::Node *&, const char * = 0, int = 0) { BUG("Can't visit non-const pointer"); }
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                            \
    void visit(const IR::CLASS *&n, const char *name = 0);              \
    void visit(const IR::CLASS *const &n, const char *name = 0);        \
    void visit(const IR::CLASS *&n, const char *name, int cidx);        \
    void visit(const IR::CLASS *const &n, const char *name , int cidx); \
    void visit(IR::CLASS *&, const char * = 0, int = 0) { BUG("Can't visit non-const pointer"); }
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
    void visit(IR::Node &n, const char *name = 0) {
        if (name && ctxt) ctxt->child_name = name;
        n.visit_children(*this); }
    void visit(const IR::Node &n, const char *name = 0) {
        if (name && ctxt) ctxt->child_name = name;
        n.visit_children(*this); }
    void visit(IR::Node &n, const char *name, int cidx) {
        if (ctxt) {
            ctxt->child_name = name;
            ctxt->child_index = cidx; }
        n.visit_children(*this); }
    void visit(const IR::Node &n, const char *name, int cidx) {
        if (ctxt) {
            ctxt->child_name = name;
            ctxt->child_index = cidx; }
        n.visit_children(*this); }
    template<class T> void parallel_visit(IR::Vector<T> &v, const char *name = 0) {
        if (name && ctxt) ctxt->child_name = name;
        v.parallel_visit_children(*this); }
    template<class T> void parallel_visit(const IR::Vector<T> &v, const char *name = 0) {
        if (name && ctxt) ctxt->child_name = name;
        v.parallel_visit_children(*this); }
    template<class T> void parallel_visit(IR::Vector<T> &v, const char *name, int cidx) {
        if (ctxt) {
            ctxt->child_name = name;
            ctxt->child_index = cidx; }
        v.parallel_visit_children(*this); }
    template<class T> void parallel_visit(const IR::Vector<T> &v, const char *name, int cidx) {
        if (ctxt) {
            ctxt->child_name = name;
            ctxt->child_index = cidx; }
        v.parallel_visit_children(*this); }

    virtual Visitor *clone() const { BUG("need %s::clone method",  name()); return nullptr; }
    virtual bool check_clone(const Visitor *a) { return typeid(*this) == typeid(*a); }

    // Functions for IR visit_children to call for ControlFlowVisitors.
    virtual Visitor &flow_clone() { return *this; }

    /** Merge the given visitor into this visitor at a joint point in the
     * control flow graph.  Should update @this and leave the other unchanged.
     */
    virtual void flow_merge(Visitor &) { }
    /** Support methods for non-local ControlFlow computations */
    virtual void flow_merge_global_to(cstring) { }
    virtual void flow_merge_global_from(cstring) { }
    virtual void erase_global(cstring) { }
    virtual bool check_global(cstring) { return false; }
    virtual void clear_globals() { }

    static cstring demangle(const char *);
    virtual const char *name() const {
        if (!internalName)
            internalName = demangle(typeid(*this).name());
        return internalName.c_str();
    }
    void setName(const char* name) { internalName = name; }
    void print_context() const;  // for debugging; can be called from debugger

    // Context access/search functions.  getContext returns the context
    // that refers to the immediate parent of the node currently being
    // visited.  findContext searches up the context for a (grand)parent
    // of a specific type.  Orig versions return the original node (before
    // any cloning or modifications)
    const IR::Node* getOriginal() const { return ctxt->original; }
    template <class T>
    const T* getOriginal() const {
        CHECK_NULL(ctxt->original);
        BUG_CHECK(ctxt->original->is<T>(), "%1% does not have the expected type %2%",
                  ctxt->original, demangle(typeid(T).name()));
        return ctxt->original->to<T>();
    }
    const Context *getChildContext() const { return ctxt; }
    const Context *getContext() const { return ctxt->parent; }
    template <class T>
    const T* getParent() const {
        return ctxt->parent ? ctxt->parent->node->to<T>() : nullptr; }
    int getChildrenVisited() const { return ctxt->child_index; }
    int getContextDepth() const { return ctxt->depth - 1; }
    template <class T> inline const T *findContext(const Context *&c) const {
        if (!c) c = ctxt;
        while ((c = c->parent))
            if (auto *rv = dynamic_cast<const T *>(c->node)) return rv;
        return nullptr; }
    template <class T> inline const T *findContext() const {
        const Context *c = ctxt;
        return findContext<T>(c); }
    template <class T> inline const T *findOrigCtxt(const Context *&c) const {
        if (!c) c = ctxt;
        while ((c = c->parent))
            if (auto *rv = dynamic_cast<const T *>(c->original)) return rv;
        return nullptr; }
    template <class T> inline const T *findOrigCtxt() const {
        const Context *c = ctxt;
        return findOrigCtxt<T>(c); }
    inline bool isInContext(const IR::Node *n) const {
        for (auto *c = ctxt; c; c = c->parent) {
            if (c->node == n || c->original == n) return true; }
        return false; }

    /// @return the current node - i.e., the node that was passed to preorder()
    /// or postorder(). For Modifiers and Transforms, this is a clone of the
    /// node returned by getOriginal().
    const IR::Node* getCurrentNode() const { return ctxt->node; }
    template <class T>
    const T* getCurrentNode() const {
        return ctxt->node ? ctxt->node->to<T>() : nullptr; }

    /// True if the warning with this kind is enabled at this point.
    /// Warnings can be disabled by using the @noWarn("unused") annotation
    /// in an enclosing environment.
    bool warning_enabled(int warning_kind) const {
        return warning_enabled(this, warning_kind);
    }
    /// Static version of the above function, which can be called
    /// even if not directly in a visitor
    static bool warning_enabled(const Visitor* visitor, int warning_kind);
    template<class T,
             typename = typename std::enable_if<
                 std::is_base_of<Util::IHasSourceInfo, T>::value>::type,
             class... Args>
    void warn(const int kind, const char *format, const T *node, Args... args) {
        if (warning_enabled(kind))
            ::warning(kind, format, node, std::forward<Args>(args)...);
    }

    /// The const ref variant of the above
    template<class T,
             typename = typename std::enable_if<
                 std::is_base_of<Util::IHasSourceInfo, T>::value>::type,
             class... Args>
    void warn(const int kind, const char *format, const T &node, Args... args) {
        if (warning_enabled(kind))
            ::warning(kind, format, node, std::forward<Args>(args)...);
    }

 protected:
    // if visitDagOnce is set to 'false' (usually in the derived Visitor
    // class constructor), nodes that appear multiple times in the tree
    // will be visited repeatedly.  If this is done in a Modifier or Transform
    // pass, this will result in them being duplicated if they are modified.
    bool visitDagOnce = true;
    bool dontForwardChildrenBeforePreorder = false;
    // if joinFlows is 'true', Visitor will track nodes with more than one parent and
    // flow_merge the visitor from all the parents before visiting the node and its
    // children.  This only works for Inspector (not Modifier/Transform) currently.
    bool joinFlows = false;

    virtual void init_join_flows(const IR::Node *) { assert(0); }

    /** If @n is a join point in the control flow graph (i.e. has multiple incoming
     * edges) and is not filtered out by `filter_join_point`, then:
     *
     *   - if this is the first time a visitor has visited @n, store a clone of the
     *   visitor with this node and return true, deferring visiting this node until
     *   all incoming edges have been visited.
     *
     *   - if this is NOT the first visitor, but also not the last, then merge this
     *   visitor into the stored visitor clone, and return true.
     *
     *   - finally, if this is the is the final visitor, merge the stored, cloned
     *   visitor---which has accumulated all previous visitors---with this one, and
     *   return false.
     *
     * `join_flows(n)` is invoked in `apply_visitor(n, name)`, and @n is only
     * visited if this method returns false.
     *
     * @return false if all upstream nodes from @n have been visited, and it's time
     * to visit @n.
     */
    virtual bool join_flows(const IR::Node *) { return false; }

    void visit_children(const IR::Node *, std::function<void()> fn) { fn(); }
    class ChangeTracker;  // used by Modifier and Transform -- private to them
    // This overrides visitDagOnce for a single node -- can only be called from
    // preorder and postorder functions
    void visitOnce() const { *visitCurrentOnce = true; }
    void visitAgain() const { *visitCurrentOnce = false; }

 private:
    virtual void visitor_const_error();
    const Context *ctxt = nullptr;  // should be readonly to subclasses
    bool *visitCurrentOnce = nullptr;
    friend class Inspector;
    friend class Modifier;
    friend class Transform;
    friend class ControlFlowVisitor;
};

class Modifier : public virtual Visitor {
    std::shared_ptr<ChangeTracker> visited;
    void visitor_const_error() override;
    bool check_clone(const Visitor *) override;
 public:
    profile_t init_apply(const IR::Node *root) override;
    const IR::Node *apply_visitor(const IR::Node *n, const char *name = 0) override;
    virtual bool preorder(IR::Node *) { return true; }
    virtual void postorder(IR::Node *) {}
    virtual void revisit(const IR::Node *, const IR::Node *) {}
    virtual void loop_revisit(const IR::Node *) { BUG("IR loop detected"); }
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                            \
    virtual bool preorder(IR::CLASS *);                                 \
    virtual void postorder(IR::CLASS *);                                \
    virtual void revisit(const IR::CLASS *, const IR::CLASS *);         \
    virtual void loop_revisit(const IR::CLASS *);
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
    void revisit_visited();
    bool visit_in_progress(const IR::Node *) const;
};

class Inspector : public virtual Visitor {
    struct info_t { bool done, visitOnce; };
    typedef std::unordered_map<const IR::Node *, info_t>       visited_t;
    std::shared_ptr<visited_t> visited;
    bool check_clone(const Visitor *) override;
 public:
    profile_t init_apply(const IR::Node *root) override;
    const IR::Node *apply_visitor(const IR::Node *, const char *name = 0) override;
    virtual bool preorder(const IR::Node *) { return true; }  // return 'false' to prune
    virtual void postorder(const IR::Node *) {}
    virtual void revisit(const IR::Node *) {}
    virtual void loop_revisit(const IR::Node *) { BUG("IR loop detected"); }
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                            \
    virtual bool preorder(const IR::CLASS *);                           \
    virtual void postorder(const IR::CLASS *);                          \
    virtual void revisit(const IR::CLASS *);                            \
    virtual void loop_revisit(const IR::CLASS *);
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
    void revisit_visited();
    bool visit_in_progress(const IR::Node *n) const {
        if (visited->count(n)) return !visited->at(n).done;
        return false; }
};

class Transform : public virtual Visitor {
    std::shared_ptr<ChangeTracker> visited;
    bool prune_flag = false;
    void visitor_const_error() override;
    bool check_clone(const Visitor *) override;

 public:
    profile_t init_apply(const IR::Node *root) override;
    const IR::Node *apply_visitor(const IR::Node *, const char *name = 0) override;
    virtual const IR::Node *preorder(IR::Node *n) {return n;}
    virtual const IR::Node *postorder(IR::Node *n) {return n;}
    virtual void revisit(const IR::Node *, const IR::Node *) {}
    virtual void loop_revisit(const IR::Node *) { BUG("IR loop detected"); }
#define DECLARE_VISIT_FUNCTIONS(CLASS, BASE)                            \
    virtual const IR::Node *preorder(IR::CLASS *);                      \
    virtual const IR::Node *postorder(IR::CLASS *);                     \
    virtual void revisit(const IR::CLASS *, const IR::Node *);          \
    virtual void loop_revisit(const IR::CLASS *);
    IRNODE_ALL_SUBCLASSES(DECLARE_VISIT_FUNCTIONS)
#undef DECLARE_VISIT_FUNCTIONS
    void revisit_visited();
    bool visit_in_progress(const IR::Node *) const;
    // can only be called usefully from a 'preorder' function (directly or indirectly)
    void prune() { prune_flag = true; }

 protected:
    const IR::Node *transform_child(const IR::Node *child) {
        auto *rv = apply_visitor(child);
        prune_flag = true;
        return rv; }
};

class ControlFlowVisitor : public virtual Visitor {
    std::map<cstring, ControlFlowVisitor &>     &globals;

 protected:
    ControlFlowVisitor* clone() const override = 0;
    typedef std::map<const IR::Node *, std::pair<ControlFlowVisitor *, int>> flow_join_points_t;
    flow_join_points_t *flow_join_points = 0;
    class SetupJoinPoints : public Inspector {
     protected:
        flow_join_points_t &join_points;
        void revisit(const IR::Node *n) override { ++join_points[n].second; }
     public:
        explicit SetupJoinPoints(flow_join_points_t &fjp) : join_points(fjp) { }
    };

    void init_join_flows(const IR::Node *root) override;
    bool join_flows(const IR::Node *n) override;

    /** This will be called for all nodes with multiple incoming edges, and
     * should return 'false' if the node should be considered a join point, and
     * 'true' if if should not be considered one. Nodes with only one incoming
     * edge are never join points.
     */
    virtual bool filter_join_point(const IR::Node *) { return false; }
    ControlFlowVisitor &flow_clone() override;
    ControlFlowVisitor() : globals(*new std::map<cstring, ControlFlowVisitor &>) {}

 public:
    void flow_merge_global_to(cstring key) override {
        if (globals.count(key))
            globals.at(key).flow_merge(*this);
        else
            globals.emplace(key, flow_clone()); }
    void flow_merge_global_from(cstring key) override {
        if (globals.count(key)) flow_merge(globals.at(key)); }
    void erase_global(cstring key) override { globals.erase(key); }
    bool check_global(cstring key) override { return globals.count(key) != 0; }
    void clear_globals() override { globals.clear(); }

    /// RAII class to ensure global key is only used in one place
    class GuardGlobal {
        Visitor &self;
        cstring key;

     public:
        GuardGlobal(Visitor &self, cstring key) : self(self), key(key) {
            BUG_CHECK(!self.check_global(key), "ControlFlowVisitor global %s in use", key); }
        ~GuardGlobal() { self.erase_global(key); }
    };
};

class Backtrack : public virtual Visitor {
 public:
    struct trigger {
        enum type_t { OK, OTHER }       type;
        explicit trigger(type_t t) : type(t) {}
        virtual ~trigger();
        virtual void dbprint(std::ostream &out) const { out << demangle(typeid(*this).name()); }

     protected:
        // must call this from the constructor if a trigger subclass contains pointers
        // or references to GC objects
        void register_for_gc(size_t);
    };
    virtual bool backtrack(trigger &trig) = 0;
    virtual bool never_backtracks() { return false; }  // generally not overridden
        // returns true for passes that will never catch a trigger (backtrack() is always false)
};

class P4WriteContext : public virtual Visitor {
 public:
    bool isWrite(bool root_value = false);     // might write based on context
    bool isRead(bool root_value = false);      // might read based on context
    // note that the context might (conservatively) return true for BOTH isWrite and isRead,
    // as it might be an 'inout' access or it might be unable to decide.
};


/**
 * Invoke an inspector @function for every node of type @NodeType in the subtree
 * rooted at @root. The behavior is the same as a postorder Inspector.
 */
template <typename NodeType, typename Func>
void forAllMatching(const IR::Node* root, Func&& function) {
    struct NodeVisitor : public Inspector {
        explicit NodeVisitor(Func&& function) : function(function) { }
        Func function;
        void postorder(const NodeType* node) override { function(node); }
    };
    root->apply(NodeVisitor(std::forward<Func>(function)));
}

/**
 * Invoke a modifier @function for every node of type @NodeType in the subtree
 * rooted at @root. The behavior is the same as a postorder Modifier.
 *
 * @return the root of the new, modified version of the subtree.
 */
template <typename NodeType, typename RootType, typename Func>
const RootType* modifyAllMatching(const RootType* root, Func&& function) {
    struct NodeVisitor : public Modifier {
        explicit NodeVisitor(Func&& function) : function(function) { }
        Func function;
        void postorder(NodeType* node) override { function(node); }
    };
    return root->apply(NodeVisitor(std::forward<Func>(function)));
}

/**
 * Invoke a transform @function for every node of type @NodeType in the subtree
 * rooted at @root. The behavior is the same as a postorder Transform.
 *
 * @return the root of the new, transformed version of the subtree.
 */
template <typename NodeType, typename Func>
const IR::Node* transformAllMatching(const IR::Node* root, Func&& function) {
    struct NodeVisitor : public Transform {
        explicit NodeVisitor(Func&& function) : function(function) { }
        Func function;
        const IR::Node* postorder(NodeType* node) override {
            return function(node);
        }
    };
    return root->apply(NodeVisitor(std::forward<Func>(function)));
}

#endif /* _IR_VISITOR_H_ */
