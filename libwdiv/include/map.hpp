#pragma once
#include "config.hpp"

template <typename K, typename V, typename Hasher, typename Eq>
struct HashMap
{
  enum State : uint8
  {
    EMPTY,
    FILLED,
    TOMBSTONE
  };

  struct Entry
  {
    K key;
    V value;
    size_t hash;
    State state;
  };

  Entry *entries = nullptr;
  size_t capacity = 0;
  size_t count = 0;
  size_t tombstones = 0;

  static constexpr float MAX_LOAD = 0.75f;

  HashMap() {}

  ~HashMap() { destroy(); }

  HashMap(const HashMap &) = delete;
  HashMap &operator=(const HashMap &) = delete;

  void destroy()
  {
    if (!entries)
      return;
    aFree(entries);
    entries = nullptr;
    capacity = count = tombstones = 0;
  }

  size_t mask() const { return capacity - 1; }

  Entry *findSlot(const K &key, size_t hash)
  {
    size_t index = hash & mask();
    Entry *tomb = nullptr;

    for (;;)
    {
      Entry *e = &entries[index];

      if (e->state == EMPTY)
      {
        return tomb ? tomb : e;
      }

      if (e->state == TOMBSTONE)
      {
        if (!tomb)
          tomb = e;
      }
      else if (e->hash == hash && Eq{}(e->key, key))
      {
        return e;
      }

      index = (index + 1) & mask();
    }
  }

  Entry *findFilled(const K &key, size_t hash) const
  {
    if (capacity == 0)
      return nullptr;
    size_t index = hash & mask();

    for (;;)
    {
      Entry *e = &entries[index];
      if (e->state == EMPTY)
        return nullptr;
      if (e->state == FILLED && e->hash == hash && Eq{}(e->key, key))
        return e;
      index = (index + 1) & mask();
    }
  }

  void adjustCapacity(size_t newCap)
  {
    Entry *old = entries;
    size_t oldCap = capacity;

    // Aloca da arena
    entries = (Entry *)aAlloc(newCap * sizeof(Entry));
    std::memset(entries, 0, newCap * sizeof(Entry)); // Inicializa como EMPTY

    capacity = newCap;
    count = 0;
    tombstones = 0;

    if (old)
    {
      for (size_t i = 0; i < oldCap; i++)
      {
        Entry *e = &old[i];
        if (e->state == FILLED)
        {
          Entry *dst = findSlot(e->key, e->hash);
          *dst = *e;
          dst->state = FILLED;
          count++;
        }
      }
      aFree(old);
    }
  }

  void maybeGrow()
  {
    if (capacity == 0 || (count + tombstones + 1) > capacity * MAX_LOAD)
    {
      size_t newCap = capacity == 0 ? 16 : capacity * 2;
      adjustCapacity(newCap);
    }
  }

  bool set(const K &key, const V &value)
  {
    maybeGrow();
    size_t h = Hasher{}(key);
    Entry *e = findSlot(key, h);

    bool isNew = (e->state != FILLED);
    if (isNew)
    {
      if (e->state == TOMBSTONE)
        tombstones--;
      e->key = key;
      e->hash = h;
      e->state = FILLED;
      count++;
    }
    e->value = value;
    return isNew;
  }


    bool set_move(const K &key,  V &&value)
  {
    maybeGrow();
    size_t h = Hasher{}(key);
    Entry *e = findSlot(key, h);

    bool isNew = (e->state != FILLED);
    if (isNew)
    {
      if (e->state == TOMBSTONE)
        tombstones--;
      e->key = key;
      e->hash = h;
      e->state = FILLED;
      count++;
    }
    e->value = std::move(value);
    return isNew;
  }

  bool set_get(const K &key, const V &value, V *out)
  {
    maybeGrow();
    size_t h = Hasher{}(key);
    Entry *e = findSlot(key, h);

    bool isNew = (e->state != FILLED);
    if (isNew)
    {
      if (e->state == TOMBSTONE)
        tombstones--;
      e->key = key;
      e->hash = h;
      e->state = FILLED;
      count++;
    }
    if (!isNew)
    {
      *out = e->value;
    }
    e->value = value;
    return isNew;
  }

  bool get(const K &key, V *out) const
  {
    if (count == 0)
      return false;
    size_t h = Hasher{}(key);
    Entry *e = findFilled(key, h);
    if (!e)
      return false;
    *out = e->value;
    return true;
  }

  bool exist(const K &key) const
  {
    if (count == 0)
      return false;
    size_t h = Hasher{}(key);
    Entry *e = findFilled(key, h);
    if (!e)
      return false;

    return true;
  }

  template <typename Fn>
  void forEach(Fn fn) const
  {
    for (size_t i = 0; i < capacity; i++)
    {
      if (entries[i].state == FILLED)
      {
        fn(entries[i].key, entries[i].value);
      }
    }
  }

  //  early exit
  template <typename Fn>
  void forEachWhile(Fn fn) const
  {
    for (size_t i = 0; i < capacity; i++)
    {
      if (entries[i].state == FILLED)
      {
        if (!fn(entries[i].key, entries[i].value))
          break; // fn retorna bool
      }
    }
  }
};