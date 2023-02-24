#pragma once

#include "nodes.h"
#include "treap.h"
#include <stdexcept>

template <typename L, typename R, typename CL, typename CR>
struct bimap;

namespace iterator {
template <typename Left, typename Right, typename Tag>
struct bimap_iterator {
  using node_t = base_node;
  using T = typename tags::key<Left, Right, Tag>::type;
  using other_tag = typename tags::other_tag<Tag>::type;

  template <typename L, typename R, typename CL, typename CR>
  friend struct ::bimap;

  friend bimap_iterator<Left, Right, other_tag>;

  // Элемент на который сейчас ссылается итератор.
  // Разыменование итератора end_left() неопределено.
  // Разыменование невалидного итератора неопределено.
  T const& operator*() const {
    return get_value();
  }

  T const* operator->() const {
    return &get_value();
  }

  // Переход к следующему по величине left'у.
  // Инкремент итератора end_left() неопределен.
  // Инкремент невалидного итератора неопределен.
  bimap_iterator& operator++() {
    ptr = ptr->next();
    return *this;
  }

  bimap_iterator operator++(int) {
    bimap_iterator res(*this);
    ++(*this);
    return res;
  }

  // Переход к предыдущему по величине left'у.
  // Декремент итератора begin_left() неопределен.
  // Декремент невалидного итератора неопределен.
  bimap_iterator& operator--() {
    ptr = ptr->prev();
    return *this;
  }

  bimap_iterator operator--(int) {
    bimap_iterator res(*this);
    --(*this);
    return res;
  }

  friend bool operator==(bimap_iterator const& lhs, bimap_iterator const& rhs) {
    return lhs.ptr == rhs.ptr;
  }

  friend bool operator!=(bimap_iterator const& lhs, bimap_iterator const& rhs) {
    return lhs.ptr != rhs.ptr;
  }

  // left_iterator ссылается на левый элемент некоторой пары.
  // Эта функция возвращает итератор на правый элемент той же пары.
  // end_left().flip() возращает end_right().
  // end_right().flip() возвращает end_left().
  // flip() невалидного итератора неопределен.
  bimap_iterator<Left, Right, other_tag> flip() const {
    return static_cast<empty_node<other_tag>*>(
        static_cast<base_bimap_node*>(static_cast<empty_node<Tag>*>(ptr)));
  }

private:
  node_t* ptr;

  T const& get_value() const {
    return casts::down_cast<Left, Right, Tag>(ptr)->template get_value<Tag>();
  }

