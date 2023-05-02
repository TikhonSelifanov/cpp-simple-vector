#pragma once
#include "array_ptr.h"
#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <iostream>

struct Wrapper
{
    Wrapper(size_t capacity_to_reserve) : to_reserve(capacity_to_reserve)
    {

    }
    size_t to_reserve;
};

Wrapper Reserve(size_t capacity_to_reserve)
{
    return Wrapper(capacity_to_reserve);
}

template <typename Type>
class SimpleVector
{
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // vector(Reserve(size_t))
    SimpleVector(Wrapper wrap)
    {
        Reserve(wrap.to_reserve);
    }

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
        : items_(size),
          size_(size),
          capacity_(size)
    {
        std::generate(begin(), end(), []()
        {
            return Type{};
        });
        // А разве не нужен try catch, чтобы когда выбрасывается исключение, вернуть объект в нулевое состояние?
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : items_(size),
          size_(size),
          capacity_(size)
    {
        // может выбросить bad_alloc
        std::generate(begin(), end(), [&value]()
        {
            return value;
        });
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
        : items_(init.size()),
          size_(init.size()),
          capacity_(init.size())
    {
        std::copy(init.begin(), init.end(), begin());
    }

    // Copy constructor
    SimpleVector(const SimpleVector& other)
        : items_(other.GetSize()),
          size_(other.GetSize()),
          capacity_(other.GetCapacity())
    {
        std::copy(other.begin(), other.end(), begin());
    }

    SimpleVector(SimpleVector&& other) noexcept
    {
        items_ = std::move(other.items_);
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
    }

    SimpleVector& operator=(SimpleVector&& rhs) noexcept
    {
        if (*this != rhs)
        {
            SimpleVector temp(std::move(rhs));
            this->swap(temp);
        }
        return *this;
    }

    SimpleVector& operator=(const SimpleVector& rhs) noexcept
    {
        if (*this != rhs)
        {
            SimpleVector tmp(rhs);
            this->swap(tmp);
        }
        return *this;
    }

    void Reserve(size_t new_capacity)
    {
        if (new_capacity > capacity_)
        {
            SimpleVector<Type> newVec(new_capacity);
            std::move(begin(), end(), newVec.begin());
            this->items_.swap(newVec.items_);
            capacity_ = new_capacity;
        }
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item)
    {
        if (size_ + 1 > capacity_)
        {
            Resize(size_ + 1);
            items_[size_ - 1] = item;
        }
        else
        {
            items_[size_++] = item;
        }
    }

    void PushBack(Type&& item)
    {
        if (size_ + 1 > capacity_)
        {
            Resize(size_ + 1);
            items_[size_ - 1] = std::move(item);
        }
        else
        {
            items_[size_++] = std::move(item);
        }
    }

    void ShiftArrayWithInsertNoMove(const Type& value, int index)
    {
        for (int i = size_ - 1; i >= index; --i)
        {
            items_[i + 1] = items_[i];
        }
        items_[index] = value;
    }

    void ShiftArrayWithInsertAndMove(Type&& value, int index)
    {
        for (int i = size_ - 1; i >= index; --i)
        {
            items_[i + 1] = std::move(items_[i]);
        }
        items_[index] = std::move(value);
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value)
    {
        assert(pos >= cbegin() && pos <= cend());
        if (IsEmpty())
        {
            PushBack(value);
            return begin();
        }

        int index = pos - begin();
        if (size_ != capacity_)
        {
            ShiftArrayWithInsertNoMove(value, index);
            ++size_;
        }
        else
        {
            Resize(size_ + 1); // Размерность меняется в самом ресайз
            ShiftArrayWithInsertNoMove(value, index);
        }
        return begin() + index;
    }

    // Insert для rvalue ссылки
    Iterator Insert(ConstIterator pos, Type&& value)
    {
        assert(pos >= cbegin() && pos <= cend());
        if (IsEmpty())
        {
            PushBack(std::move(value));
            return begin();
        }
        int index = pos - begin();
        if (size_ != capacity_)
        {
            ShiftArrayWithInsertAndMove(std::move(value), index);
            ++size_;
        }
        else
        {
            Resize(size_ + 1);
            ShiftArrayWithInsertAndMove(std::move(value), index);
        }
        return begin() + index;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept
    {
        assert(size_ != 0);
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos)
    {
        assert(pos >= cbegin() && pos <= cend());
        if (!IsEmpty())
        {
            size_t index = pos - begin(); // с ассертом переполнения не должно быть (худший случай begin - begin)
            for (size_t i = index; i < size_ - 1; ++i)
            {
                items_[i] = std::move(items_[i + 1]);
            }
            --size_;
            return begin() + index;
        }
        return Iterator{};
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept
    {
        items_.swap(other.items_);
        std::swap(other.size_, size_);
        std::swap(other.capacity_, capacity_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept
    {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept
    {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept
    {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept
    {
        assert(index <= size_);
        return items_.Get()[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept
    {
        assert(index <= size_);
        return items_.Get()[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index)
    {
        if (index >= size_)
        {
            throw std::out_of_range("Index is out of range");
        }
        return items_.Get()[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const
    {
        if (index >= size_)
        {
            throw std::out_of_range("");
        }
        return items_.Get()[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept
    {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size)
    {
        if (size_ == new_size)
        {
            return;
        }

        if (new_size > capacity_)
        {
            SimpleVector<Type> newVec(new_size);
            std::move(begin(), end(), newVec.begin());
            this->swap(newVec);

            capacity_ = std::max(capacity_ * 2, new_size);
        }
        else if (new_size <= capacity_)
        {
            for (size_t i = size_ - 1; i < new_size; ++i)
            {
                items_.Get()[i] = Type{};
            }
        }
        size_ = new_size;
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept
    {
        return &items_.Get()[0]; // не могу использовать ...Get() и Get() + size_, в 231 строке ошибка будет
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept
    {
        return &items_.Get()[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept
    {
        return &items_.Get()[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept
    {
        return &items_.Get()[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept
    {
        return &items_.Get()[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept
    {
        return &items_.Get()[size_];
    }

private:
    ArrayPtr<Type> items_ = {};
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    if (lhs.GetSize() != rhs.GetSize())
    {
        return false;
    }
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return !(rhs == lhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    if (lhs == rhs)
    {
        return false;
    }
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return !(rhs > lhs);
}
