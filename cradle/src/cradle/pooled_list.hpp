#ifndef CRADLE_POOLED_LIST_HPP
#define CRADLE_POOLED_LIST_HPP

#include <algorithm>
#include <list>
#include <cradle/common.hpp>

namespace cradle {

// A pooled_list is a list-like container that allocates its elements in
// fixed-size blocks. Since there is only one memory allocation per block
// (rather than one per element), it reduces the overhead associated with
// memory allocations. It should be used when you're creating large lists and
// you don't know how many elements will be in the list beforehand, but you
// want to avoid the inefficiencies associated with STL lists and vectors in
// that scenario.
//
// Note that a pooled_list only allows you to add elements to the end.
// You can't insert elements at other points, move elements, or remove
// elements. Also, it's currently limited to const access to the elements.
//
// T is the type of element, and BlockSize is the number of elements per block.
//
template<typename T, std::size_t BlockSize = 0x40>
class pooled_list : noncopyable
{
 public:
    static std::size_t const block_size = BlockSize;

    pooled_list()
      : next_allocation_(NULL)
      , current_block_end_(NULL)
    {}

    ~pooled_list()
    {
        for (typename std::list<T*>::iterator
            i = blocks_.begin(); i != blocks_.end(); ++i)
        {
            delete[] *i;
        }
    }

    T* alloc()
    {
        if (next_allocation_ == current_block_end_)
            create_new_block();
        return next_allocation_++;
    }

    class const_iterator
    {
     public:
        const_iterator() {}

        const_iterator(pooled_list const* p,
            typename std::list<T*>::const_iterator block)
          : list_(p)
          , block_(block)
        { update_pointers(); }

        bool operator==(const_iterator const& other) const
        {
            return this->block_ == other.block_ &&
                this->object_ == other.object_;
        }
        bool operator!=(const_iterator const& other) const
        {
            return !(*this == other);
        }

        const_iterator& operator++()
        {
            ++object_;
            if (object_ == block_end_)
            {
                ++block_;
                update_pointers();
            }
            return *this;
        }

        void update_pointers()
        {
            if (block_ != list_->blocks_.end())
            {
                object_ = *block_;
                typename std::list<T*>::const_iterator next_block = block_;
                ++next_block;
                if (next_block == list_->blocks_.end())
                    block_end_ = list_->next_allocation_;
                else
                    block_end_ = *block_ + block_size;
            }
            else
                object_ = NULL;
        }

        T const& operator*() const
        { return *object_; }
        T const* operator->() const
        { return object_; }

     private:
        pooled_list const* list_;
        typename std::list<T*>::const_iterator block_;
        T const* block_end_;
        T const* object_;
    };

    const_iterator begin() const
    { return const_iterator(this, blocks_.begin()); }
    const_iterator end() const
    { return const_iterator(this, blocks_.end()); }

    bool empty() const { return blocks_.empty(); }

    typedef std::size_t size_type;
    size_type size() const
    {
        return blocks_.empty() ? 0 :
            (blocks_.size() - 1) * block_size +
            (next_allocation_ - blocks_.back());
    }

    void swap(pooled_list& other)
    {
        std::swap(blocks_, other.blocks_);
        std::swap(next_allocation_, other.next_allocation_);
        std::swap(current_block_end_, other.current_block_end_);
    }

 private:
    void create_new_block()
    {
        blocks_.push_back(new T[block_size]);
        current_block_end_ = blocks_.back() + block_size;
        next_allocation_ = blocks_.back();
    }

    std::list<T*> blocks_;
    T* next_allocation_;
    T* current_block_end_;
};

template<typename T, std::size_t BlockSize>
void swap(pooled_list<T,BlockSize>& a, pooled_list<T,BlockSize>& b)
{ a.swap(b); }

}

#endif
