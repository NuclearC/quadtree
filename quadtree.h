// ================================= //
//                                   //
// QuadTree Version 0.1a             //
// Copyright (c)2017 NuclearC        //
//                                   //
// ================================= //

// quadtree.h: Header only implementation

#ifndef NC_QUADTREE_H_
#define NC_QUADTREE_H_

#include <vector>

namespace nc {
    template <typename T>
    struct QuadTreeAABB {
        // boundaries
        T left, top, right, bottom;
        // center
        T x, y;

        const T& get_width() const { return right - left; }
        const T& get_height() const { return bottom - top; }

        void set_center() {
            x = (left + right) / (T)2;
            y = (top + bottom) / (T)2;
        }

        bool verify() const {
            return ((left < right) && (top < bottom));
        }

        bool intersects(const QuadTreeAABB<T>& _Other) const {
            return (left < _Other.left + (_Other.right - _Other.left) &&
                left + (right - left) > _Other.left &&
                top < _Other.top + (_Other.bottom - _Other.top) &&
                (bottom - top) + top > _Other.top);
        }

        bool contains(T _X, T _Y) const {
            return ((_X > left) && (_X < right) && (_Y > top) && (_Y < bottom));
        }
    };

    template <typename T>
    struct QuadTreeObject {
        QuadTreeAABB<T> bounds;
        void* user_data;

        bool removed = true;

        // unique id used to remove the object later
        size_t id = 0;
    };

    template <typename T = double, size_t _Capacity = 2>
    class QuadTree {
    private:
        static constexpr size_t kChildren = 4;

        QuadTreeAABB<T> bounds;

        size_t object_count = 0;
        QuadTreeObject<T>* objects;
        QuadTree* children = nullptr;
        bool has_children = false;

        void split();
        void merge();

        void remove_empty_nodes();

        size_t level = 1;
    public:
        QuadTree() {
            objects = new QuadTreeObject<T>[_Capacity];
            has_children = false;

            for (size_t i = 0; i < _Capacity; i++)
                objects[i].removed = true;
        }

        QuadTree(const QuadTreeAABB<T>& _Bounds) : bounds(_Bounds) {
            objects = new QuadTreeObject<T>[_Capacity];
            has_children = false;
        }

        ~QuadTree() {
            delete[] objects;
        }

        void set_bounds(const QuadTreeAABB<T>& _Bounds) {
            bounds = _Bounds;
        }

        const QuadTreeAABB<T>& get_bounds() const {
            return bounds;
        }

        bool insert(const QuadTreeObject<T>& _Object);
        bool remove(const QuadTreeObject<T>& _Object);

        void query(const QuadTreeAABB<T>& _Boundaries, QuadTreeObject<T>* _Objects, size_t& _Length) const;

        bool has_children_() const { return has_children; }

        const QuadTree* get_children() const { return children; }

        size_t get_total_objects() const;
    };

    template<typename T, size_t _Capacity>
    inline void QuadTree<T, _Capacity>::split()
    {
        if (!has_children) {
            QuadTreeAABB<T> child_aabb;

            children = new QuadTree[kChildren];
            // top left
            child_aabb.left = bounds.left;
            child_aabb.top = bounds.top;
            child_aabb.right = bounds.x;
            child_aabb.bottom = bounds.y;
            child_aabb.set_center();
            children[0].set_bounds(child_aabb);
            // top right
            child_aabb.left = bounds.x;
            child_aabb.top = bounds.top;
            child_aabb.right = bounds.right;
            child_aabb.bottom = bounds.y;
            child_aabb.set_center();
            children[1].set_bounds(child_aabb);
            // bottom right
            child_aabb.left = bounds.x;
            child_aabb.top = bounds.y;
            child_aabb.right = bounds.right;
            child_aabb.bottom = bounds.bottom;
            child_aabb.set_center();
            children[2].set_bounds(child_aabb);
            // bottom left
            child_aabb.left = bounds.left;
            child_aabb.top = bounds.y;
            child_aabb.right = bounds.x;
            child_aabb.bottom = bounds.bottom;
            child_aabb.set_center();
            children[3].set_bounds(child_aabb);

            has_children = true;
        }
    }

    template<typename T, size_t _Capacity>
    inline void QuadTree<T, _Capacity>::merge()
    {
        if (has_children) {
            children[0].merge();
            children[1].merge();
            children[2].merge();
            children[3].merge();

            delete[] children;

            has_children = false;
        }
    }

    template<typename T, size_t _Capacity>
    inline void QuadTree<T, _Capacity>::remove_empty_nodes()
    {
        if (has_children) {
            children[0].remove_empty_nodes();
            children[1].remove_empty_nodes();
            children[2].remove_empty_nodes();
            children[3].remove_empty_nodes();

            if (get_total_objects() < 1)
                merge();
        }
    }

    template<typename T, size_t _Capacity>
    inline bool QuadTree<T, _Capacity>::insert(const QuadTreeObject<T>& _Object)
    {
        if (bounds.intersects(_Object.bounds)) {
            if (object_count >= _Capacity) {
                if (!has_children)
                    split();

                if (!children[0].insert(_Object))
                    if (!children[1].insert(_Object))
                        if (!children[2].insert(_Object))
                            if (!children[3].insert(_Object))
                                throw std::out_of_range("object position out of range");

                return true;
            }
            else {
                for (size_t i = 0; i < _Capacity; i++) {
                    if (objects[i].removed) {
                        objects[i].bounds = _Object.bounds;
                        objects[i].user_data = _Object.user_data;
                        objects[i].id = _Object.id;
                        objects[i].removed = false;

                        object_count++;
                        return true;
                    }
                }
            }
        }

        return false;
    }

    template<typename T, size_t _Capacity>
    inline bool QuadTree<T, _Capacity>::remove(const QuadTreeObject<T>& _Object)
    {
        if (bounds.intersects(_Object.bounds)) {
            if (object_count > 0) {
                for (size_t i = 0; i < _Capacity; i++) {
                    if (!objects[i].removed && objects[i].id == _Object.id) {
                        objects[i].removed = true;
                        object_count--;
                        remove_empty_nodes();

                        return true;
                    }
                }
            }
            
            if (has_children) {
                if (!children[0].remove(_Object))
                    if (!children[1].remove(_Object))
                        if (!children[2].remove(_Object))
                            if (!children[3].remove(_Object))
                                return false;

                return true;
            }

        }
        else {
            return false;
        }
    }

    template<typename T, size_t _Capacity>
    inline void QuadTree<T, _Capacity>::query(const QuadTreeAABB<T>& _Boundaries, QuadTreeObject<T>* _Objects, size_t& _Length) const
    {
        if (bounds.intersects(_Boundaries)) {
            if (has_children) {
                children[0].query(_Boundaries, _Objects, _Length);
                children[1].query(_Boundaries, _Objects, _Length);
                children[2].query(_Boundaries, _Objects, _Length);
                children[3].query(_Boundaries, _Objects, _Length);
            }

            if (object_count < 1)
                return;

            for (size_t i = 0; i < _Capacity; i++) {
                if (!objects[i].removed && objects[i].bounds.intersects(_Boundaries)) {
                    _Objects[_Length++] = objects[i];
                }
            }
        }
    }

    template<typename T, size_t _Capacity>
    inline size_t QuadTree<T, _Capacity>::get_total_objects() const
    {
        size_t obj_count = object_count;

        if (has_children) {
            obj_count += children[0].get_total_objects() +
                children[1].get_total_objects() +
                children[2].get_total_objects() +
                children[3].get_total_objects();
        }

        return obj_count;
    }
} // namespace nc

#endif // NC_QUADTREE_H_