#pragma once

#include <algorithm>
#include <atomic>
#include <limits>
#include <iostream>
#include <memory>
#include <tuple>
#include <type_traits>

#include "skiplist-inl.h"

namespace rcu {

template<typename T, 
         typename Comp = std::less<T>,
         typename NodeAlloc = std::allocator<uint8_t>, 
         uint32_t MAX_HEIGHT = 24>
class SkipList {
    using SkipListType = SkipList<T, Comp, NodeAlloc, MAX_HEIGHT>;
public:
    using NodeType = detail::SkipListNode<T> ;
    using ValueType = T;

    SkipList(uint8_t height) : alloc_(NodeAlloc()) {
        head_ = NodeType::create(alloc_, height, ValueType());
    }
    ~SkipList() {
        NodeType* node = head_;
        while (node) {
            NodeType* node_next = node->skip(0);
            NodeType::destroy(alloc_, node);
            node = node_next;
        }
    }

    bool insert(const ValueType& data) {
        int node_height = get_random_height();
        std::cout<< "insert() node_height:" << node_height << " data:" << data << std::endl;
        auto* new_node = NodeType::create(alloc_, node_height, data);
        bool is_exist = find_insert_point(new_node, data);
        if (is_exist) {
            NodeType::destroy(alloc_, new_node);
        }
        return is_exist;
    }

    void remove(const ValueType& data) {
        std::cout << "remove(" << data << ")" << std::endl;
        int layer = height() - 1;
        NodeType* node = head_;
        NodeType* node_next = nullptr;
        NodeType* to_delete_node = nullptr;
        while (layer >= 0) {
            // 双指针：寻找 node < data <= node_next 这样的点
            node_next = node->skip(layer);
            while (node_next && greater(data, node_next->data())) {
                node = node_next;
                node_next = node_next->skip(layer);
            }
            if (node_next && equal(data, node_next->data())) {
                to_delete_node = node_next;
                node->set_skip(layer, node_next->skip(layer));
            }
            layer--;
        }
        if (to_delete_node != nullptr) {
            NodeType::destroy(alloc_, to_delete_node);
        }
    }

    bool find_insert_point(NodeType* new_node, const ValueType& data) {
        int layer = new_node->height() - 1;
        NodeType* node = head_;
        NodeType* node_next = nullptr;
        std::vector<std::tuple<NodeType*, NodeType*, int>> items;
        items.reserve(layer+1);
        bool is_exist = false;
        while (layer >= 0) {
            // 双指针：寻找 node < data <= node_next 这样的点
            node_next = node->skip(layer);
            while (node_next && greater(data, node_next->data())) {
                node = node_next;
                node_next = node_next->skip(layer);
            }
            if (node_next && equal(data, node_next->data())) {
                is_exist = true;
                break;
            }
            items.emplace_back(std::make_tuple(node, node_next, layer));
            layer--;
        }

        if (!is_exist) {
            for (auto& item : items) {
                node = std::get<0>(item);
                node_next = std::get<1>(item);
                layer = std::get<2>(item);
                node->set_skip(layer, new_node);
                new_node->set_skip(layer, node_next);
            }
        }

        return is_exist;
    }    

    bool find(const ValueType& data) {
        return _find_node_down_right(data);
    }

  void print_layer(NodeType* start, int layer) {
      NodeType* node = start;
      std::cout << "layer[" << layer << "] -> ";
      while (node) {
          std::cout << node->data() << ",";
          node = node->skip(layer);
      }
      std::cout << std::endl;
  }

  void print() {
      printf("\n============ SkipList print begin ================\n");
      auto top = maxLayer();
      for (int layer = top; layer >= 0; --layer) {
          print_layer(head_->skip(layer), layer);
      }
      printf("============ SkipList print end ================\n\n");
  }
    int int_rand(const int & min, const int & max) {
        static thread_local std::mt19937 generator;
        std::uniform_int_distribution<int> distribution(min,max);
        return distribution(generator);
    }
    int get_random_height() {
        int K = 1;
        while (int_rand(0, 1) && K < height()) {
            K++;
        }
        return K;
    }
  int height() const { return head_->height(); }
  int maxLayer() const { return height() - 1; }  
private:
    bool _find_node_down_right(const ValueType& data) {
        std::cout << "find(" << data << ")" << std::endl;
        NodeType* node = head_;
        int layer = head_->height() - 1;
        NodeType* node_next = nullptr;

        bool found = false;
        while (true) {
            // 双指针：寻找 node < node_next <= data 的节点
            while (layer >= 0) {
                node_next = node->skip(layer);
                if (node_next == nullptr || greater(node_next->data(), data)) {
                    // 跳过 node_next > data的层, node_next==nullptr当做无穷大来处理
                    layer--;
                } else {
                    break;
                }
            }

            //std::cout << "find() layer:" << layer << " node_next:" << node_next << std::endl;
            if (layer < 0) {
                found = false;
                break;
            }

            // 双指针：寻找 node < data <= node_next 的节点
            while (node_next && less(node_next->data(), data)) {
                node = node_next;
                node_next = node_next->skip(layer);
            }
            if (node_next != nullptr && equal(node_next->data(), data)) {
                found = true;
                break;
            }
        }
        return found;
    }
    
  static bool greater(const ValueType &data1, const ValueType& data2) {
    return Comp()(data2, data1);
  }
  static bool less(const ValueType &data1, const ValueType& data2) {
    return Comp()(data1, data2);
  }
  static bool greater_equal(const ValueType &data1, const ValueType& data2) {
    return !Comp()(data1, data2);
  }
  static bool less_equal(const ValueType &data1, const ValueType& data2) {
    return !Comp()(data2, data1);
  }  
  static bool equal(const ValueType &data1, const ValueType& data2) {
    return !Comp()(data2, data1) && !Comp()(data1, data2);
  }    
private:
    NodeType* head_{nullptr};
    size_t size_{0};
    NodeAlloc alloc_;
};

} // namespace