  bimap_iterator(base_node* n) : ptr(n) {}
};
} // namespace iterator

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {
  using left_t = Left;
  using right_t = Right;
  using node_t = bimap_node<left_t, right_t>;

  using left_iterator = iterator::bimap_iterator<Left, Right, tags::left_tag>;
  using right_iterator = iterator::bimap_iterator<Left, Right, tags::right_tag>;

  // Создает bimap не содержащий ни одной пары.
  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : root(), left_tree(&root, std::move(compare_left)),
        right_tree(&root, std::move(compare_right)), size_(0) {}

  // Конструкторы от других и присваивания
  bimap(bimap const& other) : bimap() {
    for (auto it = other.begin_left(); it != other.end_left(); ++it) {
      insert(*it, *it.flip());
    }
  }

  bimap(bimap&& other) noexcept
      : root(std::move(other.root)), left_tree(std::move(other.left_tree)),
        right_tree(std::move(other.right_tree)), size_(other.size_) {}

  bimap& operator=(bimap const& other) {
    if (*this != other) {
      bimap tmp(other);
      swap(tmp);
    }
    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    if (*this != other) {
      bimap tmp(std::move(other));
      swap(tmp);
    }
    return *this;
  }

  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() {
    erase_left(begin_left(), end_left());
  }

  // Вставка пары (left, right), возвращает итератор на left.
  // Если такой left или такой right уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().
  left_iterator insert(left_t const& left, right_t const& right) {
    return insert_impl(left, right);
  }

  left_iterator insert(left_t const& left, right_t&& right) {
    return insert_impl(left, std::move(right));
  }

  left_iterator insert(left_t&& left, right_t const& right) {
    return insert_impl(std::move(left), right);
  }

  left_iterator insert(left_t&& left, right_t&& right) {
    return insert_impl(std::move(left), std::move(right));
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) {
    auto tmp = it;
    ++tmp;
    left_tree.erase(it.ptr);
    right_tree.erase(it.flip().ptr);
    --size_;
    delete casts::down_cast<Left, Right, tags::left_tag>(it.ptr);
    return tmp;
  }

  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(left_t const& left) {
    auto it = find_left(left);
    if (it != end_left()) {
      erase_left(it);
      return true;
    }
    return false;
  }

  right_iterator erase_right(right_iterator it) {
    auto tmp = it;
    ++tmp;
    left_tree.erase(it.flip().ptr);
    right_tree.erase(it.ptr);
    --size_;
    delete casts::down_cast<Left, Right, tags::right_tag>(it.ptr);
    return tmp;
  }

  bool erase_right(right_t const& right) {
    auto it = find_right(right);
    if (it != end_right()) {
      erase_right(it);
      return true;
    }
    return false;
  }

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью
  left_iterator erase_left(left_iterator first, left_iterator last) {
    for (auto it = first; it != last;) {
      auto next = it.ptr->next();
      erase_left(it);
      it = next;
    }
    return last;
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
    for (auto it = first; it != last;) {
      auto next = it.ptr->next();
      erase_right(it);
      it = next;
    }
    return last;
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(left_t const& left) const {
    base_node* res = left_tree.find(left);
    return res ? res : end_left();
  }

  right_iterator find_right(right_t const& right) const {
    base_node* res = right_tree.find(right);
    return res ? res : end_right();
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  right_t const& at_left(left_t const& key) const {
    left_iterator res = find_left(key);
    if (res == end_left()) {
      throw std::out_of_range("Not found key");
    }
    return *res.flip();
  }

  left_t const& at_right(right_t const& key) const {
    right_iterator res = find_right(key);
    if (res == end_right()) {
      throw std::out_of_range("Not found key");
    }
    return *res.flip();
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)
  template <typename R = right_t,
            typename = std::enable_if_t<std::is_default_constructible_v<R>>>
  right_t const& at_left_or_default(left_t const& key) {
    left_iterator res = find_left(key);
    if (res == end_left()) {
      right_t new_right = right_t();
      erase_right(new_right);
      return *(insert(key, std::move(new_right)).flip());
    } else {
      return *res.flip();
    }
  }

  template <typename L = left_t,
            typename = std::enable_if_t<std::is_default_constructible_v<L>>>
  left_t const& at_right_or_default(right_t const& key) {
    right_iterator res = find_right(key);
    if (res == end_right()) {
      left_t new_left = left_t();
      erase_left(new_left);
      return *(insert(std::move(new_left), key));
    } else {
      return *res.flip();
    }
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
  left_iterator lower_bound_left(const left_t& left) const {
    return left_tree.lower_bound(left);
  }

  left_iterator upper_bound_left(const left_t& left) const {
    return left_tree.upper_bound(left);
  }

  right_iterator lower_bound_right(const right_t& right) const {
    return right_tree.lower_bound(right);
  }

  right_iterator upper_bound_right(const right_t& right) const {
    return right_tree.upper_bound(right);
  }

  // Возващает итератор на минимальный по порядку left.
  left_iterator begin_left() const {
    return left_tree.begin();
  }

  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const {
    return left_tree.end();
  }

  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const {
    return right_tree.begin();
  }
  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const {
    return right_tree.end();
  }

  // Проверка на пустоту
  bool empty() const {
    return size_ == 0;
  }

  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const {
    return size_;
  }

  // операторы сравнения
  friend bool operator==(bimap const& a, bimap const& b) {
    if (a.size() == b.size()) {
      for (auto it_a = a.begin_left(), it_b = b.begin_left();
           it_a != a.end_left(); ++it_a, ++it_b) {
        if (!a.left_tree.equal(*it_a, *it_b) ||
            !a.right_tree.equal(*(it_a.flip()), *(it_b.flip()))) {
          return false;
        }
      }
      return true;
    }
    return false;
  }

  friend bool operator!=(bimap const& a, bimap const& b) {
    return !(a == b);
  }

  void swap(bimap& other) {
    left_tree.swap(other.left_tree);
    right_tree.swap(other.right_tree);
    std::swap(size_, other.size_);
  }

private:
  template <typename L, typename R>
  left_iterator insert_impl(L&& left, R&& right) {
    if (find_left(left) == end_left() && find_right(right) == end_right()) {
      auto* new_node =
          new node_t(std::forward<L>(left), std::forward<R>(right));
      base_node* l = casts::up_cast<Left, Right, tags::left_tag>(new_node);
      base_node* r = casts::up_cast<Left, Right, tags::right_tag>(new_node);
      left_tree.insert(l);
      right_tree.insert(r);
      ++size_;
      return l;
    } else {
      return end_left();
    }
  }

  base_bimap_node root;
  tree<left_t, right_t, tags::left_tag, CompareLeft> left_tree;
  tree<left_t, right_t, tags::right_tag, CompareRight> right_tree;
  std::size_t size_;
};
