#include <typeinfo>
#include <functional>

class Value {
    void* _val = nullptr;
    size_t _ty_hash = 0;
    std::function<void (void*)> _destructor = nullptr;
    std::function<void (Value& lhs, Value& rhs)> _copy = nullptr;
public:
    template<class Ty>
    void Set(Ty val) {
        if (_destructor != nullptr) {
            _destructor(_val);
        }

        auto new_val = new Ty(val);
        _val = (void*) new_val;

        _destructor =  [](void* val) {
            delete ((Ty*)val);
        };
        _copy =  [](Value& lhs, Value& rhs) {
            if(lhs._ty_hash != rhs._ty_hash) {
                if (lhs._destructor != nullptr) {
                    lhs._destructor(lhs._val);
                }
                auto new_val = new Ty(*((Ty*)rhs._val));
                lhs._val = (void*) new_val;
                lhs._ty_hash = rhs._ty_hash;
                lhs._destructor = rhs._destructor;
            } else {
                *((Ty*)lhs._val) = *((Ty*)rhs._val);
            }
        };

        const std::type_info& ty_info = typeid(Ty);
        _ty_hash = ty_info.hash_code();
    }

    template<class Ty>
    Ty Get(){
        const std::type_info& ty_info = typeid(Ty);
        size_t ty_hash = ty_info.hash_code();
        if (ty_hash != _ty_hash) {
            throw std::runtime_error("Can't get type");;
        }
        return *((Ty*)_val);
    }
    virtual ~Value(){
        if (_destructor) {
            _destructor(_val);
        }
    }
    Value() {
        _val = nullptr;
        _ty_hash = 0;
        _destructor = nullptr;
        _copy = nullptr;
    }
    Value(Value&& rhs) {
        RhsAssign(std::move(rhs));
    }
    Value(Value& rhs) {
        LhsAssign(rhs);
    }
    Value operator=(Value&& rhs){
        RhsAssign(std::move(rhs));
        return *this;
    }
    Value operator=(Value& rhs){
        LhsAssign(rhs);
        return *this;
    }

private:
    void RhsAssign(Value&& rhs) {
        _val = rhs._val;
        _ty_hash = rhs._ty_hash;
        _destructor = rhs._destructor;
        _copy = rhs._copy;

        rhs._val = nullptr;
        rhs._ty_hash = 0;
        rhs._destructor = nullptr;
        rhs._copy = nullptr;
    }
    void LhsAssign(Value& rhs) {
        if (rhs._copy) {
            rhs._copy(*this, rhs);
        }
    }
};
