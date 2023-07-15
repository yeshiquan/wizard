#include <iostream>
#include <type_traits>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string_view>

template<typename Container>
void print_feature(std::string label, Container &arr) {
    std::cout << label << " -> [";
    if (arr.size() > 0) {
        for (int i = 0; i < arr.size() - 1; ++i) {
            std::cout << arr.at(i) << ",";
        }
        std::cout << arr.at(arr.size() - 1);
    }
    std::cout << "] length:" << arr.size() << std::endl;
}

// ================== StaticTypeId Begin ===================
template <typename T> static inline const char *helper1() {
  // Must have the same signature as helper2().
  return __PRETTY_FUNCTION__ + __builtin_strlen(__FUNCTION__);
}

template <typename T> static inline const char *helper2() {
  return __PRETTY_FUNCTION__ + __builtin_strlen(helper1<void>()) + __builtin_strlen(__FUNCTION__) - 5;
}

template <typename T> static inline std::string_view type_name() {
  return {helper2<T>(), __builtin_strlen(helper2<T>()) - 1};
}

template <class T> class StaticTypeId {
public:
  static const ::std::string TYPE_NAME;
  static const std::string_view name() { return type_name<T>(); }
};

template <class T>
const ::std::string
    StaticTypeId<T>::TYPE_NAME = std::string(StaticTypeId<T>::name());
// ================== StaticTypeId End ===================

template<typename... Ts>
struct TypeList {};

template<class... TYPES>
struct VisitorBase {
    using types = TypeList<TYPES...>;
};

template<typename ... Ts>
struct RemoveFunctor : VisitorBase<Ts ...>  {
   inline RemoveFunctor(std::size_t b, std::size_t e)
        : begin (b), end (e)  {   }

    const std::size_t   begin;
    const std::size_t   end;

    template<typename T>
    void operator() (T &vec) {
        //std::cout << "before remove functor size:" << vec.size() << " " << StaticTypeId<T>::TYPE_NAME << std::endl;
        for (std::size_t i = begin; i < end; ++i) {
            vec.remove(i);
        }
    }
};

using IdxList = std::vector<int>;
using IdxListPtr = std::shared_ptr<IdxList>;

// 一种可以逻辑删除的数组容器，避免stl vector删除数据时发生大量移动和拷贝
template<typename T>
class DataVector {
public:
    DataVector() {
        idx_ = std::make_shared<IdxList>();
    }
    DataVector(IdxListPtr idx) {
        idx_ = std::move(idx);
    }
    DataVector(const DataVector& other) {
        data_ = other.data_;
        idx_ = other.idx_;
    }
    void swap(DataVector &other) {
        data_.swap(other.data_);
        idx_.swap(other.idx_);
    }
    DataVector &operator=(const DataVector &other) {
        DataVector(other).swap(*this);
        return *this;
    }
    void set_idx(IdxListPtr idx) {
        idx_ = std::move(idx);
    }
    void push_back(T value) {
        int new_idx = data_.size();
        //std::cout << "DataVector push_back() new_idx:" << new_idx << std::endl;
        //print_feature("idx_", *idx_);
        if (idx_->size() == 0 || new_idx > idx_->back()) {
            idx_->push_back(new_idx);
        }
        data_.push_back(value);
    }
    T& at(size_t i) {
        size_t idx = (*idx_)[i];
        return data_[idx];
    }
    void remove(size_t i) {
        idx_->erase(idx_->begin() + i);
    }
    size_t size() const { return idx_->size(); }
private:
    std::vector<T> data_;
    IdxListPtr idx_;
};

// 一种可以存储异构数据的vector容器，来源于：
// https://gieseanw.wordpress.com/2017/05/03/a-true-heterogeneous-container-in-c/
class HeterogeneousVector {
public:
    HeterogeneousVector() = default;
    HeterogeneousVector(int id) : id_(id) {
    };
    
    template<class T>
    void push_back(const T& value) {
        items<T>.at(id_)->push_back(value);
    }

    template<typename T>
    DataVector<T>& get_vector() {
        return *items<T>.at(id_);
    }

    template<typename T>
    DataVector<T>& get_or_create_vector(IdxListPtr &idx) {
        if (id_ >= items<T>.size()) {
            //std::cout << "Create New HeterogeneousVector id:" << id_ << std::endl;
            items<T>.resize(id_ + 1, nullptr);
            items<T>.at(id_) = std::make_shared<DataVector<T>>(idx);        
        }
        //print_items<T>();
        return *items<T>.at(id_);
    }

    template<typename T>
    void print_items() {
        std::cout << "type:" << StaticTypeId<T>::TYPE_NAME << " id:" << id_ << " [";
        for (int i = 0; i < items<T>.size(); ++i) {
            std::cout << items<T>[i].get() << ",";
        }
        std::cout << "]" << std::endl;
    }

