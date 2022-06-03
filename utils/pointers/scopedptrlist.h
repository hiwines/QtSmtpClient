#ifndef SCOPEDPTRLIST_H
#define SCOPEDPTRLIST_H

#include <QList>
#include <QDataStream>
#include <utility>

/// Optimized Scoped List of Pointers
/// \todo Make const-iterator provide access to const T* instead of T* according to all other
///     public interface of the list (ex: at(..), operator[..], ..)
template <typename T>
class ScopedPtrList
{
// public definitions
public:
    /// Exports QList Types
    typedef typename QList<T*>::const_iterator const_iterator;
    typedef typename QList<T*>::iterator iterator;
    typedef typename QList<T*>::ConstIterator ConstIterator;
    typedef typename QList<T*>::Iterator Iterator;
    typedef typename QList<T*>::const_reverse_iterator const_reverse_iterator;
    typedef typename QList<T*>::reverse_iterator reverse_iterator;

// construction
public:
    /// Empty Construction
    inline ScopedPtrList() {}
    /// Construction from Initializer List
    inline ScopedPtrList(std::initializer_list<T*> args) : d(args) {}
    /// Move Constructor
    inline ScopedPtrList(ScopedPtrList &&other) : d(std::move(other.d)) {}
    /// Disable Standard Copy Constructor
    inline ScopedPtrList(const ScopedPtrList &other) = delete;
    /// Assignment Operator using Copy-And-Swap-Idiom
    ScopedPtrList& operator=(ScopedPtrList other) { swap(other); return *this; }
    /// Destruction
    inline ~ScopedPtrList() { clear(); }

// public interface
public:
    /// Reserve space for \a alloc elements
    inline void reserve(int alloc) { d.reserve(alloc); }
    /// Appends the given pointer (taking ownership) and return it
    T* append(T *elem) { d.append(elem); return elem; }
    /// Appends a new element and return it
    inline T* appendNew() { return append(new T()); }
    /// Prepend the given pointer (taking ownership) and return it
    T* prepend(T *elem) { d.prepend(elem); return elem; }
    /// Prepend a new element and return it
    inline T* prependNew() { return prepend(new T()); }
    /// Appends \a upTo N items from the \a other list into this one
    /// \warning Please note that items will be removed from the \a other list to be added into this one
    void appendFrom(ScopedPtrList<T> &other, int upTo = -1)
    {
        // call validation
        if (upTo == 0 || other.isEmpty() || this == &other)
            return;
        // checks whether we are appending the full other list
        if (upTo < 0 || upTo >= other.size()) {
            // checks for swap optimization
            if (isEmpty())
                return swap(other);
            // otherwise, just append all items to this list
            d.append(other.d);
            other.d.clear();
            return;
        }
        // worst case scenario as of performances, having to partially import other items
        // compute the amount of items to import
        auto toImportCount = std::min(other.size(), upTo);
        // ensure there is enough space into this list
        reserve(size() + toImportCount);
        // iterate to import them
        while (--toImportCount >= 0)
            d.append(other.d.takeFirst());
    }
    /// Prepends \a upTo N items from the \a other list into this one
    /// \warning Please note that items will be removed from the \a other list to be added into this one
    void prependFrom(ScopedPtrList<T> &other, int upTo = -1)
    {
        // call validation
        if (upTo == 0 || other.isEmpty() || this == &other)
            return;
        // checks whether we are appending the full other list
        if (upTo < 0 || upTo >= other.size()) {
            // checks for swap optimization
            if (isEmpty())
                return swap(other);
            // otherwise, append this list to the other and then swap
            // in order to have better performances (append is better than prepend)
            other.d.append(d);
            d.clear();
            d.swap(other.d);
            return;
        }
        // worst case scenario as of performances, having to partially import other items
        // compute the amount of items to import
        auto toImportCount = std::min(other.size(), upTo);
        // allocate a new list to translate prepend into appends
        QList<T*> newData;
        // reserve the required space to optimize growing strategy
        newData.reserve(size() + toImportCount);
        // append other elements
        while (--toImportCount >= 0)
            newData.append(other.d.takeFirst());
        // append this whole list and swap to commit changes
        newData.append(d);
        d.swap(newData);
    }
    /// Appends \a upTo N items from this list into the \a other one
    /// \warning Please note that items will be removed from this list to be added into the \a other one
    inline void appendTo(ScopedPtrList<T> &other, int upTo = -1)
        { other.appendFrom(*this, upTo); }
    /// Prepends \a upTo N items from this list into the \a other one
    /// \warning Please note that items will be removed from this list to be added into the \a other one
    inline void prependTo(ScopedPtrList<T> &other, int upTo = -1)
        { other.prependFrom(*this, upTo); }
    /// Inserts the given element (taking ownership) at given position and return it
    T* insert(int i, T *elem) { d.insert(i, elem); return elem; }
    /// Inserts a new element at given position and return it
    inline T* insertNew(int i) { return insert(i, new T()); }
    /// Gets the element at position i
    inline const T* at(int i) const { return d.at(i); }
    inline T* at(int i) { return d.at(i); }
    /// Clear the list freeing all resources
    void clear()
    {
        // free resources
        for (T *item : d) delete item;
        // clear dangling pointers
        d.clear();
    }
    /// Gets the first element
    inline const T* first() const { return d.first(); }
    inline T* first() { return d.first(); }
    /// Gets the last element
    inline const T* last() const { return d.last(); }
    inline T* last() { return d.last(); }
    /// Checks whether the element is contained into the list
    inline bool contains(T *elem) const { return d.contains(elem); }
    /// Gets the amount of internal elements
    inline int count() const { return d.count(); }
    inline int size() const { return d.size(); }
    inline int length() const { return d.length(); }
    /// Checks whether the list is empty
    inline bool isEmpty() const { return d.isEmpty(); }
    /// Take the item at the given position
    inline T* takeAt(int i) { return d.takeAt(i); }
    iterator takeAt(iterator pos, T *&elem)
    {
        // store the pointer to take
        elem = *pos;
        // erase the position
        return d.erase(pos);
    }
    /// Take the first element of the list
    inline T* takeFirst() { return d.takeFirst(); }
    inline T* takeLast() { return d.takeLast(); }
    /// Release the given element of the list, returning it
    /// \warning No check is performed over the effective existance of the elem into the list
    T* release(T *elem) {
        // remove the first occurrence of such element
        // (having multiple occurrences is not allowed cause it will surely end up in double deletion)
        d.removeOne(elem);
        // return such element
        return elem;
    }
    /// Move the element at index \a from placing it at index \a to
    inline void move(int from, int to) { d.move(from, to); }
    /// Swap element at index \a i with element at index \a j
    inline void swapItemsAt(int i, int j) { d.swapItemsAt(i, j); }
    /// Swap the content with the other list
    inline void swap(ScopedPtrList<T> &other) { d.swap(other.d); }
    /// Drop the element at the given position (freeing resources)
    inline void dropAt(int i) { delete takeAt(i); }
    iterator dropAt(iterator pos)
    {
        // free the item at given position
        delete *pos;
        // erasa such position
        return d.erase(pos);
    }
    /// Drops the first element of the list
    inline void dropFirst() { delete takeFirst(); }
    /// Drops the last element of the list
    inline void dropLast() { delete takeLast(); }
    /// Drops the given element of the list (freeing resources)
    inline void drop(T *elem) { delete release(elem); }
    /// Gets the position of the given element
    inline int indexOf(T *elem, int from = 0) const { return d.indexOf(elem, from); }
    /// Gets the element at the given pos, with fallback
    inline const T* value(int i, T *def = nullptr) const { return d.value(i, def); }
    inline T* value(int i, T *def = nullptr) { return d.value(i, def); }
    /// Gets the elem at the given position
    inline const T* operator[](int i) const { return d[i]; }
    inline T* operator[](int i) { return d[i]; }