    template<typename F, typename T>
    void change_impl_help(F &functor)  {
        //print_items<T>();
        if (id_ < items<T>.size() && items<T>.at(id_)) {
            functor(*items<T>.at(id_));
        }
    }    

    template<class F, template<class...> class TLIST, class... TYPES>
    void change_impl(F &&functor, TLIST<TYPES...>)  {
    using expander = int[];
        (void) expander { 0, (change_impl_help<F, TYPES>(functor), 0) ... };
    }

    template<typename F>
    void change (F &&functor) {
        change_impl(functor, typename std::decay_t<F>::types { });
    }
    
    ~HeterogeneousVector() {
    }
    
private:
    template<class T>
    static std::vector<std::shared_ptr<DataVector<T>>> items;
    int id_ = -1;
};

template<class T>
std::vector<std::shared_ptr<DataVector<T>>> HeterogeneousVector::items;

class FeatureTable {
public:
    FeatureTable() {
        idx_ = std::make_shared<IdxList>();
    }
    FeatureTable(size_t feature_size) : columns_(feature_size) {
        idx_ = std::make_shared<IdxList>();
    }

    template<typename T>
    void append_column(int feature_id, const T& value) {
        std::vector<T> &column_data = get_column<T>(feature_id);
        column_data.emplace_back(value);
    }

    template<typename T>
    DataVector<T> &create_column(int feature_id) {
        columns_[feature_id] = std::make_shared<HeterogeneousVector>(feature_id);
        return get_and_init_column<T>(feature_id);
    }

    template<typename T>
    DataVector<T> &get_column(int feature_id) {
        return columns_[feature_id]->get_vector<T>();
    }

    template<typename T>
    DataVector<T> &get_and_init_column(int feature_id) {
        return columns_[feature_id]->get_or_create_vector<T>(idx_);
    }

    void remove_row(int row) {
        // 所有DataVector共享一份索引数据
        // 删除一行数据变得如此简单、高效
        idx_->erase(idx_->begin() + row);
    }
private:
    std::vector<std::shared_ptr<HeterogeneousVector>> columns_;
    IdxListPtr idx_;
};

enum FeatureId {
    F_UID,
    F_CLICK,
    F_TITLE,
    F_TAG,
    FEATURE_SIZE,
};

// 自定义的复杂正排数据
struct FeatureWeightInteger {
    int id;
    float weight;
    FeatureWeightInteger(int id_, float weight_) {
        id = id_;
        weight = weight_;
    }
    friend std::ostream& operator<<(std::ostream& os, const FeatureWeightInteger& value);
};

std::ostream& operator<<(std::ostream& os, const FeatureWeightInteger& value) {
    os << "{" << value.id << "," << value.weight << "}";
    return os;
}

int main() {
    FeatureTable table(FEATURE_SIZE);
    auto &f_uid = table.create_column<int>(F_UID);
    f_uid.push_back(111);
    f_uid.push_back(222);
    f_uid.push_back(333);

    auto &f_click = table.create_column<FeatureWeightInteger>(F_CLICK);
    f_click.push_back(FeatureWeightInteger(1901, 1.42));
    f_click.push_back(FeatureWeightInteger(1902, 1.02));
    f_click.push_back(FeatureWeightInteger(1903, 1.24));

    auto &f_title = table.create_column<std::string>(F_TITLE);
    f_title.push_back(std::string("title1"));
    f_title.push_back(std::string("title2"));
    f_title.push_back(std::string("title3"));

    auto &f_tag = table.create_column<std::string>(F_TAG);
    f_tag.push_back("news");
    f_tag.push_back("video");
    f_tag.push_back("military");

    print_feature("f_uid", f_uid);
    print_feature("f_click", f_click);
    print_feature("f_title", f_title);
    print_feature("f_tag", f_tag);

    // 删除第1行
    std::cout << "\n------ remove row 1 ------\n" << std::endl;
    table.remove_row(1);

    print_feature("f_uid", f_uid);
    print_feature("f_click", f_click);
    print_feature("f_title", f_title);
    print_feature("f_tag", f_tag);

    // 删除第0行
    std::cout << "\n------ remove row 0 ------\n" << std::endl;
    table.remove_row(0);    

    print_feature("f_uid", f_uid);
    print_feature("f_click", f_click);
    print_feature("f_title", f_title);
    print_feature("f_tag", f_tag);

    // 再插入一行数据
    std::cout << "\n------ append row ------\n" << std::endl;
    f_uid.push_back(444);
    f_click.push_back(FeatureWeightInteger(1904, 1.24));
    f_title.push_back(std::string("title4"));
    f_tag.push_back("life");

    print_feature("f_uid", f_uid);
    print_feature("f_click", f_click);
    print_feature("f_title", f_title);
    print_feature("f_tag", f_tag);

    return 0;
}