    /// Standard Iterator API
    inline iterator begin() { return d.begin(); }
    inline const_iterator begin() const { return d.cbegin(); }
    inline const_iterator cbegin() const { return d.cbegin(); }
    inline iterator end() { return d.end(); }
    inline const_iterator end() const { return d.cend(); }
    inline const_iterator cend() const { return d.cend(); }
    inline reverse_iterator rbegin() { return d.rbegin(); }
    inline const_reverse_iterator crbegin() const { return d.crbegin(); }
    inline reverse_iterator rend() { return d.rend(); }
    inline const_reverse_iterator crend() const { return d.crend(); }

// private members
private:
    // Internal Container
    QList<T*> d;
};

/// Encodes the given list into a QDataStream (need operator<< for const T&)
template <typename T> QDataStream& operator<<(QDataStream &stream, const ScopedPtrList<T> &list)
{
    // ensure it is ok
    if (stream.status() != QDataStream::Ok)
        return stream;
    // write the size of the list
    stream << list.size();
    // iterate over all items of the list
    for (const auto *item : list) {
        // use a boolean to track the status of the pointer
        stream << (item != nullptr);
        // if the pointer is valid, encode the data itself
        if (item != nullptr)
            stream << *item;
        // ensure the status is ok to continue
        if (stream.status() != QDataStream::Ok)
            break;
    }
    // quit
    return stream;
}
/// Decodes a list from a QDataStream (need operator>> for std::unique_ptr<T>&)
template <typename T> QDataStream& operator>>(QDataStream &stream, ScopedPtrList<T> &list)
{
    // reset result container
    list.clear();
    // ensure the stream is ok
    if (stream.status() != QDataStream::Ok)
        return stream;
    // extract the size of the list
    // (please note that calling list.reserve(size) is not safe here cause if an error occurs
    //  into reading data, we could end up allocating lot of useless data just to fail at some point)
    int size = 0;
    stream >> size;
    // iterate to fetch all existing items
    for (int ix = 0; ix < size; ++ix) {
        // ensure the stream is ok
        if (stream.status() != QDataStream::Ok)
            return stream;
        // read the boolean status of the pointer
        bool isValidPtr = false;
        stream >> isValidPtr;
        // handles a valid pointer
        if (isValidPtr) {
            // allocate a new scoped pointer for the item
            std::unique_ptr<T> ptr;
            stream >> ptr;
            // ensure the stream is ok
            if (stream.status() != QDataStream::Ok)
                return stream;
            // ensure the item was read successfully
            if (ptr == nullptr) {
                // tracks an error
                stream.setStatus(QDataStream::ReadCorruptData);
                return stream;
            }
            // add the read item to the list
            list.append(ptr.release());

        // invalid pointer
        } else {
            // just append a null
            list.append(nullptr);
        }
    }
    // quit
    return stream;
}

#endif // SCOPEDPTRLIST_H
